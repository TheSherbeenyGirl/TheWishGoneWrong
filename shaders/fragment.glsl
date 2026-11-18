const char* fragmentShaderSource = R"(
    #version 460 core

    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoords;

    out vec4 FragColor;

    uniform vec3 lightPos;
    uniform vec3 viewPos;

    uniform sampler2D diffuseTexture;
    uniform bool hasTexture;
    uniform vec4 baseColor;

    void main()
    {
        vec4 texColor = hasTexture
            ? texture(diffuseTexture, TexCoords)
            : vec4(1.0);

        vec4 surfaceColor = texColor * baseColor;

        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);

        float diff = max(dot(norm, lightDir), 0.0);

        vec3 ambient = 0.2 * surfaceColor.rgb;
        vec3 diffuse = diff * surfaceColor.rgb;

        vec3 result = ambient + diffuse;

        FragColor = vec4(result, surfaceColor.a);
    }
)";
