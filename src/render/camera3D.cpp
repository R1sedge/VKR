#include "Camera3D.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera3D::Camera3D()
    : m_eye(0.0f, 0.0f, 15.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_dist(15.0f)
    , m_yaw(0.0f)
    , m_pitch(25.0f)
    , m_fovY(60.0f)
    , m_aspect(1.5f)
    , m_near(0.05f)
    , m_far(500.0f)
{
    recalcEye();
}


void Camera3D::setAspect(float aspect) { m_aspect = aspect; }

void Camera3D::setDist(float dist) { m_dist = std::max(dist, 0.5f); recalcEye(); }

void Camera3D::orbit(float dYaw, float dPitch)
{
    m_yaw += dYaw;
    m_pitch += dPitch;
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    recalcEye();
}

void Camera3D::zoom(float dDist)
{
    m_dist -= dDist;
    m_dist = std::max(m_dist, 0.5f);
    recalcEye();
}

void Camera3D::pan(float dxPixels, float dyPixels)
{
    // Масштаб: чем дальше камера, тем больше смещение на пиксель
    float scale = m_dist * 0.0015f;

    glm::vec3 right = getRight();
    glm::vec3 up = getCamUp();

    m_target -= right * (dxPixels * scale);
    m_target -= up * (dyPixels * scale);
    recalcEye();
}

void Camera3D::reset()
{
    m_target = glm::vec3(0.0f);
    m_dist = 15.0f;
    m_yaw = 0.0f;
    m_pitch = 25.0f;
    recalcEye();
}

glm::mat4 Camera3D::getViewMatrix() const
{
    return glm::lookAt(m_eye, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera3D::getProjMatrix() const
{
    return glm::perspective(glm::radians(m_fovY), m_aspect, m_near, m_far);
}

glm::vec3 Camera3D::getRight() const
{
    glm::vec3 dir = glm::normalize(m_target - m_eye);
    return glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 Camera3D::getCamUp() const
{
    glm::vec3 right = getRight();
    glm::vec3 dir = glm::normalize(m_target - m_eye);
    return glm::normalize(glm::cross(right, dir));
}

void Camera3D::recalcEye()
{
    float yawR = glm::radians(m_yaw);
    float pitchR = glm::radians(m_pitch);

    float x = m_dist * std::cos(pitchR) * std::sin(yawR);
    float y = m_dist * std::sin(pitchR);
    float z = m_dist * std::cos(pitchR) * std::cos(yawR);

    m_eye = m_target + glm::vec3(x, y, z);
}