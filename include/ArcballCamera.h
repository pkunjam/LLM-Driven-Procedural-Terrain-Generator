// ArcballCamera.h
#ifndef ARCBALLCAMERA_H
#define ARCBALLCAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class ArcballCamera
{
public:
    /**
     * @brief Constructs an ArcballCamera.
     * @param target The point the camera looks at.
     * @param distance The initial distance from the target.
     * @param yaw The initial yaw angle (in degrees).
     * @param pitch The initial pitch angle (in degrees).
     */
    ArcballCamera(const glm::vec3& target, float distance, float yaw, float pitch);

    /**
     * @brief Returns the view matrix computed from the camera's position and target.
     */
    glm::mat4 GetViewMatrix() const;

    /**
     * @brief Returns the camera's position in world space.
     */
    glm::vec3 GetCameraPosition() const;

    /**
     * @brief Returns the normalized forward (look) vector of the camera.
     */
    glm::vec3 GetCameraFront() const;

    /**
     * @brief Processes mouse movement to orbit the camera around the target.
     * @param xOffset The change in the mouse's x position.
     * @param yOffset The change in the mouse's y position.
     * @param constrainPitch If true, restricts the pitch angle to avoid gimbal lock.
     */
    void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

    /**
     * @brief Processes mouse panning to shift the camera target.
     * @param xOffset The change in the mouse's x position.
     * @param yOffset The change in the mouse's y position.
     */
    void ProcessMousePan(float xOffset, float yOffset);

    /**
     * @brief Processes mouse scroll input to zoom the camera in or out.
     * @param yOffset The scroll offset (typically from the mouse wheel).
     */
    void ProcessMouseScroll(float yOffset);

    // Public member to allow external code to modify the camera target if needed.
    glm::vec3 Target;

private:
    /**
     * @brief Recalculates the camera's position based on the current parameters.
     */
    void updateCameraPosition();

    // Camera attributes
    float m_distance;  // Distance from the camera to the target.
    float m_yaw;       // Yaw angle in degrees.
    float m_pitch;     // Pitch angle in degrees.
    glm::vec3 m_position; // Cached camera position.
};

#endif // ARCBALLCAMERA_H
