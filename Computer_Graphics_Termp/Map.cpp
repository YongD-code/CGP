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
    float roomHalfSize = 50.f;
    float roomSize = 100.f;

    // 바닥
    Box floor;
    floor.pos = glm::vec3(0.0f, -wallThickness, 0.0f);
    floor.size = glm::vec3(roomSize, wallThickness * 2.f, roomSize);
    floor.color = glm::vec3(0.f, 0.f, 0.f);
    boxes.push_back(floor);

    // 천장
    Box ceiling;
    ceiling.pos = glm::vec3(0.0f, wallHeight - wallThickness, 0.0f);
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

void Map::InitFromArray(int w, int h, const int* data)
{
    boxes.clear();

    float cellSize = 4.0f;
    float wallHeight = 4.0f;

    // 마지막 행은 천장 유무 검사
    int entranceRow = h - 1;
    std::vector<bool> lastRowCeil(w, false);

    for (int x = 0; x <= w - 3; ++x)
    {
        int i0 = entranceRow * w + x;
        int i1 = entranceRow * w + (x + 1);
        int i2 = entranceRow * w + (x + 2);

        int v0 = data[i0];
        int v1 = data[i1];
        int v2 = data[i2];

        if (v0 == 1 && v1 == 0 && v2 == 1)
        {
            // 1 0 1 이 연속으로 나오면 입구라는 뜻, 천장 존재
            lastRowCeil[x] = true;
            lastRowCeil[x + 1] = true;
            lastRowCeil[x + 2] = true;
        }
    }

    for (int z = 0; z < h; ++z)
    {
        for (int x = 0; x < w; ++x)
        {
            int idx = z * w + x;
            int v = data[idx];

            float fx = (x - w / 2.0f) * cellSize + cellSize * 0.5f;
            float fz = (z - h / 2.0f) * cellSize + cellSize * 0.5f;

            if (v == 1)
            {
                Box wall;
                wall.size = glm::vec3(cellSize, wallHeight, cellSize);
                wall.pos = glm::vec3(fx, wallHeight * 0.5f - 0.5f, fz);
                wall.color = glm::vec3(0.1f, 0.1f, 0.1f);
                boxes.push_back(wall);
            }
            // 천장 만들지 말지
            bool makeCeil = false;
            if (z < h - 1) makeCeil = true; // 애초에 마지막 행이 아니면 항상 천장 존재
            else makeCeil = lastRowCeil[x]; // 1 0 1 인지 검사, 아니면 초기값인 false 리턴

            if (makeCeil)
            {
                Box ceiling;
                ceiling.size = glm::vec3(cellSize, wallHeight, cellSize);
                ceiling.pos = glm::vec3(fx, wallHeight * 1.5f - 0.5f, fz);
                ceiling.color = glm::vec3(0.1f, 0.1f, 0.1f);
                boxes.push_back(ceiling);
            }
        }
    }
}
