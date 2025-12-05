#pragma once
#include <string>
#include <unordered_map>
#include <gl/glew.h>

GLuint LoadTexture(const char* filename);

class TextureManager
{
public:
    // 텍스처 이름 -> 텍스처 ID
    static std::unordered_map<std::string, GLuint> textures;

    // 텍스처 로드 (이미 로드되어 있으면 기존 ID 반환)
    static GLuint Load(const std::string& name, const std::string& file)
    {
        // 이미 로드된 텍스처면 그대로 반환
        if (textures.find(name) != textures.end())
            return textures[name];

        GLuint tex = LoadTexture(file.c_str());
        textures[name] = tex;
        return tex;
    }

    // 텍스처 가져오기
    static GLuint Get(const std::string& name)
    {
        if (textures.find(name) == textures.end())
            return 0;
        return textures[name];
    }

    // 텍스처 모두 제거
    static void Clear()
    {
        for (auto& kv : textures)
            glDeleteTextures(1, &kv.second);

        textures.clear();
    }
};
