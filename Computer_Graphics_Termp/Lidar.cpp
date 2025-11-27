#define GLM_ENABLE_EXPERIMENTAL
#include "Lidar.h"
#include "Map.h"

#include <algorithm>
#include <cmath>

#include <gl/glm/gtc/type_ptr.hpp>
#include <gl/glm/gtx/quaternion.hpp>

Lidar::Lidar()
    : VAO(0)
    , VBO(0)
    , maxPoints(1000000)
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
    const int   steps = 5;
    const float halfAngle = glm::radians(1.0f);
    const float radius = std::tan(halfAngle);

    for (int iy = 0; iy < steps; ++iy)
    {
        float ny = static_cast<float>(iy) / (steps - 1);
        ny = ny * 2.0f - 1.0f;  // -1 ~ 1로 정규화

        for (int ix = 0; ix < steps; ++ix)
        {
            float nx = static_cast<float>(ix) / (steps - 1);
            nx = nx * 2.0f - 1.0f;

            float r2 = nx * nx + ny * ny;
            if (r2 > 1.0f)
            {
                continue;   // 원 밖이면 스킵 → 사각형 격자 중 원 안만 사용
            }

            // nx, ny 는 이제 단위 원 안의 좌표
            // 이걸 radius 만큼 스케일해서 forward 주변의 원만 렌더링
            float offsetX = nx * radius;
            float offsetY = ny * radius;

            glm::vec3 dir = forward + offsetX * right + offsetY * up;
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

bool Lidar::Raycast(
    const glm::vec3& origin,
    const glm::vec3& dir,
    const std::vector<Box>& boxes,
    float maxDist,
    glm::vec3& hitPos)
{
    bool hit = false;
    float closestT = maxDist;

    for (const Box& b : boxes)
    {
        glm::vec3 half = b.size * 0.5f;
        glm::vec3 minB = b.pos - half;
        glm::vec3 maxB = b.pos + half;

        float tmin = 0.0f;
        float tmax = maxDist;

        // X축
        if (fabs(dir.x) < 1e-6f)
        {
            // 레이가 X축에 평행일 때, 범위 밖이면 교차 없음
            if (origin.x < minB.x || origin.x > maxB.x)
                continue;
        }
        else
        {
            float tx1 = (minB.x - origin.x) / dir.x;
            float tx2 = (maxB.x - origin.x) / dir.x;
            if (tx1 > tx2) std::swap(tx1, tx2);
            tmin = std::max(tmin, tx1);
            tmax = std::min(tmax, tx2);
            if (tmin > tmax) continue;
        }

        // Y축
        if (fabs(dir.y) < 1e-6f)
        {
            if (origin.y < minB.y || origin.y > maxB.y)
                continue;
        }
        else
        {
            float ty1 = (minB.y - origin.y) / dir.y;
            float ty2 = (maxB.y - origin.y) / dir.y;
            if (ty1 > ty2) std::swap(ty1, ty2);
            tmin = std::max(tmin, ty1);
            tmax = std::min(tmax, ty2);
            if (tmin > tmax) continue;
        }

        // Z축
        if (fabs(dir.z) < 1e-6f)
        {
            if (origin.z < minB.z || origin.z > maxB.z)
                continue;
        }
        else
        {
            float tz1 = (minB.z - origin.z) / dir.z;
            float tz2 = (maxB.z - origin.z) / dir.z;
            if (tz1 > tz2) std::swap(tz1, tz2);
            tmin = std::max(tmin, tz1);
            tmax = std::min(tmax, tz2);
            if (tmin > tmax) continue;
        }

        // tmin이 음수면 레이가 박스 내부에서 시작한 경우  
        // 이때는 tmax를 충돌로 사용
        float tHit = tmin >= 0.0f ? tmin : tmax;

        if (tHit < 0.0f) continue;         // 뒤쪽 충돌 제거
        if (tHit > maxDist) continue;      // 최대 거리 넘어가면 무시

        if (tHit < closestT)
        {
            closestT = tHit;
            hit = true;
        }
    }

    if (!hit) return false;

    hitPos = origin + dir * closestT;
    return true;
}



void Lidar::ScanFanWide(
    const glm::vec3& origin,
    const glm::vec3& front,
    const glm::vec3& up,
    const std::vector<Box>& boxes)
{
    const int horizontal = 60;  // 좌우 resolution
    const int vertical = 40;    // 상하 resolution
    const float maxDist = 40.0f;

    // 사각형 크기 조절 (값을 줄이면 작아짐)
    const float widthScale = 0.2f;  // 좌우 크기
    const float heightScale = 0.2f; // 상하 크기

    glm::vec3 right = glm::normalize(glm::cross(front, up));

    for (int iy = 0; iy < vertical; iy++)
    {
        float y = ((float)iy / (vertical - 1) - 0.5f) * 2.0f * heightScale;

        for (int ix = 0; ix < horizontal; ix++)
        {
            float x = ((float)ix / (horizontal - 1) - 0.5f) * 2.0f * widthScale;

            glm::vec3 dir =
                glm::normalize(front + x * right + y * up);

            glm::vec3 hit;
            if (Raycast(origin, dir, boxes, maxDist, hit))
            {
                points.push_back(hit);
            }
        }
    }
}


void Lidar::StartScan(const glm::vec3& origin,
    const glm::vec3& front,
    const glm::vec3& up,
    const std::vector<Box>& boxes)
{

    scan.active = true;
    scan.curRow = 0;

    scan.origin = origin;
    scan.front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(scan.front, up));
    if (glm::length(right) < 0.001f)
        right = glm::normalize(glm::cross(scan.front, glm::vec3(1, 0, 0)));

    scan.up = glm::normalize(glm::cross(right, scan.front));
    scan.boxes = boxes;
}

void Lidar::UpdateScan()
{
    if (!scan.active) return;

    int row = scan.curRow;
    if (row >= scan.vertical)
    {
        scan.active = false;
        return;
    }

    glm::vec3 forward = scan.front;
    glm::vec3 up = scan.up;
    glm::vec3 right = glm::normalize(glm::cross(forward, up));

    float vAngle = ((float)row / (scan.vertical - 1) - 0.5f) * scan.vFov;

    for (int i = 0; i < scan.horizontal; i++)
    {
        float hAngle = ((float)i / (scan.horizontal - 1) - 0.5f) * scan.hFov;

        glm::quat qPitch = glm::angleAxis(vAngle, right);
        glm::quat qYaw = glm::angleAxis(hAngle, up);
        glm::quat q = qYaw * qPitch;

        glm::vec3 dir = glm::normalize(q * forward);

        glm::vec3 hit;
        if (Raycast(scan.origin, dir, scan.boxes, 40.0f, hit))
            points.push_back(hit);
    }

    scan.curRow++;   // 다음 줄로 넘어가기
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
