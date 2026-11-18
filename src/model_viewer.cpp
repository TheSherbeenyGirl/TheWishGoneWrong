#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstddef>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <cstdio>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <cstdint>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// ============================================================
// SETTINGS
// ============================================================

const unsigned int WIDTH = 1200;
const unsigned int HEIGHT = 800;

const std::string MODEL_PATH = "assets/models/lamp.glb";

// ============================================================
// VERTEX SHADER
// ============================================================

const char* vertexShaderSource = R"(
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    FragPos = worldPos.xyz;
    TexCoords = aTexCoords;

    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);

    // Re-orthogonalize tangent basis.
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));

    TBN = mat3(T, B, N);
    Normal = N;

    gl_Position = projection * view * worldPos;
}
)";

// ============================================================
// FRAGMENT SHADER - PBR
// ============================================================

const char* fragmentShaderSource = R"(
#version 460 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;

uniform vec3 camPos;

// Original GLB textures
uniform sampler2D baseColorMap;
uniform sampler2D metallicRoughnessMap;
uniform sampler2D normalMap;

uniform bool hasBaseColor;
uniform bool hasMetallicRoughness;
uniform bool hasNormal;

// Material factors
uniform float metallicFactor;
uniform float roughnessFactor;

// ------------------------------------------------------------
// PBR FUNCTIONS
// ------------------------------------------------------------

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denominator =
        NdotH2 * (a2 - 1.0) + 1.0;

    return a2 /
        max(PI * denominator * denominator, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV /
        max(NdotV * (1.0 - k) + k, 0.0001);
}

float GeometrySmith(
    vec3 N,
    vec3 V,
    vec3 L,
    float roughness
)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 +
        (1.0 - F0) *
        pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ------------------------------------------------------------
// DIRECT LIGHTING
// ------------------------------------------------------------

vec3 CalculateLight(
    vec3 lightPosition,
    vec3 lightColor,
    vec3 N,
    vec3 V,
    vec3 albedo,
    float metallic,
    float roughness
)
{
    vec3 L = normalize(lightPosition - FragPos);
    vec3 H = normalize(V + L);

    float distance = length(lightPosition - FragPos);

    float attenuation = 1.0 / max(distance * distance, 0.01);

    vec3 radiance = lightColor * attenuation;

    vec3 F0 = mix(
        vec3(0.04),
        albedo,
        metallic
    );

    vec3 F = FresnelSchlick(
        max(dot(H, V), 0.0),
        F0
    );

    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);

    vec3 numerator = NDF * G * F;

    float denominator =
        4.0 *
        max(dot(N, V), 0.0) *
        max(dot(N, L), 0.0);

    vec3 specular = numerator / max(denominator, 0.001);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;

    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular)
        * radiance
        * NdotL;
}

// ------------------------------------------------------------
// SIMPLE STUDIO ENVIRONMENT
// ------------------------------------------------------------

vec3 EnvironmentColor(vec3 direction)
{
    float height = direction.y * 0.5 + 0.5;

    vec3 sky = vec3(0.75, 0.78, 0.85);
    vec3 ground = vec3(0.18, 0.13, 0.08);

    vec3 environment = mix(ground, sky, height);

    // Bright studio panels.
    float panel1 = pow(
        max(dot(direction, normalize(vec3(1.0, 0.7, 0.5))), 0.0),
        32.0
    );

    float panel2 = pow(
        max(dot(direction, normalize(vec3(-1.0, 0.5, -0.4))), 0.0),
        24.0
    );

    environment += vec3(2.0, 1.65, 1.15) * panel1;
    environment += vec3(1.2, 1.3, 1.5) * panel2;

    return environment;
}

// ============================================================
// MAIN
// ============================================================

