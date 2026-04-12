#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera3D
{
public:
    Camera3D();

    // Каждый кадр: App передаёт текущий aspect window
    void setAspect(float aspect);
    void setDist(float dist);

    // Управление
    void orbit(float dYaw, float dPitch); // градусы
    void zoom(float dDist); // >0 = приблизить
    void pan(float dxPixels, float dyPixels);
    void reset();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjMatrix() const;

    // Для billboard-рендеринга
    glm::vec3 getRight() const;
    glm::vec3 getCamUp() const;
    glm::vec3 getEye() const { return m_eye; }

private:
    void recalcEye();

    glm::vec3 m_eye;
    glm::vec3 m_target;

    float m_dist;
    float m_yaw; // градусы, горизонталь
    float m_pitch; // градусы, вертикаль [-89..89]

    float m_fovY;
    float m_aspect;
    float m_near;
    float m_far;
};