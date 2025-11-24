#include "Player.h"

#include <gl/freeglut.h>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <cmath>

Player::Player()
{
    camPos = glm::vec3(2.5f, 0.0f, 25.0f);
    camFront = glm::vec3(0.0f, 0.0f, -1.0f);
    camUp = glm::vec3(0.0f, 1.0f, 0.0f);

    yaw = -90.0f;
    pitch = 0.0f;

    screenWidth = 800.0f;
    screenHeight = 600.0f;

    lastX = screenWidth * 0.5f;
    lastY = screenHeight * 0.5f;
    firstMouse = true;

    moveSpeed = 6.0f;
    mouseSensitivity = 0.1f;
    ignoreMouse = false;

    for (int i = 0; i < 256; ++i)
    {
        keyState[i] = false;
    }
}

void Player::OnResize(int w, int h)
{
    screenWidth = static_cast<float>(w);
    screenHeight = static_cast<float>(h);

    lastX = screenWidth * 0.5f;
    lastY = screenHeight * 0.5f;
    firstMouse = true;
}

void Player::OnKeyDown(unsigned char key)
{
    if (key < 256)
    {
        keyState[(int)key] = true;
    }
}

void Player::OnKeyUp(unsigned char key)
{
    if (key < 256)
    {
        keyState[(int)key] = false;
    }
}

void Player::OnMouseMotion(int x, int y)
{
    float centerX = screenWidth * 0.5f;
    float centerY = screenHeight * 0.5f;

    if (ignoreMouse)
    {
        ignoreMouse = false;
        lastX = static_cast<float>(x);
        lastY = static_cast<float>(y);
        return;
    }

    if (firstMouse)
    {
        lastX = static_cast<float>(x);
        lastY = static_cast<float>(y);
        firstMouse = false;
    }

    float offsetX = static_cast<float>(x) - lastX;
    float offsetY = lastY - static_cast<float>(y);

    lastX = static_cast<float>(x);
    lastY = static_cast<float>(y);

    offsetX *= mouseSensitivity;
    offsetY *= mouseSensitivity;

    yaw += offsetX;
    pitch += offsetY;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 dir;
    dir.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    dir.y = std::sin(glm::radians(pitch));
    dir.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

    camFront = glm::normalize(dir);

    ignoreMouse = true;
    glutWarpPointer(static_cast<int>(centerX), static_cast<int>(centerY));
}

glm::mat4 Player::UpdateMoveAndGetViewMatrix(float dt)
{
    float speed = moveSpeed * dt;
    glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));

    if (keyState['w'] || keyState['W'])
    {
        camPos += camFront * speed;
    }
    if (keyState['s'] || keyState['S'])
    {
        camPos -= camFront * speed;
    }
    if (keyState['a'] || keyState['A'])
    {
        camPos -= right * speed;
    }
    if (keyState['d'] || keyState['D'])
    {
        camPos += right * speed;
    }

    return glm::lookAt(camPos, camPos + camFront, camUp);
}

glm::vec3 Player::GetPosition() const
{
    return camPos;
}

glm::vec3 Player::GetFront() const
{
    return camFront;
}
