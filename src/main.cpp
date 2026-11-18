#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// Window resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Read shader files
std::string loadShader(const char* path)
{
    std::ifstream file(path);
    std::stringstream buffer;

    if (!file.is_open())
    {
        std::cerr << "Failed to open shader: " << path << '\n';
        return "";
    }

    buffer << file.rdbuf();
    return buffer.str();
}

// Compile a shader and check for errors
unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << '\n';
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// Create shader program
unsigned int createShaderProgram()
{
    std::string vertexCode = loadShader("shaders/vertex.glsl");
    std::string fragmentCode = loadShader("shaders/fragment.glsl");

    if (vertexCode.empty() || fragmentCode.empty())
        return 0;

    unsigned int vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexCode.c_str());

    unsigned int fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());

    if (!vertexShader || !fragmentShader)
    {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return 0;
    }

    unsigned int program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed:\n" << infoLog << '\n';

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1000, 700,
        "The Wish Gone Wrong - 3D Cube",
        nullptr, nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, 1000, 700);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Cube vertices
    float vertices[] = {
        // Front and back corners
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f
    };

    // Cube face indices
    unsigned int indices[] = {
        0, 1, 2,  2, 3, 0, // Back
        4, 5, 6,  6, 7, 4, // Front
        0, 4, 7,  7, 3, 0, // Left
        1, 5, 6,  6, 2, 1, // Right
        3, 2, 6,  6, 7, 3, // Top
        0, 1, 5,  5, 4, 0  // Bottom
    };

    unsigned int VAO, VBO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), (void*)0
    );

    glEnableVertexAttribArray(0);

    unsigned int shaderProgram = createShaderProgram();

    if (!shaderProgram)
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Get uniform locations
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    // Camera position
    glm::mat4 view = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(0.0f, 0.0f, -3.0f)
    );

    // Perspective projection
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        1000.0f / 700.0f,
        0.1f,
        100.0f
    );

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(
        viewLoc, 1, GL_FALSE, glm::value_ptr(view)
    );

    glUniformMatrix4fv(
        projectionLoc, 1, GL_FALSE, glm::value_ptr(projection)
    );

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.08f, 0.04f, 0.15f, 1.0f);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float time = glfwGetTime();

        // Rotate the cube
        glm::mat4 model = glm::rotate(
            glm::mat4(1.0f),
            time,
            glm::vec3(0.5f, 1.0f, 0.0f)
        );

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(
            modelLoc, 1, GL_FALSE, glm::value_ptr(model)
        );

        glBindVertexArray(VAO);

        glDrawElements(
            GL_TRIANGLES,
            36,
            GL_UNSIGNED_INT,
            0
        );

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
