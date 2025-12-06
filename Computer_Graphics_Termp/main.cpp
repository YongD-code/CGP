#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>

#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>
#include <gl/glm/gtc/matrix_transform.hpp>
#include <gl/glm/gtc/type_ptr.hpp>

#include "Player.h"
#include "Map.h"
#include "gunrender.h"
#include "tiny_obj_loader.h"
#include "Lidar.h"
#include "stb_image.h"
#include "TextureManager.h"

using std::cout;
using std::endl;
using std::vector;

void   make_vertexShaders();
void   make_fragmentShaders();
GLuint make_shaderProgram();

GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid InitGL();
GLvoid InitCubeMesh();
void KeyUp(unsigned char key, int x, int y);
void KeyDown(unsigned char key, int x, int y);
void MouseMotion(int x, int y);
void MouseButton(int button, int state, int x, int y);
void StartScanBeam();

GLuint width = 800, height = 600;
GLuint shaderProgramID = 0;
GLuint vertexShader = 0;
GLuint fragmentShader = 0;

GLuint VAO_cube = 0;
GLuint VBO_cube = 0;
GLuint EBO_cube = 0;

GLint uModelLoc = -1;
GLint uViewLoc = -1;
GLint uProjLoc = -1;
GLint uColorLoc = -1;
GLint uDarkModeLoc = -1;

GLint uTexRotLoc = -1;
GLint uFlipXLoc = -1;
GLint uHasTexLoc = -1;
GLint uTextureLoc = -1;
GLint uRevealMaskLoc = -1;

bool cull = false;
bool wire_mode = false;
int lastTime = 0;
Player g_player;
Map g_map;
Lidar g_lidar;
GunRenderer g_gun;

bool g_isScanning = false;
bool g_darkMode = false;
std::string password = "1209";
std::string entered = "";
bool g_doorOpening = false;
float g_doorFallY = 0.0f;
float g_doorFallSpeed = 6.0f;

struct ScanBeam
{
    bool        active;

    float       maxLength;
    float       curLength;
    float       speed;
    float       tailTime;
};

ScanBeam g_beam{};
float g_beamTime = 0.0f;

bool IsInputLocked()
{
    return g_lidar.IsScanActive();
}

static std::string readTextFile(const char* path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
    {
        return {};
    }

    std::string s;
    ifs.seekg(0, std::ios::end);
    s.resize(static_cast<size_t>(ifs.tellg()));
    ifs.seekg(0, std::ios::beg);
    ifs.read(&s[0], s.size());
    return s;
}

void main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(width, height);
    glutCreateWindow("LIDAR Game");
    glDisable(GL_CULL_FACE);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "ERROR: GLEW 초기화 실패\n";
        return;
    }

    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();
    if (shaderProgramID == 0)
    {
        std::cerr << "ERROR: shader program 생성 실패\n";
        return;
    }
    TextureManager::Load("footprint", "footprint.png");
    TextureManager::Load("hint1", "keypad_hint1.png");
    TextureManager::Load("hint2", "keypad_hint2.png");
    TextureManager::Load("hint3", "keypad_hint3.png");
    TextureManager::Load("hint4", "keypad_hint4.png");
    TextureManager::Load("background", "keypad_bg.png");
    TextureManager::Load("human", "human.png");
    for (int i = 0; i < 10; i++)
    {
        TextureManager::Load(
            "digit_" + std::to_string(i),
            "digit_" + std::to_string(i) + ".png"
        );
    }
    InitGL();

    glutDisplayFunc(drawScene);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);

    glutMotionFunc(MouseMotion);
    glutPassiveMotionFunc(MouseMotion);
    glutMouseFunc(MouseButton);

    glutMainLoop();
}

void make_vertexShaders()
{
    std::string src = readTextFile("vertex.glsl");
    if (src.empty())
    {
        std::cerr << "ERROR: vertex.glsl 읽기 실패\n";
        return;
    }

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* p = src.c_str();
    glShaderSource(vertexShader, 1, &p, nullptr);
    glCompileShader(vertexShader);

    GLint ok = GL_FALSE;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        std::vector<char> log(1024);
        glGetShaderInfoLog(vertexShader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << "ERROR: vertex shader 컴파일 실패\n" << log.data() << std::endl;
    }
}

