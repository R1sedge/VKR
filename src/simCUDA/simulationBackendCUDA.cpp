#include "simulationBackendCUDA.h"

#include <cuda_runtime.h>

#include "common/Config.h"

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/utils/cudaSphKernels.cuh"

#include "simCUDA/kernels/cudaFillInstanceData.cuh"
#include "simCUDA/kernels/cudaKernelsBasic.cuh"

#include "simCUDA/pbf/cudaPbfDensity.cuh"
#include "simCUDA/pbf/cudaPbfLambda.cuh"
#include "simCUDA/pbf/cudaPbfDeltaPositions.cuh"
#include "simCUDA/pbf/cudaVorticity.cuh"
#include "simCUDA/pbf/cudaXSPH.cuh"

#include "simCUDA/constraints/cudaKernelsBounds.cuh"

#include "simCUDA/neighborSearch/neighborsGrid.cuh"

#include "scene/SceneFiller.h"


SimulationBackendCUDA::SimulationBackendCUDA()
{
    refreshCachedKernelConstants();
}

SimulationBackendCUDA::~SimulationBackendCUDA()
{
    releaseVesselPlanes();

    freeDeviceUniformGrid(m_grid);
    freeDeviceNeighborList(m_neighbors);
    freeDeviceParticles(m_deviceParticles);
}

void SimulationBackendCUDA::releaseVesselPlanes()
{
    if (m_dVesselPlanes)
    {
        CUDA_CHECK(cudaFree(m_dVesselPlanes));
        m_dVesselPlanes = nullptr;
    }

    m_vesselPlaneCount = 0;
}

void SimulationBackendCUDA::uploadInternalPatches(
    const std::vector<InternalBoundaryPatch>& patches)
{
    if (patches.empty())
    {
        clearInternalPatchesConstantMemory();
        return;
    }

    std::vector<CudaInternalBoundaryPatch> cudaPatches;
    cudaPatches.reserve(patches.size());
    for (const auto& p : patches)
        cudaPatches.push_back(toCuda(p));

    uploadInternalPatchesToConstantMemory(cudaPatches.data(), static_cast<int>(cudaPatches.size()));
}

void SimulationBackendCUDA::refreshCachedKernelConstants()
{
    // h и deltaQ сейчас берутся из Config и фактически глобальны для backend.
    const float dq = Config::artificialPressureDeltaQ * Config::smoothingRadius;
    m_cachedWDeltaQ = CudaSPH::poly6(dq, Config::smoothingRadius);
}

void SimulationBackendCUDA::setWorldBounds(float left, float right,
                                           float bottom, float top,
                                           float front, float back)
{
    m_gridBounds = AABB{
        left, right,
        bottom, top,
        front, back
    };
}


void SimulationBackendCUDA::reset()
{
   if (!m_hasLoadedScene)
        return;

    loadScene(m_loadedScene);
}

void SimulationBackendCUDA::setInteropVbo(GLuint vboId) 
{
    // Снять старую регистрацию если была
    unregisterInterop();
    CUDA_CHECK(cudaGraphicsGLRegisterBuffer(
        &m_vboResource,
        vboId,
        cudaGraphicsMapFlagsWriteDiscard));  // GPU пишет, GL читает
}

bool SimulationBackendCUDA::setupInterop(unsigned int vbo)
{
    setInteropVbo(vbo);
    fillInteropBuffer();
    return true;
}

void SimulationBackendCUDA::resetInterop(unsigned int vbo)
{
    unregisterInterop();
    setInteropVbo(vbo);
    fillInteropBuffer();
}

void SimulationBackendCUDA::unregisterInterop() 
{
    if (m_vboResource) 
    {
        cudaGraphicsUnregisterResource(m_vboResource);
        m_vboResource = nullptr;
    }
}

void SimulationBackendCUDA::fillInteropBuffer() 
{
    if (!m_vboResource || m_deviceParticles.count == 0) return;

    float* d_ptr = nullptr;
    size_t sz = 0;

    CUDA_CHECK(cudaGraphicsMapResources(1, &m_vboResource, 0));
    CUDA_CHECK(cudaGraphicsResourceGetMappedPointer(
        (void**)&d_ptr, &sz, m_vboResource));
        
    launchFillInstanceData(m_deviceParticles, d_ptr);
    CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_vboResource, 0));
}

