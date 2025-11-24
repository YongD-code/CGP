#include "Map.h"

#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>

void Map::InitTestRoom()
{
    boxes.clear();

    // 벽 공통 설정
    float wallHeight = 3.0f;
    float wallThickness = 0.5f;
    float roomHalfSize = 5.0f;
    float roomSize = 10.f;

    // 바닥
    Box floor;
    floor.pos = glm::vec3(0.0f, -wallThickness, 0.0f);
    floor.size = glm::vec3(roomSize, wallThickness * 2.f, roomSize);
    floor.color = glm::vec3(0.f, 0.f, 0.f);
    boxes.push_back(floor);

    // 천장
    Box ceiling;
    ceiling.pos = glm::vec3(0.0f, wallHeight, 0.0f);
    ceiling.size = glm::vec3(roomSize, wallThickness * 2.f, roomSize);
    ceiling.color = glm::vec3(0.f, 0.f, 0.f);
    boxes.push_back(ceiling);

    // 앞쪽 벽 (z = -roomHalfSize)
    Box wallFront;
    wallFront.pos = glm::vec3(0.0f, wallHeight * 0.5f - wallThickness, -roomHalfSize);
    wallFront.size = glm::vec3(roomSize, wallHeight, wallThickness);
    wallFront.color = glm::vec3(0.f, 0.f, 0.f);
    boxes.push_back(wallFront);

    // 뒤쪽 벽 (z = +roomHalfSize)
    Box wallBack = wallFront;
    wallBack.pos.z = roomHalfSize;
    boxes.push_back(wallBack);

    // 왼쪽 벽 (x = -roomHalfSize)
    Box wallLeft;
    wallLeft.pos = glm::vec3(-roomHalfSize, wallHeight * 0.5f - wallThickness, 0.0f);
    wallLeft.size = glm::vec3(wallThickness, wallHeight, roomSize);
    wallLeft.color = glm::vec3(0.f, 0.f, 0.f);
    boxes.push_back(wallLeft);

    // 오른쪽 벽 (x = +roomHalfSize)
    Box wallRight = wallLeft;
    wallRight.pos.x = roomHalfSize;
    boxes.push_back(wallRight);
}

void Map::Draw(
    GLuint shaderProgram,
    GLuint vaoCube,
    GLint uModelLoc,
    GLint uViewLoc,
    GLint uProjLoc,
    GLint uColorLoc,
    const glm::mat4& view,
    const glm::mat4& proj
) const
{
    glUseProgram(shaderProgram);
    glBindVertexArray(vaoCube);

    for (const Box& b : boxes)
    {
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, b.pos);
        model = glm::scale(model, b.size);

        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(uColorLoc, 1, glm::value_ptr(b.color));

        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}
