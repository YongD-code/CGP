#pragma once

#include <vector>
#include <gl/glm/glm.hpp>
#include <gl/glew.h>

// 하나의 직육면체
struct Box
{
    glm::vec3 pos;   // 중심 위치
    glm::vec3 size;  // 스케일 (x, y, z)
    glm::vec3 color; // 색상 (0~1)
};

class Map
{
public:
    // 테스트용 방
    void InitTestRoom();

    // 맵 그리기
    void Draw(
        GLuint shaderProgram,
        GLuint vaoCube,
        GLint uModelLoc,
        GLint uViewLoc,
        GLint uProjLoc,
        GLint uColorLoc,
        const glm::mat4& view,
        const glm::mat4& proj
    ) const;

    // 2D 배열에따라 3D 맵을 렌더링하는 함수
    void InitFromArray(int w, int h, const int* data);

    // C++에서는 보통 클래스의 멤버 변수를 private: 영역에 둠
    // private: 영역에 있는 변수나 함수들은 클래스 외부에서 접근이 불가능함 (friend 키워드 쓰지 않는이상)
    // 그래서 보통 private: 영역에 있는 멤버 변수 값을 받아오기 위해서 접근 가능한 public: 영역에 getter 함수를 사용함
    const std::vector<Box>& GetBoxes() const
    {
        return boxes;
    }

private:
    std::vector<Box> boxes;
};
