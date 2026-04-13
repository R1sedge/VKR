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
    reset();
}

SimulationBackendCUDA::~SimulationBackendCUDA()
{
    freeDeviceCollisionCheck(m_collisionCheck);
    freeDeviceNeighborList(m_neighbors);
    freeDeviceParticles(m_deviceParticles);
}

void SimulationBackendCUDA::setWorldBounds(float left, float right, float bottom, float top) //TODO Надо переименовать метод
{
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;

    // считаем один раз, т.к. h и deltaQ — константы
    const float dq = Config::artificialPressureDeltaQ * Config::smoothingRadius;
    m_cachedWDeltaQ = CudaSPH::poly6(dq, Config::smoothingRadius);
}

void SimulationBackendCUDA::reset()
{
    const float r = Config::particleRadius;
    const float step = r * 2.2f;

    const int cols = 125;
    const int rows = 80;
    const int n = cols * rows;

    m_particles.resize(n);

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const int idx = row * cols + col;

            const float fx = (col - cols / 2) * step;
            const float fy = (row - rows / 2) * step + 0.0f;

            m_particles.x[idx] = fx;
            m_particles.y[idx] = fy;
            m_particles.px[idx] = fx;
            m_particles.py[idx] = fy;
            m_particles.vx[idx] = 0.0f;
            m_particles.vy[idx] = 0.0f;
            m_particles.mass[idx] = 1.0f;
        }
    }

    m_particles.clearDerived();
    uploadParticlesToDevice(m_particles, m_deviceParticles);
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
        m_left, 
        m_right, 
        m_bottom,
        m_top);

    
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

        launchApplyDeltaPositions(m_deviceParticles, 1.0f);
        
        /*
        launchProjectParticleCollisions(
            m_deviceParticles,
            m_neighbors,
            Config::particleRadius
        );
        */

        launchProjectBounds(
            m_deviceParticles,
            m_left,
            m_right,
            m_bottom,
            m_top,
            Config::particleRadius);
    }

    launchUpdateVelocities(m_deviceParticles, dt, Config::maxSpeed,
        m_left, m_right, m_bottom,m_top, Config::particleRadius);

    if (m_vorticityEpsilon > 0.0f) 
    {
        launchComputeVorticity(m_deviceParticles, m_neighbors,
                            Config::smoothingRadius);

        launchApplyVorticityConfinement(m_deviceParticles, m_neighbors,
                                        dt, m_vorticityEpsilon,
                                        Config::smoothingRadius);
        
        launchApplyXSPH(m_deviceParticles, m_neighbors,
                        m_xsphViscosity, Config::smoothingRadius);
}

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

const Particles2D& SimulationBackendCUDA::getParticles() const
{
    return m_particles;
}

void SimulationBackendCUDA::loadScene(const SceneDescription& desc) {
    // 1. Применить гравитацию из сцены
    Config::gravityX = desc.gravityX;
    Config::gravityY = desc.gravityY;
    Config::gravityZ = desc.gravityZ;

    // 2. Заполнить CPU-буфер через SceneFiller
    m_particles = SceneFiller::fill(desc);

    // 3. Залить на устройство (realloc если n изменился)
    uploadParticlesToDevice(m_particles, m_deviceParticles);

    // Примечание: interop-VBO ресайзит App после вызова:
    // ensureInstanceBufferSize(n) + resetInterop(vbo)
}

void SimulationBackendCUDA::applyMouseForce(float worldX, float worldY,
                                            float radius, float strength,
                                            int forceType)
{
    launchApplyMouseForce(m_deviceParticles, worldX, worldY,
                         radius, strength, forceType);
}
