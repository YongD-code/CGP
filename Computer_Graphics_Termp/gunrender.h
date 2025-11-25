#pragma once
#pragma once
#include <gl/glew.h>
#include <vector>
#include <gl/glm/glm.hpp>

class GunRenderer {
public:
    bool Load(const char* path);
    void Draw(GLuint shaderProgram,
        GLint uModelLoc,
        GLint uViewLoc,
        GLint uProjLoc,
        GLint uColorLoc,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& camPos,
        const glm::vec3& camFront,
        const glm::vec3& camUp);

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    size_t indexCount = 0;
};
