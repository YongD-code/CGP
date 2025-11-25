#include "GunRender.h"
#include "tiny_obj_loader.h"
#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>

bool GunRenderer::Load(const char* path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path);
    if (!ret) return false;

    struct V {
        float x, y, z;
        float nx, ny, nz;
    };

    std::vector<V> vertices;
    std::vector<unsigned int> indices;

    for (auto& sh : shapes)
    {
        for (auto& idx : sh.mesh.indices)
        {
            tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
            tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
            tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

            tinyobj::real_t nx = 0, ny = 0, nz = 1;
            if (!attrib.normals.empty())
            {
                nx = attrib.normals[3 * idx.normal_index + 0];
                ny = attrib.normals[3 * idx.normal_index + 1];
                nz = attrib.normals[3 * idx.normal_index + 2];
            }

            vertices.push_back({ vx, vy, vz, nx, ny, nz });
            indices.push_back(indices.size());
        }
    }

    indexCount = indices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(V), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

void GunRenderer::Draw(GLuint shaderProgram,
    GLint uModelLoc,
    GLint uViewLoc,
    GLint uProjLoc,
    GLint uColorLoc,
    const glm::mat4& view,
    const glm::mat4& proj,
    const glm::vec3& camPos,
    const glm::vec3& camFront,
    const glm::vec3& camUp)
{
    glUseProgram(shaderProgram);

    // FPS 무기 특징:
    // 카메라 View 행렬의 회전 성분만 따고, 위치는 무시한다.
    glm::mat4 camRot = glm::mat4(glm::mat3(view)); // view의 rotation만 추출
    glm::mat4 invCamRot = glm::inverse(camRot);    // 카메라 회전 따라가기

    glm::mat4 model = glm::mat4(1.0f);

    // 1) 무기 기본 위치: 카메라 기준 오른쪽/아래/앞
    glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));

    glm::vec3 gunOffset =
        right * 0.38f     // 오른쪽으로 이동
        + camUp * -0.60f    // 아래로 이동
        + camFront * 0.65f; // 화면 안쪽으로 밀기

    model = glm::translate(model, camPos + gunOffset);

    // 2) 카메라 회전에 고정되도록 설정
    model *= invCamRot;

    // 3) OBJ 자체 회전 보정 (총이 옆으로 눕거나 뒤집히면 이 부분 조절)
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(-5.0f), glm::vec3(1, 0, 0)); // 살짝 위로 들어주는 보정

    // 4) 크기 조절
    model = glm::scale(model, glm::vec3(0.0035f));

    // Shader 전달
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(proj));

    glm::vec3 gunColor(0.15f, 0.15f, 0.15f);
    glUniform3fv(uColorLoc, 1, glm::value_ptr(gunColor));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