void SimulationBackendCUDA::computeAngularVelocity(float dt)
{
    const glm::quat dq = m_vessel.orientation * glm::inverse(m_prevVesselOrientation);
    const float sinHalfAngle = glm::length(glm::vec3(dq.x, dq.y, dq.z));
    if (sinHalfAngle > 1e-6f && dt > 1e-6f)
    {
        const float angle = 2.0f * std::atan2(sinHalfAngle, dq.w);
        const glm::vec3 axis = glm::vec3(dq.x, dq.y, dq.z) / sinHalfAngle;
        m_vesselAngularVelocity = axis * (angle / dt);
    }
    else
    {
        m_vesselAngularVelocity = {0.0f, 0.0f, 0.0f};
    }
    m_prevVesselOrientation = m_vessel.orientation;
}

void SimulationBackendCUDA::update(float dt)
{
    if (m_deviceParticles.count <= 0)
        return;
    
    computeAngularVelocity(dt);

    launchClearDerived(m_deviceParticles);

    launchPredictPositions(
        m_deviceParticles,
        dt,
        Config::gravityX,
        Config::gravityY,
        Config::gravityZ,
        m_velocityDamping);

    buildNeighborsGridCUDA(
        m_deviceParticles,
        m_neighbors,
        m_grid,
        Config::smoothingRadius,
        m_gridBounds.xMin, m_gridBounds.xMax,
        m_gridBounds.yMin, m_gridBounds.yMax,
        m_gridBounds.zMin, m_gridBounds.zMax,
        Config::particleRadius, Config::enableBafflePairFiltering);

    for (int iter = 0; iter < iterations; ++iter)
    {   
        launchComputeDensity(
            m_deviceParticles,
            m_neighbors,
            Config::smoothingRadius);
        
        launchComputeLambda(
            m_deviceParticles,
            m_neighbors,
            Config::restDensity,
            Config::epsilon,
            Config::smoothingRadius);

        launchComputeDeltaPositions(
            m_deviceParticles,
            m_neighbors,
            Config::restDensity,
            Config::smoothingRadius,
            m_artPressureK,    
            m_cachedWDeltaQ);

        launchApplyDeltaPositions(m_deviceParticles);
        

        if (m_dVesselPlanes && m_vesselPlaneCount > 0)
        {
            launchProjectToVesselPlanes(
                m_deviceParticles,
                m_dVesselPlanes,
                m_vesselPlaneCount,
                Config::particleRadius);
        }
        else
        {
            launchProjectBounds(
                m_deviceParticles,
                m_gridBounds.xMin,
                m_gridBounds.xMax,
                m_gridBounds.yMin,
                m_gridBounds.yMax,
                m_gridBounds.zMin,
                m_gridBounds.zMax,
                Config::particleRadius);
        }

        launchProjectToInternalPatches(m_deviceParticles, Config::particleRadius);
    }

    launchUpdateVelocities(m_deviceParticles, dt, Config::maxSpeed, Config::particleRadius);

    if (m_vorticityEpsilon > 0.0f)
    {
        launchComputeVorticity(m_deviceParticles, m_neighbors,
                            Config::smoothingRadius);

        launchApplyVorticityConfinement(m_deviceParticles, m_neighbors,
                                        dt, m_vorticityEpsilon,
                                        Config::smoothingRadius);
    }

    launchApplyXSPH(m_deviceParticles, m_neighbors,
                    m_xsphViscosity, Config::smoothingRadius);

    if (m_dVesselPlanes && m_vesselPlaneCount > 0)
    {
        const glm::vec3 pivot = m_vessel.pivot;  // или {0,0,0} если поля нет
        launchApplyBoundaryVelocityResponse(
            m_deviceParticles,
            m_dVesselPlanes,
            m_vesselPlaneCount,
            Config::particleRadius,
            Config::wallRestitution,
            Config::wallFriction,
            m_vesselAngularVelocity.x,
            m_vesselAngularVelocity.y,
            m_vesselAngularVelocity.z,
            pivot.x, pivot.y, pivot.z);

        launchApplyInternalBaffleVelocityResponse(
            m_deviceParticles,
            Config::particleRadius,
            Config::wallRestitution,
            Config::wallFriction,
            m_vesselAngularVelocity.x, m_vesselAngularVelocity.y, m_vesselAngularVelocity.z,
            pivot.x, pivot.y, pivot.z);
    }

    // ======= CUDA-GL INTEROP: пишем в VBO прямо на GPU =======
    if (m_vboResource) 
    {
        float* d_ptr = nullptr;
        size_t sz = 0;

        CUDA_CHECK(cudaGraphicsMapResources(1, &m_vboResource, 0));
        CUDA_CHECK(cudaGraphicsResourceGetMappedPointer((void**)&d_ptr, &sz, m_vboResource));

        launchFillInstanceData(m_deviceParticles, d_ptr);

        // cudaGraphicsUnmapResources сам синхронизирует стрим перед return
        CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_vboResource, 0));
    } 
    else 
    {
        // Fallback: старый путь без interop
        CUDA_CHECK(cudaDeviceSynchronize());
        syncDeviceToHost();
    }
}

