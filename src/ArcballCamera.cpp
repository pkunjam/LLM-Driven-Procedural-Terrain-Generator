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
        glm::vec3 right = glm::normalize(glm::cross(Target - GetCameraPosition(), glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, Target - GetCameraPosition()));

        Target += -right * xOffset * PanSpeed;
        Target += up * yOffset * PanSpeed;
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