#include "simCUDA/constraints/cudaKernelsBounds.cuh"

#include <cuda_runtime.h>

#include "simCUDA/utils/cudaCheck.h"
#include "simCUDA/utils/cudaUtils.cuh"
#include "simCUDA/utils/cudaInternalBoundaryUtils.cuh"

namespace
{
    __global__ void applyBoundaryVelocityResponseKernel(
    int n,
    float* __restrict__ vx,
    float* __restrict__ vy,
    float* __restrict__ vz,
    const float* __restrict__ x,
    const float* __restrict__ y,
    const float* __restrict__ z,
    const DeviceBoundaryPlane* __restrict__ planes,
    int planeCount,
    float radius,
    float restitution,
    float friction,
    float angVx, float angVy, float angVz,   // угловая скорость сосуда (world space)
    float pivotX, float pivotY, float pivotZ) // точка вращения
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float vi_x = vx[i], vi_y = vy[i], vi_z = vz[i];
        const float xi = x[i], yi = y[i], zi = z[i];

        for (int planeIdx = 0; planeIdx < planeCount; ++planeIdx)
        {
            const DeviceBoundaryPlane plane = planes[planeIdx];
            const float dist = plane.nx * (xi - plane.px)
                            + plane.ny * (yi - plane.py)
                            + plane.nz * (zi - plane.pz);

            // Применяем только к частицам у стенки (не дальше 1.5*radius)
            if (dist > 1.5f * radius) continue;

            // Скорость стенки в точке контакта: v_wall = ω × (pos − pivot)
            const float rpx = xi - pivotX;
            const float rpy = yi - pivotY;
            const float rpz = zi - pivotZ;
            const float wallVx = angVy * rpz - angVz * rpy;
            const float wallVy = angVz * rpx - angVx * rpz;
            const float wallVz = angVx * rpy - angVy * rpx;

            // Относительная скорость частицы относительно стенки
            const float vRelX = vi_x - wallVx;
            const float vRelY = vi_y - wallVy;
            const float vRelZ = vi_z - wallVz;

            const float vn = vRelX * plane.nx + vRelY * plane.ny + vRelZ * plane.nz;

            // Response только если летим в стенку (vn < 0)
            if (vn >= 0.0f) continue;

            // Нормальная и касательная компоненты относительной скорости
            const float vnX = vn * plane.nx;
            const float vnY = vn * plane.ny;
            const float vnZ = vn * plane.nz;
            const float vtX = vRelX - vnX;
            const float vtY = vRelY - vnY;
            const float vtZ = vRelZ - vnZ;

            // v_rel_new = -restitution * vN + (1 - friction) * vT
            const float newRelX = -restitution * vnX + (1.0f - friction) * vtX;
            const float newRelY = -restitution * vnY + (1.0f - friction) * vtY;
            const float newRelZ = -restitution * vnZ + (1.0f - friction) * vtZ;

            // Переводим обратно в мировое пространство
            vi_x = newRelX + wallVx;
            vi_y = newRelY + wallVy;
            vi_z = newRelZ + wallVz;
        }