void SimulationBackendCUDA::syncDeviceToHost()
{
    downloadParticlesFromDevice(m_deviceParticles, m_particles);
}

const Particles3D& SimulationBackendCUDA::getParticles() const
{
    return m_particles;
}

void SimulationBackendCUDA::setVesselPlanes(const std::vector<BoundaryPlane>& planes)
{
    releaseVesselPlanes();

    if (planes.empty())
        return;

    std::vector<DeviceBoundaryPlane> hostDevicePlanes;
    hostDevicePlanes.reserve(planes.size());

    for (const BoundaryPlane& plane : planes)
        hostDevicePlanes.push_back(makeDeviceBoundaryPlane(plane));

    m_vesselPlaneCount = static_cast<int>(hostDevicePlanes.size());

    CUDA_CHECK(cudaMalloc(
        reinterpret_cast<void**>(&m_dVesselPlanes),
        sizeof(DeviceBoundaryPlane) * m_vesselPlaneCount));

    CUDA_CHECK(cudaMemcpy(
        m_dVesselPlanes,
        hostDevicePlanes.data(),
        sizeof(DeviceBoundaryPlane) * m_vesselPlaneCount,
        cudaMemcpyHostToDevice));
}

void SimulationBackendCUDA::loadScene(const SceneDescription& desc)
{
    m_loadedScene = desc;
    m_hasLoadedScene = true;

    // 1. Сохраняем геометрию сосуда
    m_vessel = desc.vessel;

    // 2. Строим частицы из новой scene-модели
    m_particles = SceneFiller::fill(desc);

    // 3. Загружаем частицы на устройство
    uploadParticlesToDevice(m_particles, m_deviceParticles);

    // 4. Вычисляем bounds для neighbour-grid из VesselBoundary
    const float gridMargin = Config::smoothingRadius + Config::particleRadius;
    m_gridBounds = m_vessel.computeGridAABB(gridMargin);

    // 5. прокидываем новый grid AABB
    setWorldBounds(m_gridBounds.xMin, m_gridBounds.xMax,
                   m_gridBounds.yMin, m_gridBounds.yMax,
                   m_gridBounds.zMin, m_gridBounds.zMax);

    // 6. Кэшировать мировые плоскости сосуда.
    m_worldPlanesCache = m_vessel.getWorldPlanes();

    // 7. Загружаем плоскости на GPU
    setVesselPlanes(m_worldPlanesCache);
    
    // 8. Внутренние перегородки → constant memory
    uploadInternalPatches(desc.vessel.getWorldInternalPatches());

    // interop-VBO ресайзит App после вызова: ensureInstanceBufferSize(n) + resetInterop(vbo)
}

void SimulationBackendCUDA::setVesselOrientation(const glm::quat& orientation)
{
    m_vessel.orientation = glm::normalize(orientation);
    m_worldPlanesCache = m_vessel.getWorldPlanes();
    setVesselPlanes(m_worldPlanesCache);
    uploadInternalPatches(m_vessel.getWorldInternalPatches());
}

