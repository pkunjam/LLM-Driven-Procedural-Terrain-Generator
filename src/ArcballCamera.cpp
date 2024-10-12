#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/gl.h>
#include <glad/glad.h>

class ArcballCamera
{
public:
    glm::vec3 Target; // The point around which the camera orbits
    float Distance;   // Distance from the target
    float Yaw;        // Horizontal angle
    float Pitch;      // Vertical angle
    float ZoomSpeed;
    float PanSpeed;
    float RotationSpeed;

    ArcballCamera(glm::vec3 target, float distance, float yaw, float pitch)
        : Target(target), Distance(distance), Yaw(yaw), Pitch(pitch), ZoomSpeed(1.0f), PanSpeed(0.005f), RotationSpeed(0.1f) {}

    glm::mat4 GetViewMatrix()
    {
        glm::vec3 direction;
        direction.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        direction.y = sin(glm::radians(Pitch));
        direction.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        glm::vec3 position = Target - direction * Distance;
        return glm::lookAt(position, Target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void ProcessMouseMovement(float xOffset, float yOffset, bool rotate)
    {
        if (rotate)
        {
            Yaw += xOffset * RotationSpeed;
            Pitch += yOffset * RotationSpeed;
            Pitch = glm::clamp(Pitch, -89.0f, 89.0f); // Avoid flipping the camera
        }
    }

    void ProcessMouseScroll(float yOffset)
    {
        Distance -= yOffset * ZoomSpeed;
        Distance = glm::clamp(Distance, 1.0f, 50.0f); // Set min and max zoom levels
    }

    void ProcessMousePan(float xOffset, float yOffset)
    {
        // Get the right vector (camera's local x-axis direction) based on the current yaw and pitch
        glm::vec3 right = glm::normalize(glm::cross(GetCameraFront(), glm::vec3(0.0f, 1.0f, 0.0f)));

        // The up vector is always the global up vector (0.0, 1.0, 0.0)
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        // Adjust the camera's target based on the pan direction
        Target += -right * xOffset * PanSpeed; // Move left/right based on right vector
        Target += up * yOffset * PanSpeed;     // Move up/down based on up vector
    }

    // Helper function to get the camera's forward direction (normalized front vector)
    glm::vec3 GetCameraFront() const
    {
        glm::vec3 direction;
        direction.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        direction.y = sin(glm::radians(Pitch));
        direction.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        return glm::normalize(direction);
    }

    glm::vec3 GetCameraPosition() const
    {
        glm::vec3 direction;
        direction.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        direction.y = sin(glm::radians(Pitch));
        direction.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        return Target - direction * Distance;
    }
};