void make_fragmentShaders()
{
    std::string src = readTextFile("fragment.glsl");
    if (src.empty())
    {
        std::cerr << "ERROR: fragment.glsl 읽기 실패\n";
        return;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* p = src.c_str();
    glShaderSource(fragmentShader, 1, &p, nullptr);
    glCompileShader(fragmentShader);

    GLint ok = GL_FALSE;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        std::vector<char> log(1024);
        glGetShaderInfoLog(fragmentShader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << "ERROR: fragment shader 컴파일 실패\n" << log.data() << std::endl;
    }
}

GLuint make_shaderProgram()
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertexShader);
    glAttachShader(prog, fragmentShader);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        std::vector<char> log(1024);
        glGetProgramInfoLog(prog, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << "ERROR: shader program 링크 실패\n" << log.data() << std::endl;
        glDeleteProgram(prog);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    uModelLoc = glGetUniformLocation(prog, "uModel");
    uViewLoc = glGetUniformLocation(prog, "uView");
    uProjLoc = glGetUniformLocation(prog, "uProj");
    uColorLoc = glGetUniformLocation(prog, "objectColor");
    uDarkModeLoc = glGetUniformLocation(prog, "uDarkMode");

    uTexRotLoc = glGetUniformLocation(prog, "uTexRot");
    uFlipXLoc = glGetUniformLocation(prog, "uFlipX");
    uHasTexLoc = glGetUniformLocation(prog, "uHasTex");
    uTextureLoc = glGetUniformLocation(prog, "uTexture");
    uRevealMaskLoc = glGetUniformLocation(prog, "uRevealMask");


    // 안전빵
    glUniform1i(uTextureLoc, 0);
    glUniform1i(uRevealMaskLoc, 1);


    return prog;
}

GLvoid InitCubeMesh()
{
    float vertices[] =
    {
        // 뒤(-Z)
        -0.5f,-0.5f,-0.5f,   0,0,-1,   0,0,
         0.5f,-0.5f,-0.5f,   0,0,-1,   1,0,
         0.5f, 0.5f,-0.5f,   0,0,-1,   1,1,
        -0.5f, 0.5f,-0.5f,   0,0,-1,   0,1,

        // 앞(+Z)
        -0.5f,-0.5f, 0.5f,   0,0,1,    0,0,
         0.5f,-0.5f, 0.5f,   0,0,1,    1,0,
         0.5f, 0.5f, 0.5f,   0,0,1,    1,1,
        -0.5f, 0.5f, 0.5f,   0,0,1,    0,1,

        // 왼(-X)
        -0.5f,-0.5f,-0.5f,  -1,0,0,    0,0,
        -0.5f, 0.5f,-0.5f,  -1,0,0,    0,1,
        -0.5f, 0.5f, 0.5f,  -1,0,0,    1,1,
        -0.5f,-0.5f, 0.5f,  -1,0,0,    1,0,

        // 오른(+X)
         0.5f,-0.5f,-0.5f,   1,0,0,    0,0,
         0.5f, 0.5f,-0.5f,   1,0,0,    0,1,
         0.5f, 0.5f, 0.5f,   1,0,0,    1,1,
         0.5f,-0.5f, 0.5f,   1,0,0,    1,0,

         // 아래(-Y)
         -0.5f,-0.5f,-0.5f,   0,-1,0,   0,0,
          0.5f,-0.5f,-0.5f,   0,-1,0,   1,0,
          0.5f,-0.5f, 0.5f,   0,-1,0,   1,1,
         -0.5f,-0.5f, 0.5f,   0,-1,0,   0,1,

         // 위(+Y)
         -0.5f, 0.5f,-0.5f,   0,1,0,    0,0,
          0.5f, 0.5f,-0.5f,   0,1,0,    1,0,
          0.5f, 0.5f, 0.5f,   0,1,0,    1,1,
         -0.5f, 0.5f, 0.5f,   0,1,0,    0,1,
    };

    unsigned int indices[] =
    {
        // 뒤
        1,0,3,  3,2,1,
        // 앞
        4,5,6,  6,7,4,
        // 왼
        11,10,9, 9,8,11,
        // 오른
        12,13,14, 14,15,12,
        // 아래
        16,17,18, 18,19,16,
        // 위
        20,23,22, 22,21,20
    };

    glGenVertexArrays(1, &VAO_cube);
    glGenBuffers(1, &VBO_cube);
    glGenBuffers(1, &EBO_cube);

    glBindVertexArray(VAO_cube);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_cube);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // aPos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // aNormal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // aTexCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}


