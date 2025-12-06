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

void Map::Draw(

    GLuint shaderProgram,
    GLuint vaoCube,
    GLint uModelLoc,
    GLint uViewLoc,
    GLint uProjLoc,
    GLint uColorLoc,
    GLint uTexRotLoc,
    GLint uHasTexLoc,
    GLint uTextureLoc,
    GLint uRevealMaskLoc,
    GLint uFlipXLoc,
    const glm::mat4& view,
    const glm::mat4& proj
) const
{
    glUseProgram(shaderProgram);
    glBindVertexArray(vaoCube);

    glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(proj));

    for (const Box& b : boxes)
    {

        glm::mat4 model = glm::translate(glm::mat4(1.0f), b.pos);
        model = glm::scale(model, b.size);

        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        glUniform3fv(uColorLoc, 1, glm::value_ptr(b.color));

        for (int face = 0; face < 6; face++)
        {
            glUniform1i(uTexRotLoc, b.texRot[face]);
            glUniform1i(uFlipXLoc, b.texFlipX[face] ? 1 : 0);
            if (b.hasTex[face])
            {
                glUniform1i(uHasTexLoc, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, b.texID[face]);
                glUniform1i(uTextureLoc, 0);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, b.revealMask[face]);
                glUniform1i(uRevealMaskLoc, 1);
            }
            else
            {
                glUniform1i(uHasTexLoc, 0);
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
                if (x == 1 && z == 22)
                {
                    wall.hasTex[3] = true;   // 오른쪽면
                    wall.texID[3] = TextureManager::Get("hint2");
                    wall.texFlipX[3] = true;
                }
                if (x == 7 && z == 12)
                {
                    wall.hasTex[0] = true;   // 뒷면 
                    wall.texID[0] = TextureManager::Get("hint1");
                    wall.texFlipX[0] = true;
                }
                if (x == 6 && z == 30)
                {
                    wall.hasTex[0] = true;   // 뒷면 
                    wall.texID[0] = TextureManager::Get("rule");
                    wall.texFlipX[0] = true;
                }
                if (x == 13 && z == 5)
                {
                    wall.hasTex[2] = true;
                    wall.texID[2] = TextureManager::Get("human");
                }
                if (x == 5 && z == 18)
                {
                    wall.hasTex[3] = true;
                    wall.texID[3] = TextureManager::Get("help");
                    wall.texFlipX[3] = true;
                }
                boxes.push_back(wall);
                
                Box wall2;
                wall2.size = glm::vec3(cellSize, wallHeight, cellSize);
                wall2.pos = glm::vec3(fx, wallHeight * 1.5f - 0.5f, fz);
                wall2.color = glm::vec3(0.1f, 0.1f, 0.1f);
                for (int f = 0; f < 6; f++)
                    CreateRevealMask(wall2.revealMask[f]);
                if (x == 5 && z == 5)
                {
                    wall2.hasTex[2] = true;   // 왼쪽면
                    wall2.texID[2] = TextureManager::Get("hint4");
                }
                if (x == 6 && z == 30)
                {
                    wall2.hasTex[0] = true;   // 뒷면 
                    wall2.texID[0] = TextureManager::Get("project");
                    wall2.texFlipX[0] = true;
                }
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
            if (x == 14 && z == 14)
            {
                ceiling.hasTex[4] = true;
                ceiling.texID[4] = TextureManager::Get("hint3");
            }
            boxes.push_back(ceiling);

            Box floor;
            floor.size = glm::vec3(cellSize, wallHeight, cellSize);
            floor.pos = glm::vec3(fx, wallHeight * (-0.5f) - 0.5f, fz);
            floor.color = glm::vec3(0.1f, 0.1f, 0.1f);
            for (int f = 0; f < 6; f++)
                CreateRevealMask(floor.revealMask[f]);

            if (x == 6 && (z == 25||z == 26))
            {
                floor.hasTex[5] = true;
                floor.texID[5] = TextureManager::Get("footprint");
                floor.texRot[5] = 1;
            }
            if (x == 8 && (z == 2 || z == 3))
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

    {
        int doorMapX = 8;
        int doorMapZ = 1;

        float fx = (doorMapX - w / 2.0f) * cellSize + cellSize * 0.5f;
        float fz = (doorMapZ - h / 2.0f) * cellSize + cellSize * 0.5f;

        Box door;
        door.size = glm::vec3(cellSize, wallHeight, cellSize);
        door.pos = glm::vec3(fx, wallHeight * 0.5f - 0.5f, fz);
        door.color = glm::vec3(0.3f, 0.3f, 0.3f);

        for (int f = 0; f < 6; f++) CreateRevealMask(door.revealMask[f]);

        doorIndex = boxes.size();
        if (doorMapX == 8 && doorMapZ == 1)
        {
            door.hasTex[1] = true;   // 앞면(face=1)
            door.texID[1] = TextureManager::Get("background");
        }
        boxes.push_back(door);
    }

    //---------------------------------------------------------
    // 키패드 생성 (PC 키패드 형태)
    // [1][2][3]
    // [4][5][6]
    // [7][8][9]
    //    [0]
    //---------------------------------------------------------
    {
        float cellSize = 4.0f;
        float wallHeight = 4.0f;

        int doorMapX = 8;
        int doorMapZ = 1;

        float doorX = (doorMapX - w / 2.0f) * cellSize + cellSize * 0.5f;
        float doorZ = (doorMapZ - h / 2.0f) * cellSize + cellSize * 0.5f;

        glm::vec3 base = glm::vec3(doorX, 3.0f, doorZ + 2.0f);

        std::vector<int> layout = {
            1,2,3,
            4,5,6,
            7,8,9,
           -1,0,-1
        };

        float dx = 1.0f;
        float dy = 1.0f;

        keypadStartIndex = boxes.size();

        for (int r = 0; r < 4; r++)
        {
            for (int c = 0; c < 3; c++)
            {
                int k = layout[r * 3 + c];
                if (k == -1) continue;

                Box key;
                key.size = glm::vec3(0.8f, 0.8f, 0.15f);

                float ox = (c - 1) * dx;
                float oy = -(r * dy);

                key.pos = base + glm::vec3(ox, oy, 0.0f);

                key.hasTex[1] = true;
                key.texID[1] = TextureManager::Get("digit_" + std::to_string(k));

                key.color = glm::vec3(0.2f, 0.2f, 0.2f);

                for (int f = 0; f < 6; f++) CreateRevealMask(key.revealMask[f]);

                keypadDigits.push_back(k);
                boxes.push_back(key);
            }
        }

        keypadEndIndex = boxes.size() - 1;
    }

    scareBoxStartIndex = boxes.size(); 

    for (int i = 0; i < 3; ++i)
    {
        Box scareBox;
        scareBox.size = glm::vec3(0.0f, 0.0f, 0.0f);
        scareBox.pos = glm::vec3(0.0f, -9999.0f, 0.0f);
        scareBox.color = glm::vec3(1.0f, 1.0f, 1.0f);

        scareBox.hasTex[1] = true;
        CreateRevealMask(scareBox.revealMask[1]); 
        boxes.push_back(scareBox);
    }
}