void main()
{
    // --------------------------------------------------------
    // ORIGINAL BASE COLOR
    // --------------------------------------------------------

    vec4 baseColor = vec4(1.0);

    if (hasBaseColor)
    {
        baseColor = texture(baseColorMap, TexCoords);
    }

    vec3 albedo = baseColor.rgb;

    // --------------------------------------------------------
    // METALLIC AND ROUGHNESS
    // glTF: Roughness = Green, Metallic = Blue
    // --------------------------------------------------------

    float metallic = metallicFactor;
    float roughness = roughnessFactor;

    if (hasMetallicRoughness)
    {
        vec3 mr = texture(
            metallicRoughnessMap,
            TexCoords
        ).rgb;

        roughness *= mr.g;
        metallic *= mr.b;
    }

    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.045, 1.0);

    // --------------------------------------------------------
    // NORMAL MAP
    // --------------------------------------------------------

    vec3 N = normalize(Normal);

    if (hasNormal)
    {
        vec3 normalSample = texture(
            normalMap,
            TexCoords
        ).rgb;

        normalSample = normalSample * 2.0 - 1.0;

        N = normalize(TBN * normalSample);
    }

    vec3 V = normalize(camPos - FragPos);

    // --------------------------------------------------------
    // STUDIO LIGHTS
    // --------------------------------------------------------

    vec3 color = vec3(0.0);

    color += CalculateLight(
        vec3(3.0, 4.0, 4.0),
        vec3(65.0, 57.0, 45.0),
        N, V, albedo, metallic, roughness
    );

    color += CalculateLight(
        vec3(-4.0, 2.0, 1.0),
        vec3(35.0, 40.0, 50.0),
        N, V, albedo, metallic, roughness
    );

    color += CalculateLight(
        vec3(0.0, 3.0, -4.0),
        vec3(50.0, 35.0, 20.0),
        N, V, albedo, metallic, roughness
    );

    // --------------------------------------------------------
    // ENVIRONMENT REFLECTION APPROXIMATION
    // --------------------------------------------------------

    vec3 R = reflect(-V, N);

    vec3 environment = EnvironmentColor(R);

    vec3 F0 = mix(
        vec3(0.04),
        albedo,
        metallic
    );

    vec3 F = FresnelSchlick(
        max(dot(N, V), 0.0),
        F0
    );

    // Metallic surfaces reflect their environment.
    vec3 ambientDiffuse =
        EnvironmentColor(N) * albedo * (1.0 - metallic);

    vec3 ambientSpecular =
        environment * F * (1.0 - roughness * 0.65);

    color += ambientDiffuse * 0.25;
    color += ambientSpecular * 0.55;

    // --------------------------------------------------------
    // TONE MAPPING
    // --------------------------------------------------------

    color = color / (color + vec3(1.0));

    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, baseColor.a);
}
)";
// ============================================================
// CAMERA BACKGROUND SHADERS
// ============================================================

const char* backgroundVertexShader = R"(
#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* backgroundFragmentShader = R"(
#version 460 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D cameraTexture;

void main()
{
    FragColor = texture(cameraTexture, TexCoord);
}
)";
// ============================================================
// SHADER COMPILATION
// ============================================================

unsigned int compileShader(GLenum type, const char* source)
{
    unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char log[2048];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 2048, nullptr, log);
        std::cerr << "SHADER ERROR:\n" << log << std::endl;
    }

    return shader;
}

unsigned int createShaderProgram()
{
    unsigned int vs = compileShader(
        GL_VERTEX_SHADER,
        vertexShaderSource
    );

    unsigned int fs = compileShader(
        GL_FRAGMENT_SHADER,
        fragmentShaderSource
    );

    unsigned int program = glCreateProgram();

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    int success;
    char log[2048];

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(program, 2048, nullptr, log);
        std::cerr << "LINK ERROR:\n" << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}
unsigned int createBackgroundShaderProgram()
{
    unsigned int vs = compileShader(
        GL_VERTEX_SHADER,
        backgroundVertexShader
    );

    unsigned int fs = compileShader(
        GL_FRAGMENT_SHADER,
        backgroundFragmentShader
    );

    unsigned int program = glCreateProgram();

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char log[2048];

        glGetProgramInfoLog(
            program,
            2048,
            nullptr,
            log
        );

        std::cerr << "BACKGROUND SHADER LINK ERROR:\n"
                  << log << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}
