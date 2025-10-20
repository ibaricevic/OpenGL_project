// main.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <filesystem>   // C++17
#include <cctype>

namespace fs = std::filesystem;

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

// === Podaci meša ===
struct Vec3 { float x{}, y{}, z{}; };
struct Triangle { uint32_t a{}, b{}, c{}; };
struct ObjMesh {
    std::vector<Vec3>     vertices;
    std::vector<Triangle> triangles;
};

// Parsiranje "vi/ti/ni" -> vi
static bool parseVertexIndexToken(const std::string& token, int& outIndex) {
    if (token.empty()) return false;
    size_t slash = token.find('/');
    std::string head = (slash == std::string::npos) ? token : token.substr(0, slash);
    try { outIndex = std::stoi(head); return true; }
    catch (...) { return false; }
}

// Trim helper
static inline std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// Učitavanje .obj (samo v i f)
bool loadObj(const std::string& path, ObjMesh& out) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "Ne mogu otvoriti OBJ datoteku: " << path << "\n";
        return false;
    }
    out.vertices.clear();
    out.triangles.clear();

    std::string line;
    size_t lineNo = 0;

    while (std::getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("g ", 0) == 0) continue;

        std::istringstream iss(line); // (ispravno: istringstream, ne istringstringstream)
        std::string tag; iss >> tag;

        if (tag == "v") {
            float x = 0.f, y = 0.f, z = 0.f;
            if (!(iss >> x >> y >> z)) {
                std::cerr << "Upozorenje: krivi format 'v' u liniji " << lineNo << "\n";
                continue;
            }
            out.vertices.push_back({ x,y,z });
        }
        else if (tag == "f") {
            std::string t1, t2, t3;
            if (!(iss >> t1 >> t2 >> t3)) {
                std::cerr << "Upozorenje: krivi format 'f' u liniji " << lineNo << "\n";
                continue;
            }
            int i1 = 0, i2 = 0, i3 = 0;
            if (!parseVertexIndexToken(t1, i1) || !parseVertexIndexToken(t2, i2) || !parseVertexIndexToken(t3, i3)) {
                std::cerr << "Upozorenje: krivi index u 'f' liniji " << lineNo << "\n";
                continue;
            }
            auto fixIndex = [&](int idx)->uint32_t {
                if (idx > 0) {
                    if (static_cast<size_t>(idx) <= out.vertices.size())
                        return static_cast<uint32_t>(idx - 1);
                    return static_cast<uint32_t>(out.vertices.size() - 1);
                }
                size_t N = out.vertices.size();
                size_t real = (idx == 0) ? 0 : (N + idx);
                if (real >= N) real = N - 1;
                return static_cast<uint32_t>(real);
                };
            out.triangles.push_back({ fixIndex(i1), fixIndex(i2), fixIndex(i3) });
        }
    }
    return true;
}

// === GLFW util ===
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// --- izbor datoteke iz podmape "objects" uz main.cpp ---
static const char* MODELS_SUBDIR = "objects";

static std::string toLower(std::string s) {
    for (auto& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}
static std::vector<fs::path> listObjFiles(const fs::path& dir) {
    std::vector<fs::path> out;
    if (!fs::exists(dir) || !fs::is_directory(dir)) return out;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        auto ext = toLower(entry.path().extension().string());
        if (ext == ".obj") out.push_back(entry.path());
    }
    std::sort(out.begin(), out.end(), [](const fs::path& a, const fs::path& b) {
        return a.filename().string() < b.filename().string();
        });
    return out;
}

// === Shader helperi ===
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::cerr << "Shader compile error: " << log << "\n";
    }
    return s;
}
static GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::cerr << "Program link error: " << log << "\n";
    }
    glDetachShader(p, vs);
    glDetachShader(p, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

// === GPU bufferi ===
struct GLMesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei indexCount = 0;
};
static GLMesh createMeshBuffers(const ObjMesh& mesh) {
    GLMesh gm{};
    // linearni niz pozicija
    std::vector<float> positions; positions.reserve(mesh.vertices.size() * 3);
    for (auto& v : mesh.vertices) { positions.push_back(v.x); positions.push_back(v.y); positions.push_back(v.z); }
    // linearni niz indeksa
    std::vector<uint32_t> indices; indices.reserve(mesh.triangles.size() * 3);
    for (auto& t : mesh.triangles) { indices.push_back(t.a); indices.push_back(t.b); indices.push_back(t.c); }

    glGenVertexArrays(1, &gm.vao);
    glGenBuffers(1, &gm.vbo);
    glGenBuffers(1, &gm.ebo);

    glBindVertexArray(gm.vao);

    glBindBuffer(GL_ARRAY_BUFFER, gm.vbo);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // layout(location=0) vec3 aPos;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    gm.indexCount = static_cast<GLsizei>(indices.size());
    return gm;
}

