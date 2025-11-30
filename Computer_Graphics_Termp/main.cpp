#define _CRT_SECURE_NO_WARNINGS

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

bool cull = false;
bool wire_mode = true;
int lastTime = 0;
Player g_player;
Map g_map;
Lidar g_lidar;
GunRenderer g_gun;

bool g_isScanning = false;

struct ScanBeam
{
    bool        active;

    float       maxLength;
    float       curLength;
    float       speed;
    float       tailTime;
};

ScanBeam g_beam{};

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
    glutCreateWindow("LIDAR Project - Step 0 (Cube + Normal)");
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

    return prog;
}

GLvoid InitCubeMesh()
{
    float vertices[] =
    {
        // 뒤쪽(-Z) 면
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f, // 0
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f, // 1
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f, // 2
        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f, // 3

        // 앞쪽(+Z) 면
        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f, // 4
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f, // 5
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f, // 6
        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f, // 7

        // 왼쪽(-X) 면
        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f, // 8
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f, // 9
        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f, // 10
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f, // 11

        // 오른쪽(+X) 면
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f, // 12
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f, // 13
         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f, // 14
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f, // 15

         // 아래(-Y) 면
         -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f, // 16
          0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f, // 17
          0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f, // 18
         -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f, // 19

         // 위(+Y) 면
         -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f, // 20
          0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f, // 21
          0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f, // 22
         -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f  // 23
    };

    unsigned int indices[] =
    {
        // 뒤
        1, 0, 3,   3, 2, 1,
        // 앞
        4, 5, 6,   6, 7, 4,
        // 왼
        11, 10, 9,   9, 8, 11,
        // 오른
        12,13,14,  14,15,12,
        // 아래
        16,17,18,  18,19,16,
        // 위
        20,23,22,  22,21,20
    };

    glGenVertexArrays(1, &VAO_cube);
    glGenBuffers(1, &VBO_cube);
    glGenBuffers(1, &EBO_cube);

    glBindVertexArray(VAO_cube);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_cube);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_cube);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

GLvoid InitGL()
{
    glClearColor(1.f, 1.f, 1.f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glutSetCursor(GLUT_CURSOR_NONE);
    InitCubeMesh();

    lastTime = glutGet(GLUT_ELAPSED_TIME);
    g_player.OnResize(width, height);
    const int mapW = 12;
    const int mapH = 14;

    int mapData[mapH][mapW] =
    {
        {1,1,1,1,1,1,1,1,1,1,1,1},
        {1,0,0,0,0,0,0,0,0,0,0,1},
        {1,0,1,1,0,0,1,1,1,0,0,1},
        {1,0,1,0,0,0,1,0,1,0,0,1},
        {1,0,1,0,1,1,1,0,1,1,0,1},
        {1,0,0,0,0,0,0,0,0,1,0,1},
        {1,1,1,1,1,1,0,1,0,1,0,1},
        {1,0,0,0,0,0,0,1,0,0,0,1},
        {1,1,1,1,1,1,0,1,1,1,1,1},
        {0,0,0,0,0,1,0,1,0,0,0,0},
        {0,0,0,0,0,1,0,1,0,0,0,0},
        {0,0,0,0,1,0,0,0,1,0,0,0},
        {0,0,0,0,1,0,0,0,1,0,0,0},
        {0,0,0,0,1,1,1,1,1,0,0,0},

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

GLvoid drawScene()
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (now - lastTime) * 0.001f;
    lastTime = now;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgramID);

    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 view = g_player.UpdateMoveAndGetViewMatrix(deltaTime, g_map);

    float aspect = static_cast<float>(width) / static_cast<float>(height);
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 200.0f);

    g_map.Draw(shaderProgramID, VAO_cube, uModelLoc, uViewLoc, uProjLoc, uColorLoc, view, proj);

    g_lidar.Draw(shaderProgramID,
        uModelLoc, uViewLoc, uProjLoc, uColorLoc,
        view, proj);

    g_gun.Draw(shaderProgramID,
        uModelLoc, uViewLoc, uProjLoc, uColorLoc,
        view, proj,
        g_player.camPos,
        g_player.camFront,
        g_player.camUp);

    glPointSize(4.0f);
    glBegin(GL_POINTS);
    for (auto& p : g_lidar.GetPoints())
    {
        glVertex3f(p.x, p.y, p.z);
    }
    glEnd();
    g_lidar.UpdateScan();

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

        if (!g_isScanning)
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
        
        glm::mat4 rot = glm::lookAt(glm::vec3(0, 0, 0), camFront, camUp);
        rot = glm::inverse(rot);

        // 총의 translate 값
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

        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex3f(start.x, start.y, start.z);
        glVertex3f(endPos.x, endPos.y, endPos.z);
        glEnd();
        glLineWidth(1.0f);
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
    if (key == 27) glutLeaveMainLoop();

    g_player.OnKeyDown(key);
}

void KeyUp(unsigned char key, int x, int y)
{
    g_player.OnKeyUp(key);
}

void MouseButton(int button, int state, int x, int y)
{
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

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
    {
        g_lidar.StartScan(
            g_player.camPos,
            g_player.camFront,
            g_player.camUp,
            g_map.GetBoxes()
        );
        StartScanBeam();
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