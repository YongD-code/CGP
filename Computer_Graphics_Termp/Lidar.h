#pragma once

#include <vector>
#include <cstddef>

#include <gl/glew.h>
#include <gl/glm/glm.hpp>

class Map;

class Lidar
{
public:
    Lidar();
    ~Lidar();

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
    std::vector<glm::vec3> points;
    std::size_t maxPoints;
};
