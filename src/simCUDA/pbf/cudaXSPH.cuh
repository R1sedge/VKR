#pragma once
#include "simCUDA/utils/cudaParticles.cuh"
#include "simCUDA/neighborSearch/deviceNeighborList.cuh"

// Двухфазный Jacobi-XSPH.
void launchApplyXSPH(DeviceParticles2D& particles,
                     const DeviceNeighborList& neighbors,
                     float xsphViscosity,
                     float smoothingRadius);