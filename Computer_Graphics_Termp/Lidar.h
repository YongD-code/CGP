#pragma once

#include <vector>
#include <cstddef>

#include <gl/glew.h>
#include <gl/glm/glm.hpp>
#include "Map.h"  

struct ScanState {
    bool active = false;   // 스캔 중인지
    int curRow = 0;        // 현재 처리 중인 vertical index
    int horizontal = 80;   // 좌우 레이 수
    int vertical = 60;     // 위아래 레이 수
    float hFov = glm::radians(30.0f);
    float vFov = glm::radians(30.0f);
    glm::vec3 origin;
    glm::vec3 front;
    glm::vec3 up;
    std::vector<Box> boxes;
};

class Map;

class Lidar
{
public:
    Lidar();
    ~Lidar();
    std::vector<glm::vec3> points;
    // VAO / VBO 초기화
    void Init();

    // 저장된 포인트 모두 삭제
    void Clear();

    // origin 에서 dir 방향으로 레이 1개 쏘고,
    // Map 의 박스들과 가장 가까운 교차점을 저장
    void ScanSingleRay(const glm::vec3& origin,
        const glm::vec3& dir,
        const Map& map);

    void ScanFan(const glm::vec3& origin,
        const glm::vec3& front,
        const Map& map);

    bool Raycast(const glm::vec3& origin,
        const glm::vec3& dir,
        const std::vector<Box>& boxes,
        float maxDist,
        glm::vec3& hitPos);

    void ScanFanWide(const glm::vec3& origin,
        const glm::vec3& forward,
        const glm::vec3& up,
        const std::vector<Box>& boxes);

    void StartScan(const glm::vec3& origin,
        const glm::vec3& front,
        const glm::vec3& up,
        const std::vector<Box>& boxes);

    void UpdateScan();

    // 저장된 포인트들을 GL_POINTS 로 렌더링
    void Draw(GLuint shaderProgram,
        GLint uModelLoc,
        GLint uViewLoc,
        GLint uProjLoc,
        GLint uColorLoc,
        const glm::mat4& view,
        const glm::mat4& proj) const;

private:
    GLuint VAO;
    GLuint VBO;
    ScanState scan;
    std::size_t maxPoints;
};
