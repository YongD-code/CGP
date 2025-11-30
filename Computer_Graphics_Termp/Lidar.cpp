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

void Lidar::AddHitPoint(const glm::vec3& p)
{
    points.push_back(p);

    if (points.size() > maxPoints)
    {
        points.erase(points.begin(), points.begin() + maxPoints - 1);
    }
}

void Lidar::ScanFan(const glm::vec3& origin, const glm::vec3& front, const Map& map)
{
    debugRays.clear();

    glm::vec3 forward = glm::normalize(front);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    // 카메라 기준 오른쪽 / 위쪽 벡터
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // 스캔 영역 크기 / 해상도
    const int   steps = 5;
    const float halfAngle = glm::radians(3.0f);
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

            debugRays.push_back(dir);
            ScanSingleRay(origin, dir, map);
        }
    }
}

void Lidar::ScanSingleRay(const glm::vec3& origin, const glm::vec3& dir, const Map& map)
{
    glm::vec3 nDir = glm::normalize(dir);

    const std::vector<Box>& boxes = map.GetBoxes();

    glm::vec3 hitPos;
    const float maxDist = 1000.0f;   // 맵 크기에 맞춰 나중에 조정 가능

    if (Raycast(origin, nDir, map.GetBoxes(), maxDist, hitPos))
    {
        AddHitPoint(hitPos);
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

void Lidar::StartScan(const glm::vec3& origin,
    const glm::vec3& front,
    const glm::vec3& up,
    const std::vector<Box>& boxes)
{

    scan.active = true;
    scan.curRow = scan.vertical - 1;

    scan.origin = origin;
    scan.front = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(scan.front, up));
    if (glm::length(right) < 0.001f)    // front와 up이 거의 평행하면 외적 결과인 right가 0 근사값이 될 수도 있음
        right = glm::normalize(glm::cross(scan.front, glm::vec3(1, 0, 0))); // 그러면 대체축을 통해서라도 right 계산

    scan.up = glm::normalize(glm::cross(right, scan.front));    // 스캔에 쓰일 진짜 up 계산
    scan.boxes = boxes;

    debugRays.clear();
}

void Lidar::UpdateScan()
{
    if (!scan.active) return;

    int row = scan.curRow;
    if (row < 0)
    {
        scan.active = false;
        return;
    }

    debugRays.clear();

    glm::vec3 forward = scan.front;
    glm::vec3 up = scan.up;
    glm::vec3 right = glm::normalize(glm::cross(forward, up));

    float vAngle = ((float)row / (scan.vertical - 1) - 0.5f) * scan.vFov;   // row 인덱스를 각도로 매핑 -vFov/2 ~ +vFov/2

    for (int i = 0; i < scan.horizontal; i++)
    {
        float hAngle = ((float)i / (scan.horizontal - 1) - 0.5f) * scan.hFov;   // 마찬가지

        glm::quat qPitch = glm::angleAxis(vAngle, right);   // right를 축으로 하는 쿼터니언
        glm::quat qYaw = glm::angleAxis(hAngle, up);
        glm::quat q = qYaw * qPitch;    // 하나의 쿼터니언으로 합침

        glm::vec3 dir = glm::normalize(q * forward);    // q * vector만 normalize에 넣어도 내부적으로 forward를 쿼터니언으로 바꾸고 q x p x q*을 해줌
                                                        // 쿼터니언과 벡터를 곱하면 위의 연산을 해주는 operator*가 오버로드됨 ㄷㄷ
        debugRays.push_back(dir);

        glm::vec3 hit;
        if (Raycast(scan.origin, dir, scan.boxes, 40.0f, hit))
            AddHitPoint(hit);
    }

    scan.curRow--;   // 다음 줄로 넘어가기
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
