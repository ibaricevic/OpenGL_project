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

// === Jednostavne strukture za OBJ podatke ===
struct Vec3 { float x{}, y{}, z{}; };
struct Triangle { uint32_t a{}, b{}, c{}; };
struct ObjMesh {
    std::vector<Vec3>     vertices;
    std::vector<Triangle> triangles;
};

// --- Parsiranje "vi/ti/ni" tokena: uzimamo samo prvu (vertex) komponentu ---
static bool parseVertexIndexToken(const std::string& token, int& outIndex) {
    if (token.empty()) return false;
    size_t slash = token.find('/');
    std::string head = (slash == std::string::npos) ? token : token.substr(0, slash);
    try { outIndex = std::stoi(head); return true; }
    catch (...) { return false; }
}

// --- Trim helper ---
static inline std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// --- Učitavanje .obj s podrškom za "v" i "f" (trokute) ---
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

        std::istringstream iss(line);
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

// === GLFW utilities ===
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// --- Podešavanje mape s .obj datotekama ---
// Naziv podmape koja se nalazi U ISTOM direktoriju kao i main.cpp:
static const char* MODELS_SUBDIR = "objects";

// mala pomoćna: lower-case string (za provjeru ekstenzije)
static std::string toLower(std::string s) {
    for (auto& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}

// Vraća listu .obj datoteka u zadanoj mapi
static std::vector<fs::path> listObjFiles(const fs::path& dir) {
    std::vector<fs::path> out;
    if (!fs::exists(dir) || !fs::is_directory(dir)) return out;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        auto ext = toLower(entry.path().extension().string());
        if (ext == ".obj") out.push_back(entry.path());
    }
    // deterministic order by filename
    std::sort(out.begin(), out.end(), [](const fs::path& a, const fs::path& b) {
        return a.filename().string() < b.filename().string();
        });
    return out;
}

int main()
{
    // 0) Izračun stvarne lokacije mape "models" – relativno na main.cpp
    const fs::path SRC_DIR = fs::path(__FILE__).parent_path();
    const fs::path MODELS_DIR = SRC_DIR / MODELS_SUBDIR;

    // 1) Prikaži sve .obj datoteke i pitaj korisnika
    auto files = listObjFiles(MODELS_DIR);
    if (files.empty()) {
        std::cerr << "Nema .obj datoteka u mapi: " << MODELS_DIR.string() << "\n";
        std::cerr << "Kreiraj mapu \"" << MODELS_SUBDIR << "\" uz main.cpp i stavi tamo .obj fajlove.\n";
        return -1;
    }

    std::cout << "Pronadjene .obj datoteke u \"" << MODELS_DIR.string() << "\":\n";
    for (size_t i = 0; i < files.size(); ++i) {
        std::cout << "  [" << i + 1 << "] " << files[i].filename().string() << "\n";
    }
    std::cout << "\nUpisi IME datoteke ili REDNI BROJ za ucitavanje: ";

    std::string choice;
    std::getline(std::cin, choice);
    choice = trim(choice);

    fs::path chosenPath;
    if (!choice.empty() && std::all_of(choice.begin(), choice.end(), ::isdigit)) {
        // korisnik je unio broj (1-based)
        size_t idx = static_cast<size_t>(std::stoul(choice));
        if (idx == 0 || idx > files.size()) {
            std::cerr << "Neispravan indeks.\n";
            return -1;
        }
        chosenPath = files[idx - 1];
    }
    else {
        // korisnik je unio ime datoteke
        fs::path byName = MODELS_DIR / choice;
        if (fs::exists(byName) && fs::is_regular_file(byName)) {
            chosenPath = byName;
        }
        else {
            // pokušaj dodati .obj ako ga je izostavio
            byName = MODELS_DIR / (choice + ".obj");
            if (fs::exists(byName) && fs::is_regular_file(byName)) {
                chosenPath = byName;
            }
            else {
                std::cerr << "Datoteka \"" << choice << "\" nije pronadjena u " << MODELS_DIR.string() << "\n";
                return -1;
            }
        }
    }

    // 2) Učitaj odabrani OBJ (prije OpenGL-a)
    ObjMesh mesh;
    if (loadObj(chosenPath.string(), mesh)) {
        std::cout << "\nUcitana OBJ datoteka: " << chosenPath.filename().string() << "\n";
        std::cout << "Broj vrhova:   " << mesh.vertices.size() << "\n";
        std::cout << "Broj trokuta:  " << mesh.triangles.size() << "\n\n";

        std::cout << "# Lista vrhova (prvih do 5):\n";
        for (size_t i = 0; i < std::min<size_t>(mesh.vertices.size(), 5); ++i) {
            const auto& v = mesh.vertices[i];
            std::cout << "v " << v.x << " " << v.y << " " << v.z << "\n";
        }
        std::cout << "# Lista trokuta (prvih do 5):\n";
        for (size_t i = 0; i < std::min<size_t>(mesh.triangles.size(), 5); ++i) {
            const auto& f = mesh.triangles[i];
            std::cout << "f " << (f.a + 1) << " " << (f.b + 1) << " " << (f.c + 1) << "\n";
        }
        std::cout << std::endl;
    }
    else {
        std::cerr << "Neuspjelo ucitavanje OBJ-a: " << chosenPath.string() << "\n";
        return -1;
    }

    // 3) Standardna OpenGL/GLFW inicijalizacija
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // za macOS ako treba

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OBJ Loader - Odabir iz mape", NULL, NULL);
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
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
