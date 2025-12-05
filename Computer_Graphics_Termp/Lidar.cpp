#define GLM_ENABLE_EXPERIMENTAL
#include "Lidar.h"
#include "Map.h"

#include <algorithm>
#include <cmath>

#include <gl/glm/gtc/type_ptr.hpp>
#include <gl/glm/gtx/quaternion.hpp>

static void ComputeFaceUV(const Box& b, int face, int texRot, const glm::vec3& hitPos, float& u, float& v)
{
    glm::vec3 local = hitPos - b.pos;
    glm::vec3 half = b.size * 0.5f;

    switch (face)
    {
    case 0: // -Z
        u = (local.x + half.x) / b.size.x;
        v = (local.y + half.y) / b.size.y;
        u = 1.0f - u;
        break;

    case 1: // +Z
        u = (local.x + half.x) / b.size.x;
        v = (local.y + half.y) / b.size.y;
        break;

    case 2: // -X
        u = (local.z + half.z) / b.size.z;
        v = (local.y + half.y) / b.size.y;
        break;

    case 3: // +X
        u = (local.z + half.z) / b.size.z;
        v = (local.y + half.y) / b.size.y;
        u = 1.0f - u;
        break;

    case 4: // -Y
        u = (local.x + half.x) / b.size.x;
        v = (local.z + half.z) / b.size.z;
        break;

    case 5: // +Y
        u = (local.x + half.x) / b.size.x;
        v = (local.z + half.z) / b.size.z;
        break;
    }

    // texRot = 1 (180도 회전)
    if (texRot == 1)
    {
        u = 1.0f - u;
        v = 1.0f - v;
    }

}


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

    glm::vec3 hit;
    int boxIndex = -1;
    int faceIndex = -1;

    const float maxDist = 1000.0f;

    // ★ faceIndex와 boxIndex를 모두 얻는 Raycast 호출
    if (Raycast(origin, nDir, boxes, maxDist, hit, &boxIndex, &faceIndex))
    {
        AddHitPoint(hit);

        // ★ 유효한 박스 + face 일 때만 revealMask 기록
        if (boxIndex >= 0 && faceIndex >= 0)
        {
            const Box& b = boxes[boxIndex];

            float u, v;
            // ★ texRot 포함한 UV 계산
            ComputeFaceUV(b, faceIndex, b.texRot[faceIndex], hit, u, v);

            int X = int(u * 255);
            int Y = int(v * 255);

            glBindTexture(GL_TEXTURE_2D, b.revealMask[faceIndex]);

            const int R = 4;
            unsigned char value = 255;

            for (int j = -R; j <= R; j++)
            {
                for (int i = -R; i <= R; i++)
                {
                    if (i * i + j * j > R * R) continue;

                    int tx = X + i;
                    int ty = Y + j;
                    if (tx < 0 || tx > 255 || ty < 0 || ty > 255) continue;

                    glTexSubImage2D(GL_TEXTURE_2D, 0,
                        tx, ty,
                        1, 1,
                        GL_RED, GL_UNSIGNED_BYTE,
                        &value);
                }
            }
        }
    }
}


