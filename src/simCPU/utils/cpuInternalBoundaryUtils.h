#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/glm.hpp>

#include "scene/boundary/boundaryPlane.h"

namespace CpuInternalBoundaryUtils
{
    inline bool insideAperture(
        float localU,
        float localV,
        const InternalBoundaryPatch& patch,
        float particleRadius)
    {
        if (patch.apertureType != InternalApertureType::Circle)
            return false;

        const float du = localU - patch.apertureCenter.x;
        const float dv = localV - patch.apertureCenter.y;
        const float effectiveRadius = std::max(0.0f, patch.apertureRadius - particleRadius);

        return du * du + dv * dv <= effectiveRadius * effectiveRadius;
    }

    inline bool segmentBlockedByInternalPatch(
        const glm::vec3& a,
        const glm::vec3& b,
        const InternalBoundaryPatch& patch,
        float particleRadius)
    {
        const float sideA = glm::dot(a - patch.point, patch.normal);
        const float sideB = glm::dot(b - patch.point, patch.normal);

        // Оба по одну сторону — пересечения с плоскостью перегородки нет.
        // Поведение синхронизировано с CUDA signbit(sideA) == signbit(sideB).
        if (std::signbit(sideA) == std::signbit(sideB))
            return false;

        const float denom = sideA - sideB;
        if (std::abs(denom) < 1e-6f)
            return false;

        const float t = sideA / denom;
        const glm::vec3 hit = a + t * (b - a) - patch.point;

        const float localU = glm::dot(hit, patch.u);
        const float localV = glm::dot(hit, patch.v);

        if (std::abs(localU) > patch.halfWidth + particleRadius)
            return false;

        if (std::abs(localV) > patch.halfHeight + particleRadius)
            return false;

        // Сегмент проходит через отверстие — пара не заблокирована.
        if (insideAperture(localU, localV, patch, particleRadius))
            return false;

        return true;
    }

    inline bool segmentBlockedByAnyInternalPatch(
        const glm::vec3& a,
        const glm::vec3& b,
        const std::vector<InternalBoundaryPatch>& patches,
        float particleRadius)
    {
        for (const InternalBoundaryPatch& patch : patches)
        {
            if (segmentBlockedByInternalPatch(a, b, patch, particleRadius))
                return true;
        }

        return false;
    }
}