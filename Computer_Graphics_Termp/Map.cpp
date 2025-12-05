#include "Map.h"

#include <stdio.h>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>
#include "TextureManager.h"

static void CreateRevealMask(GLuint& tex)
{
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    const int SIZE = 256;
    std::vector<unsigned char> blank(SIZE * SIZE, 0);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SIZE, SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, blank.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

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
    for (int f = 0; f < 6; f++)
        CreateRevealMask(floor.revealMask[f]);
    boxes.push_back(floor);


    // 천장
    Box ceiling;
    ceiling.pos = glm::vec3(0.0f, wallHeight - wallThickness, 0.0f);
    ceiling.size = glm::vec3(roomSize, wallThickness * 2.f, roomSize);
    ceiling.color = glm::vec3(0.f, 0.f, 0.f);
    for (int f = 0; f < 6; f++)
        CreateRevealMask(ceiling.revealMask[f]);
    boxes.push_back(ceiling);


    // 앞쪽 벽 (z = -roomHalfSize)
    Box wallFront;
    wallFront.pos = glm::vec3(0.0f, wallHeight * 0.5f - wallThickness, -roomHalfSize);
    wallFront.size = glm::vec3(roomSize, wallHeight, wallThickness);
    wallFront.color = glm::vec3(0.f, 0.f, 0.f);
    for (int f = 0; f < 6; f++)
        CreateRevealMask(wallFront.revealMask[f]);
    boxes.push_back(wallFront);


    // 뒤쪽 벽 (z = +roomHalfSize)
    Box wallBack = wallFront;
    wallBack.pos.z = roomHalfSize;
    for (int f = 0; f < 6; f++)
        CreateRevealMask(wallBack.revealMask[f]);
    boxes.push_back(wallBack);


    // 왼쪽 벽 (x = -roomHalfSize)
    Box wallLeft;
    wallLeft.pos = glm::vec3(-roomHalfSize, wallHeight * 0.5f - wallThickness, 0.0f);
    wallLeft.size = glm::vec3(wallThickness, wallHeight, roomSize);
    wallLeft.color = glm::vec3(0.f, 0.f, 0.f);
    for (int f = 0; f < 6; f++)
        CreateRevealMask(wallLeft.revealMask[f]);
    boxes.push_back(wallLeft);


    // 오른쪽 벽 (x = +roomHalfSize)
    Box wallRight = wallLeft;
    wallRight.pos.x = roomHalfSize;
    for (int f = 0; f < 6; f++)
        CreateRevealMask(wallRight.revealMask[f]);
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
    GLint uTexRotLoc = glGetUniformLocation(shaderProgram, "uTexRot");

    glUseProgram(shaderProgram);
    glBindVertexArray(vaoCube);

    for (const Box& b : boxes)
    {

        glm::mat4 model = glm::translate(glm::mat4(1.0f), b.pos);
        model = glm::scale(model, b.size);

        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(uColorLoc, 1, glm::value_ptr(b.color));

        for (int face = 0; face < 6; face++)
        {
            glUniform1i(uTexRotLoc, b.texRot[face]);
            if (b.hasTex[face])
            {
                glUniform1i(glGetUniformLocation(shaderProgram, "uHasTex"), 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, b.texID[face]);
                glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, b.revealMask[face]);
                glUniform1i(glGetUniformLocation(shaderProgram, "uRevealMask"), 1);
            }
            else
            {
                glUniform1i(glGetUniformLocation(shaderProgram, "uHasTex"), 0);
            }

            glDrawElements(
                GL_TRIANGLES,
                6,
                GL_UNSIGNED_INT,
                (void*)(sizeof(unsigned int) * face * 6)
            );
        }
    }

    glBindVertexArray(0);
}

void Map::InitFromArray(int w, int h, const int* data)
{
    boxes.clear();

    float cellSize = 4.0f;
    float wallHeight = 4.0f;

    int entranceRow = h - 4;
    std::vector<bool> entranceCeil(w, false);

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
            entranceCeil[x + 1] = true;
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
                for (int f = 0; f < 6; f++)
                    CreateRevealMask(wall.revealMask[f]);
                if (x == 7 && z == 26)
                {
                    wall.hasTex[1] = true;   // 앞면(face=1)
                    wall.texID[1] = TextureManager::Get("footprint");
                }
                boxes.push_back(wall);

                Box wall2;
                wall2.size = glm::vec3(cellSize, wallHeight, cellSize);
                wall2.pos = glm::vec3(fx, wallHeight * 1.5f - 0.5f, fz);
                wall2.color = glm::vec3(0.1f, 0.1f, 0.1f);
                for (int f = 0; f < 6; f++)
                    CreateRevealMask(wall2.revealMask[f]);
                boxes.push_back(wall2);
            }
            bool makeCeil = false;
            if (z == entranceRow) makeCeil = entranceCeil[x];

            Box ceiling;
            ceiling.size = glm::vec3(cellSize, wallHeight, cellSize);
            ceiling.pos = glm::vec3(fx, wallHeight * 2.5f - 0.5f, fz);
            ceiling.color = glm::vec3(0.1f, 0.1f, 0.1f);
            for (int f = 0; f < 6; f++)
                CreateRevealMask(ceiling.revealMask[f]);
            boxes.push_back(ceiling);

            Box floor;
            floor.size = glm::vec3(cellSize, wallHeight, cellSize);
            floor.pos = glm::vec3(fx, wallHeight * (-0.5f) - 0.5f, fz);
            floor.color = glm::vec3(0.1f, 0.1f, 0.1f);
            for (int f = 0; f < 6; f++)
                CreateRevealMask(floor.revealMask[f]);

            if (x == 6 && z == 25)
            {
                floor.hasTex[5] = true;
                floor.texID[5] = TextureManager::Get("footprint");
                floor.texRot[5] = 1;
            }

            boxes.push_back(floor);

            if (makeCeil)
            {
                Box ceiling;
                ceiling.size = glm::vec3(cellSize, wallHeight, cellSize);
                ceiling.pos = glm::vec3(fx, wallHeight * 1.5f - 0.5f, fz);
                ceiling.color = glm::vec3(0.1f, 0.1f, 0.1f);
                for (int f = 0; f < 6; f++)
                    CreateRevealMask(ceiling.revealMask[f]);
                boxes.push_back(ceiling);
            }
        }
    }
}
