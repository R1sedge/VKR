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

#include "simCUDA/constraints/collisions/cudaParticleCollisionProject.cuh"
#include "simCUDA/constraints/cudaKernelsBounds.cuh"

#include "simCUDA/neighborSearch/neighborsGrid.cuh"
#include "simCUDA/kernels/cudaMouseForce.cuh"

#include "scene/SceneFiller.h"


SimulationBackendCUDA::SimulationBackendCUDA()
{
    refreshCachedKernelConstants();
}

SimulationBackendCUDA::~SimulationBackendCUDA()
{
    releaseVesselPlanes();
    freeDeviceCollisionCheck(m_collisionCheck);
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
        
    launchFillInstanceData(m_deviceParticles, d_ptr, Config::particleRadius);
    CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_vboResource, 0));
}

void SimulationBackendCUDA::update(float dt)
{
    if (m_deviceParticles.count <= 0)
        return;

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
        m_gridBounds.zMin, m_gridBounds.zMax);

    
    for (int iter = 0; iter < iterations; ++iter)
    {
        launchCheckParticleCollisions(
            m_deviceParticles, 
            m_neighbors,                     
            m_collisionCheck, 
            Config::particleRadius);

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

    // ======= CUDA-GL INTEROP: пишем в VBO прямо на GPU =======
    if (m_vboResource) 
    {
        float* d_ptr = nullptr;
        size_t sz = 0;

        CUDA_CHECK(cudaGraphicsMapResources(1, &m_vboResource, 0));
        CUDA_CHECK(cudaGraphicsResourceGetMappedPointer((void**)&d_ptr, &sz, m_vboResource));

        launchFillInstanceData(m_deviceParticles, d_ptr, Config::particleRadius);

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

    // interop-VBO ресайзит App после вызова: ensureInstanceBufferSize(n) + resetInterop(vbo)
}

void SimulationBackendCUDA::setVesselOrientation(const glm::quat& orientation)
{
    m_vessel.orientation = glm::normalize(orientation);
    m_worldPlanesCache = m_vessel.getWorldPlanes();
    setVesselPlanes(m_worldPlanesCache);
}

void SimulationBackendCUDA::applyMouseForce(float worldX, float worldY,
                                            float radius, float strength,
                                            int forceType)
{
    launchApplyMouseForce(m_deviceParticles, worldX, worldY,
                         radius, strength, forceType);
}
