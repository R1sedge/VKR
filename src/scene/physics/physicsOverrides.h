#pragma once
#include <optional>
#include <glm/glm.hpp>

// Параметры, которые можно переопределить в сцене.
struct PhysicsOverrides 
{
    std::optional<glm::vec3> gravity; // → Config::gravityX/Y/Z

    // Записать все заданные поля в глобалы Config::
    void applyToConfig() const;
};