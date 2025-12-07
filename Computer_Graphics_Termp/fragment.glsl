#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform bool uDarkMode;

uniform bool uHasTex;
uniform sampler2D uTexture;

uniform sampler2D uRevealMask;   // 텍스처가 있는 면만 사용

uniform int  uTexRot;
uniform bool uFlipX;
uniform bool uIsScare;

void main()
{
    vec3 baseColor = objectColor;
    vec2 uv = TexCoord;

    if (uIsScare)
    {
        vec3 baseColor = objectColor;

        if (uHasTex)
        {
            vec4 texColor = texture(uTexture, TexCoord);
            baseColor *= texColor.rgb;
        }

        FragColor = vec4(baseColor, 1.0);
        return;
    }

    // 기본 텍스처 매핑
    if (uHasTex)
    {
        if (uTexRot == 1)
            uv = vec2(1.0 - uv.x, 1.0 - uv.y);

        if (uFlipX)
            uv.x = 1.0 - uv.x;

        vec4 tex = texture(uTexture, uv);
        if (tex.a < 0.1)
            tex.rgb = vec3(0.1);

        baseColor = tex.rgb;
    }

    // 조명 계산
    vec3 norm     = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff    = max(dot(norm, lightDir), 0.0);
    vec3 diffuse  = diff * baseColor;

    vec3 viewDir    = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular   = 0.35 * spec * vec3(1.0);

    vec3 ambient = 0.25 * baseColor;
    vec3 lit = ambient + diffuse + specular;

    // DarkMode 처리
    if (uDarkMode)
    {
        // 예외 처리 (총 / 레이저 / 스캔 포인트)
        if (objectColor == vec3(0.0,1.0,0.4) ||   // 스캔 점
            objectColor == vec3(0.25,0.25,0.25) ||// 총
            objectColor == vec3(1.0,0.0,0.0))     // 레이저
        {
            FragColor = vec4(lit, 1.0);
            return;
        }

        // 텍스처 없는 박스는 revealMask 적용 금지
        if (!uHasTex)
        {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        // 텍스처 있는 박스만 revealMask 적용
        float reveal = texture(uRevealMask, uv).r;

          if (reveal < 0.01)
        {
            // 스캔되지 않은 텍스처는 절대 안 보임
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
        }

        vec3 litTex = baseColor;  
        FragColor = vec4(litTex * reveal, 1.0);
        return;
    }

    FragColor = vec4(lit, 1.0);
}
