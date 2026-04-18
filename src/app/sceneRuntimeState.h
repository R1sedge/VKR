#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

struct SceneRuntimeState
{
    glm::quat vesselOrientation = glm::quat(1.f, 0.f, 0.f, 0.f);

    glm::vec3 gravityWorld = {0.0f, -9.81f, 0.0f};
};