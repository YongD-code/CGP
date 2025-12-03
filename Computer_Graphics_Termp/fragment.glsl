#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;     

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform bool uDarkMode;
uniform int uTexRot;  

uniform sampler2D uTexture;  
uniform bool uHasTex; 

out vec4 FragColor;

void main()
{
    vec3 baseColor = objectColor;

    vec2 uv = TexCoord;

    if (uHasTex)
    {
        // 180도 회전 옵션
        if (uTexRot == 1)
            uv = vec2(1.0 - uv.x, 1.0 - uv.y);

        vec4 tex = texture(uTexture, uv);

        // 투명 알파 보정
        if (tex.a < 0.1)
            tex.rgb = vec3(0.1);

        baseColor = tex.rgb;
    }

    // Ambient
    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength * baseColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * baseColor;

    // Specular
    float specStrength = 0.35;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specStrength * spec * vec3(1.0);

    // final (texture participates in lighting normally)
    vec3 result = ambient + diffuse + specular;

    // dark mode
    if (uDarkMode) {

        // 스캔된 점은 원래색 유지
        if (objectColor == vec3(0.0, 1.0, 0.4)) {
            FragColor = vec4(objectColor, 1.0);
            return;
        }
        // 총
        if (objectColor == vec3(0.25, 0.25, 0.25)) {
            FragColor = vec4(objectColor, 1.0);
            return;
        }
        // 레이저
        if (objectColor == vec3(1.0, 0.0, 0.0)) {
            FragColor = vec4(objectColor, 1.0);
            return;
        }

        // dark mode에서 텍스처도 어둡게
        FragColor = vec4(baseColor * 0.03, 1.0);
        return;
    }

    FragColor = vec4(result, 1.0);
}