// ============================================================
// CAMERA BACKGROUND QUAD
// ============================================================

unsigned int backgroundVAO = 0;
unsigned int backgroundVBO = 0;
unsigned int cameraTexture = 0;

void createBackgroundQuad()
{
    float vertices[] = {
        // Position       // Texture coordinates
        -1.0f,  1.0f,     0.0f, 1.0f,
        -1.0f, -1.0f,     0.0f, 0.0f,
         1.0f, -1.0f,     1.0f, 0.0f,

        -1.0f,  1.0f,     0.0f, 1.0f,
         1.0f, -1.0f,     1.0f, 0.0f,
         1.0f,  1.0f,     1.0f, 1.0f
    };

    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);

    glBindVertexArray(backgroundVAO);

    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    // Position
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    // Texture coordinates
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float),
        (void*)(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}
void createCameraTexture()
{
    glGenTextures(1, &cameraTexture);
    glBindTexture(GL_TEXTURE_2D, cameraTexture);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    glBindTexture(GL_TEXTURE_2D, 0);
}
// ============================================================
// TEXTURE LOADING
// ============================================================

unsigned int loadEmbeddedTexture(
    const aiScene* scene,
    const aiString& texturePath,
    bool srgb
)
{
    const aiTexture* embedded =
        scene->GetEmbeddedTexture(texturePath.C_Str());

    if (!embedded)
    {
        std::cerr << "Embedded texture not found: "
                  << texturePath.C_Str() << std::endl;
        return 0;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = nullptr;

    std::vector<unsigned char> rawPixels;

    if (embedded->mHeight == 0)
    {
        // Compressed PNG/JPEG image inside GLB.
        pixels = stbi_load_from_memory(
            reinterpret_cast<const unsigned char*>(embedded->pcData),
            static_cast<int>(embedded->mWidth),
            &width,
            &height,
            &channels,
            4
        );
    }
    else
    {
        // Uncompressed embedded image.
        width = static_cast<int>(embedded->mWidth);
        height = static_cast<int>(embedded->mHeight);

        rawPixels.resize(width * height * 4);

        for (int i = 0; i < width * height; i++)
        {
            rawPixels[i * 4 + 0] = embedded->pcData[i].r;
            rawPixels[i * 4 + 1] = embedded->pcData[i].g;
            rawPixels[i * 4 + 2] = embedded->pcData[i].b;
            rawPixels[i * 4 + 3] = embedded->pcData[i].a;
        }

        pixels = rawPixels.data();
    }

    if (!pixels)
    {
        std::cerr << "Failed to decode embedded texture: "
                  << texturePath.C_Str() << std::endl;
        return 0;
    }

    unsigned int textureID;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum internalFormat = srgb
        ? GL_SRGB8_ALPHA8
        : GL_RGBA8;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    if (embedded->mHeight == 0)
    {
        stbi_image_free(pixels);
    }

    std::cout << "Loaded embedded texture: "
              << texturePath.C_Str()
              << " (" << width << "x" << height << ")"
              << std::endl;

    return textureID;
}

unsigned int loadMaterialTexture(
    const aiScene* scene,
    aiMaterial* material,
    aiTextureType type,
    bool srgb
)
{
    if (material->GetTextureCount(type) == 0)
    {
        return 0;
    }

    aiString path;

    if (material->GetTexture(type, 0, &path) != AI_SUCCESS)
    {
        return 0;
    }

    return loadEmbeddedTexture(scene, path, srgb);
}

// ============================================================
// VERTEX AND MESH
// ============================================================

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct Mesh
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    unsigned int indexCount = 0;

    unsigned int baseColorTexture = 0;
    unsigned int metallicRoughnessTexture = 0;
    unsigned int normalTexture = 0;

    bool hasBaseColor = false;
    bool hasMetallicRoughness = false;
    bool hasNormal = false;
};

std::vector<Mesh> meshes;

glm::vec3 modelCenter(0.0f);
float modelScale = 1.0f;