bool Lidar::Raycast(
    const glm::vec3& origin,
    const glm::vec3& dir,
    const std::vector<Box>& boxes,
    float maxDist,
    glm::vec3& hitPos,
    int* outBoxIndex,
    int* outFaceIndex)
{
    bool hit = false;
    float closestT = maxDist;

    int bestBox = -1;
    int bestFace = -1;

    for (int bi = 0; bi < boxes.size(); bi++)
    {
        const Box& b = boxes[bi];

        glm::vec3 half = b.size * 0.5f;
        glm::vec3 minB = b.pos - half;
        glm::vec3 maxB = b.pos + half;

        float tmin = 0.0f;
        float tmax = maxDist;

        int hitFace = -1;

        // X축 평면 충돌 판단
        if (fabs(dir.x) > 1e-6f)
        {
            float tx1 = (minB.x - origin.x) / dir.x;
            float tx2 = (maxB.x - origin.x) / dir.x;
            if (tx1 > tx2) std::swap(tx1, tx2);

            float old = tmin;
            tmin = std::max(tmin, tx1);
            tmax = std::min(tmax, tx2);

            if (tmin != old)
                hitFace = (dir.x > 0) ? 2 : 3;  // -X = 2, +X = 3

            if (tmin > tmax) continue;
        }
        else
        {
            if (origin.x < minB.x || origin.x > maxB.x)
                continue;
        }

        // Y축
        if (fabs(dir.y) > 1e-6f)
        {
            float ty1 = (minB.y - origin.y) / dir.y;
            float ty2 = (maxB.y - origin.y) / dir.y;
            if (ty1 > ty2) std::swap(ty1, ty2);

            float old = tmin;
            tmin = std::max(tmin, ty1);
            tmax = std::min(tmax, ty2);

            if (tmin != old)
                hitFace = (dir.y > 0) ? 4 : 5;  // -Y = 4, +Y = 5

            if (tmin > tmax) continue;
        }
        else
        {
            if (origin.y < minB.y || origin.y > maxB.y)
                continue;
        }

        // Z축
        if (fabs(dir.z) > 1e-6f)
        {
            float tz1 = (minB.z - origin.z) / dir.z;
            float tz2 = (maxB.z - origin.z) / dir.z;
            if (tz1 > tz2) std::swap(tz1, tz2);

            float old = tmin;
            tmin = std::max(tmin, tz1);
            tmax = std::min(tmax, tz2);

            if (tmin != old)
                hitFace = (dir.z > 0) ? 0 : 1;  // -Z = 0, +Z = 1

            if (tmin > tmax) continue;
        }
        else
        {
            if (origin.z < minB.z || origin.z > maxB.z)
                continue;
        }

        float tHit = tmin;

        if (tHit < 0.0f || tHit > maxDist)
            continue;

        if (tHit < closestT)
        {
            closestT = tHit;
            hit = true;
            bestBox = bi;
            bestFace = hitFace;
        }
    }

    if (!hit) return false;

    hitPos = origin + dir * closestT;

    if (outBoxIndex)  *outBoxIndex = bestBox;
    if (outFaceIndex) *outFaceIndex = bestFace;

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

    scan.rowTimer = 0.0f;
    scan.rowInterval = 0.03f;
    debugRays.clear();
}

void Lidar::UpdateScan(float deltaTime)
{
    if (!scan.active) return;

    scan.rowTimer += deltaTime;
    if (scan.rowTimer < scan.rowInterval)
    {
        return;
    }
    scan.rowTimer = 0.0f;

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

    float vAngle = ((float)row / (scan.vertical - 1) - 0.45f) * scan.vFov;   // row 인덱스를 각도로 매핑 -vFov/2 ~ +vFov/2 -> 미세조정, 살짝 위로 올림

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
        int boxIndex, faceIndex;
        if (Raycast(scan.origin, dir, scan.boxes, 1000.0f, hit, &boxIndex, &faceIndex))
        {
            AddHitPoint(hit);

            // ★ 여기에서 revealMask 칠하기
            Box& b = scan.boxes[boxIndex];

            float u, v;
            ComputeFaceUV(b, faceIndex, b.texRot[faceIndex], hit, u, v);

            int X = int(u * 255);
            int Y = int(v * 255);

            glBindTexture(GL_TEXTURE_2D, b.revealMask[faceIndex]);

            const int R = 6;
            unsigned char value = 255;

            for (int j = -R; j <= R; j++)
                for (int i = -R; i <= R; i++)
                {
                    if (i * i + j * j > R * R) continue;

                    int tx = X + i;
                    int ty = Y + j;

                    if (tx < 0 || tx > 255 || ty < 0 || ty > 255) continue;

                    glTexSubImage2D(GL_TEXTURE_2D, 0,
                        tx, ty,
                        1, 1,
                        GL_RED, GL_UNSIGNED_BYTE,
                        &value);
                }
        }
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