        vx[i] = vi_x;
        vy[i] = vi_y;
        vz[i] = vi_z;
    }

    __global__ void projectBoundsKernel(
        int n,
        float* x,
        float* y,
        float* z,
        float left,
        float right,
        float bottom,
        float top,
        float front,
        float back,
        float radius)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        if (x[i] < left + radius)   x[i] = left + radius;
        if (x[i] > right - radius)  x[i] = right - radius;
        if (y[i] < bottom + radius) y[i] = bottom + radius;
        if (y[i] > top - radius)    y[i] = top - radius;
        if (z[i] < front + radius)  z[i] = front + radius;
        if (z[i] > back - radius)   z[i] = back - radius;
    }

    __global__ void projectToVesselPlanesKernel(
        int n,
        float* __restrict__ x,
        float* __restrict__ y,
        float* __restrict__ z,
        const DeviceBoundaryPlane* __restrict__ planes,
        int planeCount,
        float radius)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float px = x[i];
        float py = y[i];
        float pz = z[i];

        // Для inward-facing planes считаем particle center валидным,
        // если signedDist >= radius. Иначе двигаем центр по нормали внутрь.
        for (int pass = 0; pass < 2; ++pass)
        {
            bool any = false;
            for (int planeIdx = 0; planeIdx < planeCount; ++planeIdx)
            {
                const DeviceBoundaryPlane plane = planes[planeIdx];

                const float dist = plane.nx * (px - plane.px) + plane.ny * (py - plane.py) + plane.nz * (pz - plane.pz);

                if (dist < radius)
                {
                    const float correction = radius - dist;
                    px += correction * plane.nx;
                    py += correction * plane.ny;
                    pz += correction * plane.nz;
                    any = true;
                }
            }
            if (!any) break;
        }

        x[i] = px;
        y[i] = py;
        z[i] = pz;
    }

    __global__ void projectToInternalPatchesKernel(
    int     n,
    float* __restrict__ x,
    float* __restrict__ y,
    float* __restrict__ z,
    const float* __restrict__ px,  // позиция на начало шага
    const float* __restrict__ py,
    const float* __restrict__ pz,
    float   particleRadius)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float posX = x[i], posY = y[i], posZ = z[i];
        const float prevX = px[i], prevY = py[i], prevZ = pz[i];

        for (int k = 0; k < c_internalPatchCount; ++k)
        {
            const CudaInternalBoundaryPatch& p = c_internalPatches[k];

            // Вектор pos - point
            const float relX = posX - p.pointX;
            const float relY = posY - p.pointY;
            const float relZ = posZ - p.pointZ;

            // Расстояние по нормали
            const float side = relX * p.normalX + relY * p.normalY + relZ * p.normalZ;

            // Быстрый отсев: вне вертикальной зоны
            if (fabsf(side) >= p.thickness * 0.5f + particleRadius) continue;

            // Локальные координаты в плоскости
            const float localU = relX * p.uX + relY * p.uY + relZ * p.uZ;
            const float localV = relX * p.vX + relY * p.vY + relZ * p.vZ;

            // Вне прямоугольника перегородки
            if (fabsf(localU) > p.halfWidth)  continue;
            if (fabsf(localV) > p.halfHeight) continue;

            // Попала в отверстие — пропускаем
            if (insideAperture(localU, localV, p, particleRadius)) continue;

            // Сторона по prevPos — откуда пришла частица
            const float prevSide =
                (prevX - p.pointX) * p.normalX +
                (prevY - p.pointY) * p.normalY +
                (prevZ - p.pointZ) * p.normalZ;
            const float sign = (prevSide >= 0.0f) ? 1.0f : -1.0f;

            // Выталкиваем на нужную сторону
            const float correction = p.thickness * 0.5f + particleRadius - sign * side;
            posX += correction * p.normalX * sign;
            posY += correction * p.normalY * sign;
            posZ += correction * p.normalZ * sign;
        }

        x[i] = posX;
        y[i] = posY;
        z[i] = posZ;
    }

    __global__ void applyInternalBaffleVelocityResponseKernel(
    int n,
    float* __restrict__ vx,
    float* __restrict__ vy,
    float* __restrict__ vz,
    const float* __restrict__ x,
    const float* __restrict__ y,
    const float* __restrict__ z,
    float particleRadius,
    float restitution,
    float friction,
    float angVx, float angVy, float angVz,
    float pivotX, float pivotY, float pivotZ)
    {
        const int i = blockIdx.x * blockDim.x + threadIdx.x;
        if (i >= n) return;

        float vix = vx[i], viy = vy[i], viz = vz[i];
        const float xi = x[i], yi = y[i], zi = z[i];

        for (int k = 0; k < c_internalPatchCount; ++k)
        {
            const CudaInternalBoundaryPatch& p = c_internalPatches[k];

            // Расстояние до плоскости перегородки
            const float relX = xi - p.pointX;
            const float relY = yi - p.pointY;
            const float relZ = zi - p.pointZ;
            const float dist = relX * p.normalX + relY * p.normalY + relZ * p.normalZ;

            // Реагируем только в зоне вблизи поверхности
            if (fabsf(dist) > 1.5f * (p.thickness * 0.5f + particleRadius)) continue;

            // Проверяем локальные координаты — в пределах прямоугольника
            const float localU = relX * p.uX + relY * p.uY + relZ * p.uZ;
            const float localV = relX * p.vX + relY * p.vY + relZ * p.vZ;
            if (fabsf(localU) > p.halfWidth)  continue;
            if (fabsf(localV) > p.halfHeight) continue;

            // Частица в отверстии — пропускаем
            if (insideAperture(localU, localV, p, particleRadius)) continue;

            // Скорость поверхности перегородки: v_wall = omega × (pos - pivot)
            // Идентично applyBoundaryVelocityResponseKernel
            const float rpx = xi - pivotX;
            const float rpy = yi - pivotY;
            const float rpz = zi - pivotZ;
            const float wallVx = angVy * rpz - angVz * rpy;
            const float wallVy = angVz * rpx - angVx * rpz;
            const float wallVz = angVx * rpy - angVy * rpx;

            // Относительная скорость частицы к поверхности
            const float vRelX = vix - wallVx;
            const float vRelY = viy - wallVy;
            const float vRelZ = viz - wallVz;

            // Проекция относительной скорости на нормаль
            const float vn = vRelX * p.normalX + vRelY * p.normalY + vRelZ * p.normalZ;

            // Реагируем только если частица движется НАВСТРЕЧУ стенке
            // dist > 0 → частица с положительной стороны → нормаль "на неё" → vn < 0
            if (vn >= 0.0f) continue;

            // Нормальная и касательная компоненты
            const float vnX = vn * p.normalX;
            const float vnY = vn * p.normalY;
            const float vnZ = vn * p.normalZ;
            const float vtX = vRelX - vnX;
            const float vtY = vRelY - vnY;
            const float vtZ = vRelZ - vnZ;
            
            vix = -restitution * vnX + (1.0f - friction) * vtX + wallVx;
            viy = -restitution * vnY + (1.0f - friction) * vtY + wallVy;
            viz = -restitution * vnZ + (1.0f - friction) * vtZ + wallVz;
        }

        vx[i] = vix;
        vy[i] = viy;
        vz[i] = viz;
    }

}

