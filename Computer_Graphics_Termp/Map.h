#pragma once

#include <vector>
#include <gl/glm/glm.hpp>
#include <gl/glew.h>

// 하나의 직육면체
struct Box
{
    glm::vec3 pos;   // 중심 위치
    glm::vec3 size;  // 스케일 (x, y, z)
    glm::vec3 color; // 색상 (0~1)
};

class Map
{
public:
    // 테스트용 방
    void InitTestRoom();

    // 맵 그리기
    void Draw(
        GLuint shaderProgram,
        GLuint vaoCube,
        GLint uModelLoc,
        GLint uViewLoc,
        GLint uProjLoc,
        GLint uColorLoc,
        const glm::mat4& view,
        const glm::mat4& proj
    ) const;

private:
    std::vector<Box> boxes;
};
