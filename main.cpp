#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

struct Vec3 { float x, y, z; };
struct Tri3 { unsigned int v1, v2, v3; };

struct ObjModel {
    std::vector<Vec3> vertices;
    std::vector<Tri3> faces;
};

static bool loadOBJ_strict_v_f(const std::string& path, ObjModel& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Zadatak 1] Greska: ne mogu otvoriti datoteku: " << path << "\n";
        return false;
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(file, line)) {
        ++lineNo;
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "v") {
            Vec3 v{};
            if (!(iss >> v.x >> v.y >> v.z)) {
                std::cerr << "[Zadatak 1] Upozorenje: neispravna v linija u " << path << " (red " << lineNo << ")\n";
                continue;
            }
            out.vertices.push_back(v);
        }
        else if (tag == "f") {
            int a, b, c;
            if (!(iss >> a >> b >> c)) {
                std::cerr << "[Zadatak 1] Upozorenje: neispravna f linija (mora biti tri cijela indeksa) u " << path << " (red " << lineNo << ")\n";
                continue;
            }
            if (a <= 0 || b <= 0 || c <= 0) {
                std::cerr << "[Zadatak 1] Upozorenje: indeksi u .obj moraju biti 1-based pozitivni (red " << lineNo << ")\n";
                continue;
            }
            out.faces.push_back(Tri3{ (unsigned)(a - 1), (unsigned)(b - 1), (unsigned)(c - 1) });
        }
        else {
        }
    }

    std::cout << "=== ZADATAK 1: REZULTAT UCITAVANJA OBJ ===\n";
    std::cout << "Datoteka: " << path << "\n";
    std::cout << "Broj vrhova:   " << out.vertices.size() << "\n";
    std::cout << "Broj trokuta:  " << out.faces.size() << "\n\n";

    for (size_t i = 0; i < out.vertices.size(); ++i) {
        const auto& v = out.vertices[i];
        std::cout << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }
    for (size_t i = 0; i < out.faces.size(); ++i) {
        const auto& f = out.faces[i];
        std::cout << "f " << (f.v1 + 1) << " " << (f.v2 + 1) << " " << (f.v3 + 1) << "\n";
    }
    std::cout << "=== KRAJ ZADATKA 1 ===\n\n";

    return true;
}

static bool compileShader(GLuint sh, const char* src) {
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(sh, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
        return false;
    }
    return true;
}

static GLuint makeProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    if (!compileShader(vs, vsSrc)) { glDeleteShader(vs); return 0; }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    if (!compileShader(fs, fsSrc)) { glDeleteShader(vs); glDeleteShader(fs); return 0; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = GL_FALSE; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
        glDeleteProgram(prog);
        prog = 0;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

int main(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    ObjModel model;
    const std::string objPath = "triangle.obj";
    bool ok = loadOBJ_strict_v_f(objPath, model);
    if (!ok) {
        std::cerr << "[Zadatak 1] Ucitavanje nije uspjelo ili je bez podataka.\n";
    }

    std::vector<float> vertexData;
    vertexData.reserve(model.vertices.size() * 3);
    for (const auto& v : model.vertices) {
        vertexData.push_back(v.x);
        vertexData.push_back(v.y);
        vertexData.push_back(v.z);
    }

    std::vector<unsigned int> indices;
    indices.reserve(model.faces.size() * 3);
    for (const auto& f : model.faces) {
        indices.push_back(f.v1);
        indices.push_back(f.v2);
        indices.push_back(f.v3);
    }

    const char* vsSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    const char* fsSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.9, 0.7, 0.2, 1.0);
        }
    )";

    GLuint program = makeProgram(vsSrc, fsSrc);
    if (!program) {
        std::cerr << "Ne mogu kreirati shader program.\n";
        glfwTerminate();
        return -1;
    }

    GLuint VAO = 0, VBO = 0, EBO = 0;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertexData.size() * sizeof(float),
        vertexData.empty() ? nullptr : vertexData.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.empty() ? nullptr : indices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!indices.empty() && !vertexData.empty()) {
            glUseProgram(program);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