GLvoid InitGL()
{
    glClearColor(1.f, 1.f, 1.f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glutSetCursor(GLUT_CURSOR_NONE);
    InitCubeMesh();

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    g_player.OnResize(width, height);
    const int mapW = 17;
    const int mapH = 31;

    int mapData[mapH][mapW] =
    {
        {0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0},//z 0
        {1,1,1,1,1,1,0,1,0,1,0,0,0,0,0,0,0},
        {1,0,0,0,0,1,1,1,0,1,1,1,1,1,0,0,0},
        {1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0},
        {1,0,0,0,0,1,1,1,1,1,1,1,0,1,0,0,0},
        {1,0,0,0,0,1,0,0,0,0,0,1,0,1,0,0,0},//z 5
        {1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,0},
        {0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0},
        {0,0,0,0,0,1,0,0,1,1,1,1,1,1,0,0,0},
        {0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0},//z 10
        {0,1,1,1,1,1,0,0,1,0,0,0,0,0,0,0,0},
        {0,1,0,0,0,0,0,1,0,0,0,0,1,1,1,1,1},
        {0,1,0,0,1,1,0,1,0,0,0,0,1,0,0,0,1},
        {0,1,0,0,1,1,0,1,0,0,0,0,1,0,0,0,1},
        {0,1,0,0,1,1,0,1,1,1,1,1,0,0,0,0,1},//z 15
        {0,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1},
        {0,1,0,0,1,1,0,1,1,1,1,1,0,0,0,0,1},
        {0,1,0,0,1,1,0,1,0,0,0,0,1,0,0,0,1},
        {0,1,0,0,1,1,0,1,0,0,0,0,1,0,0,0,1},
        {0,1,0,0,1,1,0,1,0,0,0,0,1,1,1,1,1},//z 20
        {0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
        {0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0},
        {0,1,1,1,1,1,0,1,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0},//z 25
        {0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0},
        {0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0},//z 30
// x =   0   2   4   6   8  10  12  14  16 
    };

    g_map.InitFromArray(mapW, mapH, &mapData[0][0]);
    g_gun.Load("Gun.obj");
    g_lidar.Init();


}

// 레이저 위치는 항상 갱신해야 화면이 확 돌아갔을때 레이저가 이상한 위치에 가있지 않음
void StartScanBeam()
{
    g_beam.active = true;
    g_beam.maxLength = 40.0f;
    g_beam.curLength = 0.0f;
    g_beam.speed = 200.0f;
    g_beam.tailTime = 0.05f;
}

GLuint LoadTexture(const char* filename)
{
    int w, h, n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &w, &h, &n, 0);
    if (!data) return 0;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLenum format = (n == 4 ? GL_RGBA : GL_RGB);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return tex;
}

void OpenDoor()
{
    g_doorOpening = true;
    g_doorFallY = 0.0f;
}


void OnDigitPressed(int d)
{
    //디버깅용 콘솔창에서 뭐 눌렸는지 확인해보기 위해서 일단 넣어놓음
    std::cout << "[KEYPAD] Pressed: " << d << std::endl;
    entered.push_back('0' + d);
    std::cout << "[KEYPAD] Buffer: " << entered << std::endl;

    if (entered.size() == password.size())
    {

        if (entered == password)
        {
            std::cout << "[KEYPAD] PASSWORD OK\n";
            OpenDoor();
        }
        else
        {
            std::cout << "[KEYPAD] PASSWORD FAIL\n";
            entered.clear();
        }
    }
}

GLvoid drawScene()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (now - lastTime) * 0.001f;
    lastTime = now;
    g_beamTime += deltaTime;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);
    glUniform1i(uDarkModeLoc, g_darkMode ? 1 : 0);

    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 view;
    if (IsInputLocked())
        view = glm::lookAt(g_player.camPos, g_player.camPos + g_player.camFront, g_player.camUp);
    else
        view = g_player.UpdateMoveAndGetViewMatrix(deltaTime, g_map);

    float aspect = static_cast<float>(width) / static_cast<float>(height);
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 200.0f);

    g_map.Draw(shaderProgramID, VAO_cube, uModelLoc, uViewLoc, uProjLoc, uColorLoc, uTexRotLoc, uHasTexLoc, uTextureLoc, uRevealMaskLoc,uFlipXLoc, view, proj);

    g_lidar.Draw(shaderProgramID,
        uModelLoc, uViewLoc, uProjLoc, uColorLoc,
        view, proj);

    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    g_gun.Draw(shaderProgramID,
        uModelLoc, uViewLoc, uProjLoc, uColorLoc,
        view, proj,
        g_player.camPos,
        g_player.camFront,
        g_player.camUp);

    g_lidar.UpdateScan(deltaTime);

    if (g_beam.active)
    {
        if (g_beam.curLength < g_beam.maxLength)
        {
            g_beam.curLength += g_beam.speed * deltaTime;
            if (g_beam.curLength > g_beam.maxLength)
            {
                g_beam.curLength = g_beam.maxLength;
            }
        }
        // g_isScanning은 좌클릭용 bool이라서 우클릭 시에는 항상 꺼져있음. scan이 active인지 체크하는 함수 추가
        bool anyScanActive = g_isScanning || g_lidar.IsScanActive();

        if (!anyScanActive)
        {
            g_beam.tailTime -= deltaTime;
            if (g_beam.tailTime <= 0.0f)
            {
                g_beam.active = false;
            }
        }

        glm::vec3 camPos = g_player.camPos;
        glm::vec3 camFront = glm::normalize(g_player.camFront);
        glm::vec3 camUp = glm::normalize(g_player.camUp);
        glm::vec3 camRight = glm::normalize(glm::cross(camFront, camUp));
        
        glm::mat4 rot = glm::lookAt(glm::vec3(0, 0, 0), camFront, camUp);
        rot = glm::inverse(rot);

        // 총의 월드변환을 그대로 받아와서 해보려고 했는데 계속 이상하게 찍힘
        // 그냥 숫자 노가다로 위치 맞춰놓음
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, camPos);
        m *= rot;
        m = glm::translate(m, glm::vec3(0.45f, -0.25f, -0.8f));

        // 총 모델의 원점이 월드에서 어디인지
        glm::vec3 gunBase = glm::vec3(m * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

        glm::vec3 start = gunBase + camFront * 0.35f;
        glm::vec3 endPos = start + camFront * g_beam.curLength;

        glm::mat4 modelBeam = glm::mat4(1.0f);
        glUniformMatrix4fv(uModelLoc, 1, GL_FALSE, glm::value_ptr(modelBeam));
        glUniformMatrix4fv(uViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, glm::value_ptr(proj));

        glm::vec3 beamColor(1.0f, 0.0f, 0.0f);
        glUniform3fv(uColorLoc, 1, glm::value_ptr(beamColor));

        const vector<glm::vec3>& rays = g_lidar.GetDebugRays();

        glLineWidth(3.0f);
        glBegin(GL_LINES);

        int rayIndex = 0;
        // 저장해놓았던 ray선들을 다 렌더링
        for (const glm::vec3& dir : rays)
        {
            glm::vec3 ndir = glm::normalize(dir);

            // 1000 같은 상수값은 만져보면서 수정, 0.7, 1.3 이런 값은 레이마다 다르게 흔들리도록
            float phase1 = g_beamTime * 1000.0f + rayIndex * 0.7;
            float phase2 = g_beamTime * 1000.0f + rayIndex * 1.3;
            float shakeAmp = 0.2f; // 흔들림 세기

            // x축, y축 방향으로 흔들림 벡터 생성 / sin, cos값이 시간에 따라 계속 바뀌면서 흔들림 효과 생성
            glm::vec3 jitter =
                camRight * (sinf(phase1) * shakeAmp) +
                camUp * (cosf(phase2) * shakeAmp);

            glm::vec3 endPos = start + ndir * g_beam.curLength + jitter;

            glVertex3f(start.x, start.y, start.z);
            glVertex3f(endPos.x, endPos.y, endPos.z);

            ++rayIndex;
        }
        glEnd();
        glLineWidth(1.0f);
    }

    if (g_doorOpening)
    {
        auto& bx = g_map.GetBoxesMutable();

        float dy = g_doorFallSpeed * deltaTime;


        if (g_map.doorIndex >= 0 && g_map.doorIndex < bx.size())
        {
            bx[g_map.doorIndex].pos.y -= dy;
        }

        for (int i = g_map.keypadStartIndex; i <= g_map.keypadEndIndex; i++)
        {
            if (i >= 0 && i < bx.size())
            {
                bx[i].pos.y -= dy;
            }
        }

        g_doorFallY += dy;

        if (g_doorFallY > 20.0f)
        {
            if (g_map.doorIndex >= 0 && g_map.doorIndex < bx.size())
            {
                bx[g_map.doorIndex].size = glm::vec3(0, 0, 0);
                bx[g_map.doorIndex].pos.y = -9999.0f;
            }

            for (int i = g_map.keypadStartIndex; i <= g_map.keypadEndIndex; i++)
            {
                if (i >= 0 && i < bx.size())
                {
                    bx[i].size = glm::vec3(0, 0, 0);
                    bx[i].pos.y = -9999.0f;
                }
            }

            g_doorOpening = false;
        }
    }
    glutSwapBuffers();
    glutPostRedisplay();
}