// AABB i normalizacija
struct Bounds { Vec3 min, max; };
static Bounds computeBounds(const ObjMesh& m) {
    Bounds b{};
    if (m.vertices.empty()) return b;
    b.min = b.max = m.vertices[0];
    for (auto& v : m.vertices) {
        b.min.x = std::min(b.min.x, v.x); b.max.x = std::max(b.max.x, v.x);
        b.min.y = std::min(b.min.y, v.y); b.max.y = std::max(b.max.y, v.y);
        b.min.z = std::min(b.min.z, v.z); b.max.z = std::max(b.max.z, v.z);
    }
    return b;
}
static Vec3 centerOf(const Bounds& b) {
    return Vec3{ (b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f, (b.min.z + b.max.z) * 0.5f };
}
static float uniformScaleToUnit(const Bounds& b, float target = 1.6f) {
    // target < 2.0 da ostane mala margina u [-1,1] clip prostoru
    float dx = b.max.x - b.min.x;
    float dy = b.max.y - b.min.y;
    float dz = b.max.z - b.min.z;
    float maxd = std::max(dx, std::max(dy, dz));
    if (maxd <= 0.0f) return 1.0f;
    return (target / maxd);
}

// === Shaders ===
// Statičan 2D vertex shader (XY) s aspect kompenzacijom
static const char* kVert = R"(#version 330 core
layout(location=0) in vec3 aPos;
uniform vec3  uCenter;   // centar AABB-a
uniform float uScale;    // uniformni faktor skaliranja
uniform float uAspect;   // HEIGHT / WIDTH
void main(){
    // Uzmemo samo XY, centriramo i skaliramo
    vec2 p = (aPos.xy - uCenter.xy) * uScale;

    // Kompenzacija aspect ratio-a prozora (širi prozor bi istegnuo po X):
    p.x *= uAspect;

    gl_Position = vec4(p, 0.0, 1.0); // čisto 2D (Z=0)
}
)";

static const char* kFrag = R"(#version 330 core
out vec4 FragColor;
void main(){
    FragColor = vec4(0.95, 0.95, 0.98, 1.0); // svijetla boja
}
)";

// Global toggles
static bool g_wireframe = false;

// --- MAIN ---
int main()
{
    // 0) Mapa s objektima uz main.cpp
    const fs::path SRC_DIR = fs::path(__FILE__).parent_path();
    const fs::path MODELS_DIR = SRC_DIR / MODELS_SUBDIR;

    // 1) Izbor datoteke
    auto files = listObjFiles(MODELS_DIR);
    if (files.empty()) {
        std::cerr << "Nema .obj datoteka u mapi: " << MODELS_DIR.string() << "\n";
        std::cerr << "Kreiraj mapu \"" << MODELS_SUBDIR << "\" uz main.cpp i stavi tamo .obj fajlove.\n";
        return -1;
    }
    std::cout << "Pronadjene .obj datoteke u \"" << MODELS_DIR.string() << "\":\n";
    for (size_t i = 0; i < files.size(); ++i)
        std::cout << "  [" << i + 1 << "] " << files[i].filename().string() << "\n";
    std::cout << "\nUpisi IME datoteke ili REDNI BROJ za ucitavanje: ";
    std::string choice; std::getline(std::cin, choice); choice = trim(choice);

    // sigurnije provjeravanje je li unos broj
    auto is_all_digits = [](const std::string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(),
            [](unsigned char c) { return std::isdigit(c) != 0; });
        };

    fs::path chosenPath;
    if (is_all_digits(choice)) {
        size_t idx = static_cast<size_t>(std::stoul(choice));
        if (idx == 0 || idx > files.size()) { std::cerr << "Neispravan indeks.\n"; return -1; }
        chosenPath = files[idx - 1];
    }
    else {
        fs::path byName = MODELS_DIR / choice;
        if (!(fs::exists(byName) && fs::is_regular_file(byName))) {
            byName = MODELS_DIR / (choice + std::string(".obj"));
            if (!(fs::exists(byName) && fs::is_regular_file(byName))) {
                std::cerr << "Datoteka \"" << choice << "\" nije pronadjena u " << MODELS_DIR.string() << "\n";
                return -1;
            }
        }
        chosenPath = byName;
    }

    // 2) Učitavanje OBJ-a
    ObjMesh mesh;
    if (!loadObj(chosenPath.string(), mesh)) {
        std::cerr << "Neuspjelo ucitavanje: " << chosenPath << "\n";
        return -1;
    }
    std::cout << "Ucitano: " << chosenPath.filename().string()
        << " | vrhova: " << mesh.vertices.size()
        << " | trokuta: " << mesh.triangles.size() << "\n";

    // 3) OpenGL init
    if (!glfwInit()) { std::cerr << "GLFW init fail\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OBJ Viewer 2D", NULL, NULL);
    if (!window) { std::cerr << "GLFW window fail\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init fail\n"; glfwTerminate(); return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glDisable(GL_DEPTH_TEST); // 2D, bez depth-a

    // 4) Shaders + bufferi
    GLuint vs = compileShader(GL_VERTEX_SHADER, kVert);
    GLuint fs_ = compileShader(GL_FRAGMENT_SHADER, kFrag);
    GLuint prog = linkProgram(vs, fs_);
    GLint locCenter = glGetUniformLocation(prog, "uCenter");
    GLint locScale = glGetUniformLocation(prog, "uScale");
    GLint locAspect = glGetUniformLocation(prog, "uAspect");

    GLMesh glmesh = createMeshBuffers(mesh);

    // 5) Normalizacija (centriranje + uniform scale)
    Bounds b = computeBounds(mesh);
    Vec3 center = centerOf(b);
    float scale = uniformScaleToUnit(b, 1.6f);

    // 6) Petlja crtanja (statično 2D)
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.12f, 0.14f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPolygonMode(GL_FRONT_AND_BACK, g_wireframe ? GL_LINE : GL_FILL);

        glUseProgram(prog);
        glUniform3f(locCenter, center.x, center.y, center.z);
        glUniform1f(locScale, scale);
        glUniform1f(locAspect, float(HEIGHT) / float(WIDTH)); // važna korekcija

        glBindVertexArray(glmesh.vao);
        glDrawElements(GL_TRIANGLES, glmesh.indexCount, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &glmesh.vao);
    glDeleteBuffers(1, &glmesh.vbo);
    glDeleteBuffers(1, &glmesh.ebo);
    glDeleteProgram(prog);

    glfwTerminate();
    return 0;
}

// === Callbacks & input ===
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_wireframe = true;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE)
        g_wireframe = false;
}