// ============================================================
// CREATE MESH
// ============================================================

Mesh createMesh(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices
)
{
    Mesh mesh;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    // Position
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, normal)
    );
    glEnableVertexAttribArray(1);

    // UV
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, texCoords)
    );
    glEnableVertexAttribArray(2);

    // Tangent
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, tangent)
    );
    glEnableVertexAttribArray(3);

    // Bitangent
    glVertexAttribPointer(
        4, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, bitangent)
    );
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    mesh.indexCount = static_cast<unsigned int>(indices.size());

    return mesh;
}

// ============================================================
// LOAD MODEL
// ============================================================

bool loadModel(const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->mRootNode)
    {
        std::cerr << "ASSIMP ERROR: "
                  << importer.GetErrorString()
                  << std::endl;
        return false;
    }

    std::cout << "\nMODEL LOADED\n";
    std::cout << "Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "Materials: " << scene->mNumMaterials << std::endl;

    glm::vec3 minBounds(1e9f);
    glm::vec3 maxBounds(-1e9f);

    // Calculate model bounds.
    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* mesh = scene->mMeshes[m];

        for (unsigned int v = 0; v < mesh->mNumVertices; v++)
        {
            aiVector3D p = mesh->mVertices[v];

            glm::vec3 position(p.x, p.y, p.z);

            minBounds = glm::min(minBounds, position);
            maxBounds = glm::max(maxBounds, position);
        }
    }

    modelCenter = (minBounds + maxBounds) * 0.5f;

    glm::vec3 dimensions = maxBounds - minBounds;

    float largestDimension = std::max(
        dimensions.x,
        std::max(dimensions.y, dimensions.z)
    );

    if (largestDimension > 0.0001f)
    {
        modelScale = 2.0f / largestDimension;
    }

    // Load every mesh.
    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* aiMeshData = scene->mMeshes[m];

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve(aiMeshData->mNumVertices);

        for (unsigned int v = 0; v < aiMeshData->mNumVertices; v++)
        {
            Vertex vertex{};

            aiVector3D p = aiMeshData->mVertices[v];
            aiVector3D n = aiMeshData->mNormals[v];

            vertex.position = glm::vec3(p.x, p.y, p.z);
            vertex.normal = glm::vec3(n.x, n.y, n.z);

            if (aiMeshData->HasTextureCoords(0))
            {
                aiVector3D uv = aiMeshData->mTextureCoords[0][v];
                vertex.texCoords = glm::vec2(uv.x, uv.y);
            }

            if (aiMeshData->HasTangentsAndBitangents())
            {
                aiVector3D t = aiMeshData->mTangents[v];
                aiVector3D b = aiMeshData->mBitangents[v];

                vertex.tangent = glm::vec3(t.x, t.y, t.z);
                vertex.bitangent = glm::vec3(b.x, b.y, b.z);
            }
            else
            {
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                vertex.bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        for (unsigned int f = 0; f < aiMeshData->mNumFaces; f++)
        {
            const aiFace& face = aiMeshData->mFaces[f];

            for (unsigned int i = 0; i < face.mNumIndices; i++)
            {
                indices.push_back(face.mIndices[i]);
            }
        }

        Mesh mesh = createMesh(vertices, indices);

        // ----------------------------------------------------
        // ORIGINAL MATERIAL
        // ----------------------------------------------------

        if (aiMeshData->mMaterialIndex < scene->mNumMaterials)
        {
            aiMaterial* material =
                scene->mMaterials[aiMeshData->mMaterialIndex];

            // Base color: glTF BASE_COLOR, fallback DIFFUSE.
            mesh.baseColorTexture = loadMaterialTexture(
                scene,
                material,
                aiTextureType_BASE_COLOR,
                true
            );

            if (mesh.baseColorTexture == 0)
            {
                mesh.baseColorTexture = loadMaterialTexture(
                    scene,
                    material,
                    aiTextureType_DIFFUSE,
                    true
                );
            }

            // Metallic-roughness packed texture.
            mesh.metallicRoughnessTexture = loadMaterialTexture(
                scene,
                material,
                aiTextureType_METALNESS,
                false
            );

            if (mesh.metallicRoughnessTexture == 0)
            {
                mesh.metallicRoughnessTexture = loadMaterialTexture(
                    scene,
                    material,
                    aiTextureType_DIFFUSE_ROUGHNESS,
                    false
                );
            }

            // Normal map.
            mesh.normalTexture = loadMaterialTexture(
                scene,
                material,
                aiTextureType_NORMALS,
                false
            );

            if (mesh.normalTexture == 0)
            {
                mesh.normalTexture = loadMaterialTexture(
                    scene,
                    material,
                    aiTextureType_HEIGHT,
                    false
                );
            }

            mesh.hasBaseColor = mesh.baseColorTexture != 0;
            mesh.hasMetallicRoughness =
                mesh.metallicRoughnessTexture != 0;
            mesh.hasNormal = mesh.normalTexture != 0;
        }

        std::cout
            << "Mesh " << m
            << " | Vertices: " << vertices.size()
            << " | Base color: " << (mesh.hasBaseColor ? "YES" : "NO")
            << " | Metallic/Roughness: "
            << (mesh.hasMetallicRoughness ? "YES" : "NO")
            << " | Normal: " << (mesh.hasNormal ? "YES" : "NO")
            << std::endl;

        meshes.push_back(mesh);
    }

    return true;
}

