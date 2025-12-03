#version 330 core

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform bool uDarkMode;

out vec4 FragColor;

void main()
{
    // Ambient
    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength * objectColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * objectColor;

    // Specular
    float specStrength = 0.35;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specStrength * spec * vec3(1.0);

    // final
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);

    if (uDarkMode) {
    // 스캔된 점은 원래색 유지
    if (objectColor == vec3(0.0, 1.0, 0.4)) {
        FragColor = vec4(objectColor, 1.0);
        return;
    }
    //총은 보이도록 
    if (objectColor == vec3(0.25, 0.25, 0.25)) {
        FragColor = vec4(objectColor, 1.0);
        return;
    }
    // 레이저 광선도 보이도록
    if (objectColor == vec3(1.0, 0.0, 0.0)) {
        FragColor = vec4(objectColor, 1.0);
        return;
    }
    // 그 외는 어둡게
    FragColor = vec4(objectColor * 0.03, 1.0);
    return;
}

}
