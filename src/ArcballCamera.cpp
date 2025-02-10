// ArcballCamera.cpp
#include "ArcballCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

ArcballCamera::ArcballCamera(const glm::vec3& target, float distance, float yaw, float pitch)
    : Target(target), m_distance(distance), m_yaw(yaw), m_pitch(pitch)
{
    updateCameraPosition();
}

glm::mat4 ArcballCamera::GetViewMatrix() const
{
    // Typically use glm::lookAt with camera position, target, and up vector.
    return glm::lookAt(m_position, Target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::vec3 ArcballCamera::GetCameraPosition() const
{
    return m_position;
}

glm::vec3 ArcballCamera::GetCameraFront() const
{
    // Compute and return the forward vector (from position to target)
    return glm::normalize(Target - m_position);
}

void ArcballCamera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    // Example implementation. Adjust sensitivity as needed.
    const float sensitivity = 0.1f;
    m_yaw   += xOffset * sensitivity;
    m_pitch += yOffset * sensitivity;

    if (constrainPitch)
    {
        if (m_pitch > 89.0f)
            m_pitch = 89.0f;
        if (m_pitch < -89.0f)
            m_pitch = -89.0f;
    }

    updateCameraPosition();
}

void ArcballCamera::ProcessMousePan(float xOffset, float yOffset)
{
    // Example implementation for panning. Adjust pan speed as needed.
    const float panSpeed = 0.005f;
    Target += glm::vec3(-xOffset * panSpeed, yOffset * panSpeed, 0.0f);
    updateCameraPosition();
}

void ArcballCamera::ProcessMouseScroll(float yOffset)
{
    // Example implementation for zooming.
    m_distance -= yOffset * 0.1f;
    if (m_distance < 0.1f)
        m_distance = 0.1f;
    updateCameraPosition();
}

void ArcballCamera::updateCameraPosition()
{
    // Convert yaw and pitch from degrees to radians.
    float yawRad   = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    // Spherical coordinates to Cartesian conversion.
    m_position.x = Target.x + m_distance * cos(pitchRad) * cos(yawRad);
    m_position.y = Target.y + m_distance * sin(pitchRad);
    m_position.z = Target.z + m_distance * cos(pitchRad) * sin(yawRad);
}