// ============================================================
// DRAW MODEL
// ============================================================

void drawModel(unsigned int shaderProgram)
{
    for (const Mesh& mesh : meshes)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.baseColorTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mesh.metallicRoughnessTexture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mesh.normalTexture);

        glUniform1i(
            glGetUniformLocation(shaderProgram, "hasBaseColor"),
            mesh.hasBaseColor
        );

        glUniform1i(
            glGetUniformLocation(shaderProgram, "hasMetallicRoughness"),
            mesh.hasMetallicRoughness
        );

        glUniform1i(
            glGetUniformLocation(shaderProgram, "hasNormal"),
            mesh.hasNormal
        );

        glBindVertexArray(mesh.VAO);

        glDrawElements(
            GL_TRIANGLES,
            mesh.indexCount,
            GL_UNSIGNED_INT,
            nullptr
        );
    }

    glBindVertexArray(0);
}

// ============================================================
// MAIN
// ============================================================
// ============================================================
// MAIN
// ============================================================

int main()
{
    // --------------------------------------------------------
    // GLFW INITIALIZATION
    // --------------------------------------------------------

    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "The Wish Gone Wrong - AR",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // --------------------------------------------------------
    // GLAD
    // --------------------------------------------------------

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "GLAD initialization failed\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: "
              << glGetString(GL_VERSION)
              << std::endl;

    glViewport(0, 0, WIDTH, HEIGHT);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // --------------------------------------------------------
    // SHADERS
    // --------------------------------------------------------

    unsigned int shaderProgram = createShaderProgram();

    unsigned int backgroundShaderProgram =
        createBackgroundShaderProgram();

    // --------------------------------------------------------
    // CAMERA BACKGROUND
    // --------------------------------------------------------

    createBackgroundQuad();
    createCameraTexture();

    glUseProgram(backgroundShaderProgram);

    glUniform1i(
        glGetUniformLocation(
            backgroundShaderProgram,
            "cameraTexture"
        ),
        0
    );

    std::cout << "Camera background initialized\n";

    // --------------------------------------------------------
    // LOAD LAMP
    // --------------------------------------------------------

    if (!loadModel(MODEL_PATH))
    {
        glDeleteProgram(shaderProgram);
        glDeleteProgram(backgroundShaderProgram);

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    // --------------------------------------------------------
    // LAMP CAMERA
    // --------------------------------------------------------

    glm::vec3 cameraPosition(0.0f, 0.0f, 3.5f);

    glm::mat4 view = glm::lookAt(
        cameraPosition,
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(WIDTH) / HEIGHT,
        0.1f,
        100.0f
    );

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"),
        1,
        GL_FALSE,
        glm::value_ptr(view)
    );

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1,
        GL_FALSE,
        glm::value_ptr(projection)
    );

    glUniform3fv(
        glGetUniformLocation(shaderProgram, "camPos"),
        1,
        glm::value_ptr(cameraPosition)
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "baseColorMap"),
        0
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "metallicRoughnessMap"),
        1
    );

    glUniform1i(
        glGetUniformLocation(shaderProgram, "normalMap"),
        2
    );

    glUniform1f(
        glGetUniformLocation(shaderProgram, "metallicFactor"),
        1.0f
    );

    glUniform1f(
        glGetUniformLocation(shaderProgram, "roughnessFactor"),
        1.0f
    );

    // --------------------------------------------------------
    // UDP HAND TRACKING - PORT 5005
    // --------------------------------------------------------

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket < 0)
    {
        std::cerr << "Failed to create hand UDP socket\n";
        return -1;
    }

    sockaddr_in udpAddress{};

    udpAddress.sin_family = AF_INET;
    udpAddress.sin_port = htons(5005);
    udpAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(
        udpSocket,
        reinterpret_cast<sockaddr*>(&udpAddress),
        sizeof(udpAddress)
    ) < 0)
    {
        std::cerr << "Failed to bind port 5005\n";

        close(udpSocket);
        return -1;
    }

    int flags = fcntl(udpSocket, F_GETFL, 0);
    fcntl(udpSocket, F_SETFL, flags | O_NONBLOCK);

    int leftPresent = 0;
    int rightPresent = 0;

    float leftX = -1.0f;
    float leftY = -1.0f;

    float rightX = -1.0f;
    float rightY = -1.0f;

    // --------------------------------------------------------
    // UDP CAMERA RECEIVER - PORT 5006
    // --------------------------------------------------------

    int cameraSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (cameraSocket < 0)
    {
        std::cerr << "Failed to create camera UDP socket\n";

        close(udpSocket);
        return -1;
    }

    sockaddr_in cameraAddress{};

    cameraAddress.sin_family = AF_INET;
    cameraAddress.sin_port = htons(5006);
    cameraAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(
        cameraSocket,
        reinterpret_cast<sockaddr*>(&cameraAddress),
        sizeof(cameraAddress)
    ) < 0)
    {
        std::cerr << "Failed to bind camera port 5006\n";

        close(cameraSocket);
        close(udpSocket);
        return -1;
    }

    int cameraFlags = fcntl(cameraSocket, F_GETFL, 0);
    fcntl(cameraSocket, F_SETFL, cameraFlags | O_NONBLOCK);

    unsigned char cameraBuffer[65536];

    cv::Mat latestCameraFrame;

    int cameraTextureWidth = 0;
    int cameraTextureHeight = 0;

    std::cout << "Hand tracking receiver: port 5005\n";
    std::cout << "Camera receiver: port 5006\n";

    // --------------------------------------------------------
    // LAMP MOVEMENT
    // --------------------------------------------------------

    glm::vec3 lampPosition(0.0f);
    glm::vec3 targetPosition(0.0f);

    std::cout << "\nAR Renderer ready!\n";

    // ========================================================
    // RENDER LOOP
    // ========================================================

    while (!glfwWindowShouldClose(window))
    {
        // ----------------------------------------------------
        // RECEIVE CAMERA FRAMES
        // ----------------------------------------------------

        while (true)
        {
            ssize_t bytesReceived = recvfrom(
                cameraSocket,
                cameraBuffer,
                sizeof(cameraBuffer),
                0,
                nullptr,
                nullptr
            );

            if (bytesReceived <= 0)
                break;

            std::vector<unsigned char> jpegData(
                cameraBuffer,
                cameraBuffer + bytesReceived
            );

            cv::Mat decodedFrame = cv::imdecode(
                jpegData,
                cv::IMREAD_COLOR
            );

            if (!decodedFrame.empty())
            {
                latestCameraFrame = decodedFrame;
            }
        }

        // ----------------------------------------------------
        // UPDATE CAMERA TEXTURE
        // ----------------------------------------------------

        if (!latestCameraFrame.empty())
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cameraTexture);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            int width = latestCameraFrame.cols;
            int height = latestCameraFrame.rows;

            if (width != cameraTextureWidth ||
                height != cameraTextureHeight)
            {
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGB8,
                    width,
                    height,
                    0,
                    GL_BGR,
                    GL_UNSIGNED_BYTE,
                    latestCameraFrame.data
                );

                cameraTextureWidth = width;
                cameraTextureHeight = height;
            }
            else
            {
                glTexSubImage2D(
                    GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    width,
                    height,
                    GL_BGR,
                    GL_UNSIGNED_BYTE,
                    latestCameraFrame.data
                );
            }
        }

        // ----------------------------------------------------
        // RECEIVE HAND TRACKING DATA
        // ----------------------------------------------------

        char buffer[1024];

        while (true)
        {
            ssize_t bytesReceived = recvfrom(
                udpSocket,
                buffer,
                sizeof(buffer) - 1,
                0,
                nullptr,
                nullptr
            );

            if (bytesReceived <= 0)
                break;

            buffer[bytesReceived] = '\0';

            sscanf(
                buffer,
                "%d,%f,%f,%d,%f,%f",
                &leftPresent,
                &leftX,
                &leftY,
                &rightPresent,
                &rightX,
                &rightY
            );
        }

        // ----------------------------------------------------
        // LEFT HAND CONTROLS THE LAMP
        // ----------------------------------------------------

        if (leftPresent)
        {
            float x = (leftX - 0.5f) * 1.8f;
            float y = (0.5f - leftY) * 1.3f;

            targetPosition = glm::vec3(x, y, 0.0f);
        }

        lampPosition = glm::mix(
            lampPosition,
            targetPosition,
            0.15f
        );

        // ----------------------------------------------------
        // CLEAR WINDOW
        // ----------------------------------------------------

        glClearColor(
            0.035f,
            0.025f,
            0.045f,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        // ----------------------------------------------------
        // DRAW CAMERA BACKGROUND
        // ----------------------------------------------------

        if (!latestCameraFrame.empty())
        {
            glDisable(GL_DEPTH_TEST);

            glUseProgram(backgroundShaderProgram);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cameraTexture);

            glBindVertexArray(backgroundVAO);

            glDrawArrays(
                GL_TRIANGLES,
                0,
                6
            );

            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);
        }

        // ----------------------------------------------------
        // DRAW LAMP
        // ----------------------------------------------------

        glUseProgram(shaderProgram);

        glm::mat4 model(1.0f);

        model = glm::translate(
            model,
            lampPosition
        );

        model = glm::translate(
            model,
            -modelCenter * modelScale
        );

        model = glm::scale(
            model,
            glm::vec3(modelScale)
        );

        // No automatic rotation.
        // The lamp orientation stays fixed for now.

        glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "model"),
            1,
            GL_FALSE,
            glm::value_ptr(model)
        );

        drawModel(shaderProgram);

        // ----------------------------------------------------
        // PRESENT FRAME
        // ----------------------------------------------------

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ========================================================
    // CLEANUP
    // ========================================================

    std::cout << "\nShutting down AR renderer...\n";

    for (const Mesh& mesh : meshes)
    {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);

        if (mesh.baseColorTexture)
            glDeleteTextures(1, &mesh.baseColorTexture);

        if (mesh.metallicRoughnessTexture)
            glDeleteTextures(1, &mesh.metallicRoughnessTexture);

        if (mesh.normalTexture)
            glDeleteTextures(1, &mesh.normalTexture);
    }

    close(cameraSocket);
    close(udpSocket);

    glDeleteProgram(shaderProgram);
    glDeleteProgram(backgroundShaderProgram);

    glDeleteVertexArrays(1, &backgroundVAO);
    glDeleteBuffers(1, &backgroundVBO);
    glDeleteTextures(1, &cameraTexture);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