GLvoid Reshape(int w, int h)
{
    width = w;
    height = h;
    glViewport(0, 0, w, h);
    g_player.OnResize(w, h);

    glutPostRedisplay();
}

void KeyDown(unsigned char key, int x, int y)
{
    if (key == 'h') {
        cull = !cull;
        if (cull) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
    }
    if (key == 'm') {
        wire_mode = !wire_mode;
        if (wire_mode)  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    if (key == 'l')
    {
        g_darkMode = !g_darkMode;
    }

    if (key == 27) glutLeaveMainLoop();

    if (IsInputLocked())
        return;
    g_player.OnKeyDown(key);
}

void KeyUp(unsigned char key, int x, int y)
{
    g_player.OnKeyUp(key);
}

void MouseButton(int button, int state, int x, int y)
{
    //이거는 좌클릭인데 레이저 안나가고 내가 누른거 버퍼에 들어가게 하는거임
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        glm::vec3 origin = g_player.camPos;
        glm::vec3 dir = glm::normalize(g_player.camFront);

        const auto& bx = g_map.GetBoxes();

        int hit = -1;

        for (int i = g_map.keypadStartIndex; i <= g_map.keypadEndIndex; i++)
        {
            glm::vec3 hp;
            int face = -1;

            if (g_lidar.Raycast(origin, dir, { bx[i] }, 1000.0f, hp, nullptr, &face))
            {
                hit = i;
                break;
            }
        }

        if (hit != -1)
        {
            int idx = hit - g_map.keypadStartIndex;
            int digit = g_map.keypadDigits[idx];
            OnDigitPressed(digit);
            return;
        }
    }

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN && !g_isScanning)
    {
        g_lidar.StartScan(
            g_player.camPos,
            g_player.camFront,
            g_player.camUp,
            g_map.GetBoxes()
        );
        StartScanBeam();
    }
    if (IsInputLocked())
        return;
    if (button == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            g_isScanning = true;
            glm::vec3 origin = g_player.GetPosition();
            glm::vec3 front = g_player.GetFront();
            g_lidar.ScanFan(origin, front, g_map);

            StartScanBeam();
        }
        else if (state == GLUT_UP)
        {
            g_isScanning = false;
        }
    }
}


void MouseMotion(int x, int y)
{
    g_player.OnMouseMotion(x, y);

    if (g_isScanning)
    {
        glm::vec3 origin = g_player.GetPosition();
        glm::vec3 front = g_player.GetFront();
        g_lidar.ScanFan(origin, front, g_map);
    }
}