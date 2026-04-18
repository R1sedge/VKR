#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

struct VesselWireframe
{
    std::vector<glm::vec3> bodyVertices;   // вершины в body-space
    std::vector<uint32_t> lineIndices;     // пары индексов для GL_LINES

    bool empty() const
    {
        return bodyVertices.empty() || lineIndices.empty();
    }
};