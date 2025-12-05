#include "Player.h"
#include "Map.h"

#include <gl/freeglut.h>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

Player::Player()
{
    camPos = glm::vec3(-8.f, 0.0f, 53.0f);
    camFront = glm::vec3(0.0f, 0.0f, -1.0f);
    camUp = glm::vec3(0.0f, 1.0f, 0.0f);

    yaw = -90.0f;
    pitch = 0.0f;

    screenWidth = 800.0f;
    screenHeight = 600.0f;

    lastX = screenWidth * 0.5f;
    lastY = screenHeight * 0.5f;
    firstMouse = true;

    moveSpeed = 60.0f;
    mouseSensitivity = 0.1f;
    ignoreMouse = false;

    eyeHeight = 1.f;
    playerRadius = 0.5f;

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

glm::mat4 Player::UpdateMoveAndGetViewMatrix(float dt, const Map& map)
{
    // 이동 방향 계산 (수평 방향만)
    glm::vec3 forwardXZ = glm::vec3(camFront.x, 0.0f, camFront.z);
    if (glm::length(forwardXZ) > 0.0001f)
    {
        forwardXZ = glm::normalize(forwardXZ);
    }

    glm::vec3 rightXZ = glm::normalize(glm::cross(forwardXZ, glm::vec3(0.0f, 1.0f, 0.0f)));

    glm::vec3 moveDir(0.0f);

    if (keyState['w'])
    {
        moveDir += forwardXZ;
    }
    if (keyState['s'])
    {
        moveDir -= forwardXZ;
    }
    if (keyState['d'])
    {
        moveDir += rightXZ;
    }
    if (keyState['a'])
    {
        moveDir -= rightXZ;
    }

    if (glm::length(moveDir) > 0.0001f)
    {
        moveDir = glm::normalize(moveDir);
    }

    glm::vec3 newPos = camPos;

    // XZ평면 이동, 충돌 체크
    if (glm::length(moveDir) > 0.0f)
    {
        float speed = moveSpeed * dt;

        // X 방향 먼저
        glm::vec3 cand = newPos;
        cand.x += moveDir.x * speed;
        if (!CheckCollisionXZ(cand, map))
        {
            newPos.x = cand.x;
        }

        // Z 방향 다음
        cand = newPos;
        cand.z += moveDir.z * speed;
        if (!CheckCollisionXZ(cand, map))
        {
            newPos.z = cand.z;
        }
    }

    // 일정 y값 유지
    newPos.y = eyeHeight;
    camPos = newPos;

    return glm::lookAt(camPos, camPos + camFront, camUp);
}

bool Player::CheckCollisionXZ(const glm::vec3& testPos, const Map& map) const
{
    const std::vector<Box>& boxes = map.GetBoxes();

    for (const Box& b : boxes)
    {
        glm::vec3 half = b.size * 0.5f;
        glm::vec3 minB = b.pos - half;
        glm::vec3 maxB = b.pos + half;

        // y값 너무 멀면 충돌 무시 (천장)
        float y = testPos.y;
        if (y < minB.y || y > maxB.y)
        {
            continue;
        }

        // AABB로 충돌처리
        float pxMin = testPos.x - playerRadius;
        float pxMax = testPos.x + playerRadius;
        float pzMin = testPos.z - playerRadius;
        float pzMax = testPos.z + playerRadius;

        if (pxMax <= minB.x || pxMin >= maxB.x) continue;
        if (pzMax <= minB.z || pzMin >= maxB.z) continue;

        // 여기까지 왔으면 XZ AABB가 겹침 → 충돌
        return true;
    }

    return false;
}