void launchProjectBounds(DeviceParticles3D& dp,
                         float left,
                         float right,
                         float bottom,
                         float top,
                         float front,
                         float back,
                         float radius)
{
    if (dp.count <= 0)
        return;

    projectBoundsKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x,
        dp.y,
        dp.z,
        left,
        right,
        bottom,
        top,
        front,
        back,
        radius);

    CUDA_CHECK(cudaGetLastError());
}

void launchProjectToVesselPlanes(DeviceParticles3D& dp,
                                 const DeviceBoundaryPlane* planes,
                                 int planeCount,
                                 float radius)
{
    if (dp.count <= 0 || planes == nullptr || planeCount <= 0)
        return;

    projectToVesselPlanesKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x,
        dp.y,
        dp.z,
        planes,
        planeCount,
        radius);

    CUDA_CHECK(cudaGetLastError());
}

void launchApplyBoundaryVelocityResponse(
    DeviceParticles3D& dp,
    const DeviceBoundaryPlane* planes,
    int planeCount,
    float radius,
    float restitution,
    float friction,
    float angVx, float angVy, float angVz,
    float pivotX, float pivotY, float pivotZ)
{
    if (dp.count <= 0 || planes == nullptr || planeCount <= 0) return;

    applyBoundaryVelocityResponseKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.vx, dp.vy, dp.vz,
        dp.x,  dp.y,  dp.z,
        planes, planeCount, radius,
        restitution, friction,
        angVx, angVy, angVz,
        pivotX, pivotY, pivotZ);

    CUDA_CHECK(cudaGetLastError());
}

void launchProjectToInternalPatches(DeviceParticles3D dp, float particleRadius)
{
    if (dp.count == 0) return;

    // Ранний выход без запуска ядра
    int hostCount = 0;
    CUDA_CHECK(cudaMemcpyFromSymbol(&hostCount, c_internalPatchCount, sizeof(int)));
    if (hostCount == 0) return;

    projectToInternalPatchesKernel<<<CudaUtils::gridSize(dp.count), CudaUtils::BLOCK_SIZE>>>(
        dp.count,
        dp.x, dp.y, dp.z,
        dp.px, dp.py, dp.pz,
        particleRadius
    );
    
    CUDA_CHECK(cudaGetLastError());
}

void launchApplyInternalBaffleVelocityResponse(
    DeviceParticles3D dp,
    float particleRadius,
    float restitution,
    float friction,
    float angVx, float angVy, float angVz,
    float pivotX, float pivotY, float pivotZ)
{
    if (dp.count == 0) return;

    int hostCount = 0;
    CUDA_CHECK(cudaMemcpyFromSymbol(&hostCount, c_internalPatchCount, sizeof(int)));
    if (hostCount == 0) return;

    applyInternalBaffleVelocityResponseKernel<<<
        CudaUtils::gridSize(dp.count),
        CudaUtils::BLOCK_SIZE
    >>>(
        dp.count,
        dp.vx, dp.vy, dp.vz,
        dp.x,  dp.y,  dp.z,
        particleRadius,
        restitution,
        friction,
        angVx, angVy, angVz,
        pivotX, pivotY, pivotZ
    );
    CUDA_CHECK(cudaGetLastError());
}
