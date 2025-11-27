#include "Lidar.h"
#include "Map.h"

#include <algorithm>
#include <cmath>

#include <gl/glm/gtc/type_ptr.hpp>

Lidar::Lidar()
    : VAO(0)
    , VBO(0)
    , maxPoints(100000)
{
}

Lidar::~Lidar()
{
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
    }
    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
    }
}

void Lidar::Init()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * maxPoints, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // normal 은 상수 값으로 (0, 1, 0) 사용 (포인터니까 원래 법선이 필요없음 곱할 디폴트 노말값만 있으면 됨)
    glDisableVertexAttribArray(1);
    glVertexAttrib3f(1, 0.0f, 1.0f, 0.0f);

    glBindVertexArray(0);
}

void Lidar::Clear()
{
    points.clear();
}

// 내부용: Ray - AABB(Box) 교차 테스트
static bool RayAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const Box& box, float& tHit)
{
    glm::vec3 half = box.size * 0.5f;
    glm::vec3 minB = box.pos - half;
    glm::vec3 maxB = box.pos + half;

    float tmin = -1.0e9f;
    float tmax = 1.0e9f;

    for (int axis = 0; axis < 3; ++axis)
    {
        // ray의 시작벡터, 방향벡터, 박스의 축마다 최소/최대값 (겜수에서 배웠던 그 방정식)
        float originComponent;
        float dirComponent;
        float minComponent;
        float maxComponent;

        if (axis == 0)
        {
            originComponent = rayOrigin.x;
            dirComponent = rayDir.x;
            minComponent = minB.x;
            maxComponent = maxB.x;
        }
        else if (axis == 1)
        {
            originComponent = rayOrigin.y;
            dirComponent = rayDir.y;
            minComponent = minB.y;
            maxComponent = maxB.y;
        }
        else
        {
            originComponent = rayOrigin.z;
            dirComponent = rayDir.z;
            minComponent = minB.z;
            maxComponent = maxB.z;
        }

        // 방향벡터의 기울기가 평행에 근사값이면
        if (std::abs(dirComponent) < 1e-6f)
        {
            if (originComponent < minComponent || originComponent > maxComponent)   // 박스 밖에 있으면 교차 X
            {
                return false;
            }
        }
        else
        {
            float ood = 1.0f / dirComponent;
            float t1 = (minComponent - originComponent) * ood;  // P(t) = O + tD 에서 교차점인 t 계산 (반복문 돌면서 x, y, z 다 계산)
            float t2 = (maxComponent - originComponent) * ood;

            if (t1 > t2)    // 방향이 음수일 경우 t1 < t2 이므로 스왑
            {
                std::swap(t1, t2);
            }

            if (t1 > tmin)
            {
                tmin = t1;
            }
            if (t2 < tmax)
            {
                tmax = t2;
            }

            if (tmin > tmax)
            {
                return false;
            }
        }
    }

    // 전부 카메라 뒤쪽에 있는 경우
    if (tmax < 0.0f)
    {
        return false;
    }

    tHit = (tmin >= 0.0f) ? tmin : tmax;
    return true;
}

void Lidar::ScanFan(const glm::vec3& origin, const glm::vec3& front, const Map& map)
{
    glm::vec3 forward = glm::normalize(front);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    // 카메라 기준 오른쪽 / 위쪽 벡터
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // 스캔 영역 크기 / 해상도
    const int   steps = 5;                       // 5 x 5 = 25개 레이
    const float halfAngle = glm::radians(2.0f);      // ±2도 정도 크기
    const float tanA = std::tan(halfAngle);

    for (int iy = 0; iy < steps; ++iy)
    {
        float vy = (static_cast<float>(iy) / (steps - 1) - 0.5f); // -0.5 ~ 0.5

        for (int ix = 0; ix < steps; ++ix)
        {
            float vx = (static_cast<float>(ix) / (steps - 1) - 0.5f); // -0.5 ~ 0.5

            glm::vec3 dir = forward + vx * tanA * right + vy * tanA * up;
            dir = glm::normalize(dir);
            ScanSingleRay(origin, dir, map);
        }
    }
}

void Lidar::ScanSingleRay(const glm::vec3& origin, const glm::vec3& dir, const Map& map)
{
    glm::vec3 nDir = glm::normalize(dir);

    const std::vector<Box>& boxes = map.GetBoxes();

    float closestT = 1.0e9f;
    bool  hit = false;
    glm::vec3 hitPos(0.0f);

    for (const Box& b : boxes)
    {
        float t = 0.0f;
        if (RayAABB(origin, nDir, b, t))
        {
            if (t < closestT)
            {
                closestT = t;
                hitPos = origin + nDir * t;
                hit = true;
            }
        }
    }

    if (hit)
    {
        points.push_back(hitPos);

        if (points.size() > maxPoints)
        {
            //// 너무 많으면 앞부분 조금 잘라내기
            //points.erase(points.begin(), points.begin() + 1000);
        }
    }
}

void Lidar::Draw(GLuint shaderProgram,
    GLint uModelLoc,
    GLint uViewLoc,
    GLint uProjLoc,
    GLint uColorLoc,
    const glm::mat4& view,
    const glm::mat4& proj) const
{
    if (points.empty())
    {
        return;
    }

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * points.size(), points.data());

    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(proj));

    // 레이다 점 색 (밝은 초록색 계열)
    glm::vec3 color(0.0f, 1.0f, 0.4f);
    glUniform3fv(uColorLoc, 1, glm::value_ptr(color));

    glPointSize(4.0f);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));

    glBindVertexArray(0);
}
