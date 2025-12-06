#pragma once
#include <gl/glm/glm.hpp>

class Map;

class Player
{
public:
    Player();

    // 창 크기 변경 시 호출 (마우스 중앙 기준 업데이트)
    void OnResize(int w, int h);

    // 키 입력 처리
    void OnKeyDown(unsigned char key);
    void OnKeyUp(unsigned char key);

    // 마우스 이동 처리
    void OnMouseMotion(int x, int y);

    // 매 프레임 호출: 위치 업데이트 + view 행렬 반환 + 충돌처리
    glm::mat4 UpdateMoveAndGetViewMatrix(float dt, const Map& map);

    // 필요하면 위치/방향 얻기
    glm::vec3 GetPosition() const
    {
        return camPos;
    }

    glm::vec3 GetFront() const
    {
        return camFront;
    }
public:
    glm::vec3 camPos;
    glm::vec3 camFront;
    glm::vec3 camUp;

private:
    float yaw;
    float pitch;

    float lastX;
    float lastY;
    bool  firstMouse;

    float moveSpeed;
    float mouseSensitivity;
    bool  ignoreMouse;

    bool  keyState[256];

    float screenWidth;
    float screenHeight;

    float eyeHeight;    // 바닥에서 눈까지 높이
    float playerRadius; // 벽과의 거리

    float footstepTimer;
    bool  wasMoving;
    bool  nextLeftStep;

    bool CheckCollisionXZ(const glm::vec3& testPos, const Map& map) const;
};
