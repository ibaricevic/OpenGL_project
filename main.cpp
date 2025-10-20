// Priprema za Lab2
// Zadatak 1: citanje .obj (samo "v x y z" i "f v1 v2 v3") + ispis u konzolu.
// Zadatak 2: iscrtavanje trokuta preko OpenGL-a (GLFW + GLAD + shaderi + VAO/VBO/EBO).

#include <glad/glad.h>      // sadrži funkcije OpenGL-a
#include <GLFW/glfw3.h>     // biblioteka za stvaranje prozora, upravljanje tipkovnicom i mišem

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


// Globalne postavke prozora --> širina i visina prozora (u px) koji se koriste pri stvaranju OpenGL prozora
const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);      // funkcija koja se poziva kad se promijeni veličina prozora
void processInput(GLFWwindow* window);      // funkcija za obradu unosa s tipkovnice


// ----------------------------
// ZADATAK 1: MODEL I UČITAVANJE
// ----------------------------
struct Vec3 { float x, y, z; };     //  predstavlja jedan vrh (x, y, z koordinate)
struct Tri3 { unsigned int v1, v2, v3; }; // predstavlja jedan trokut (indeksi tri vrha --> 0-based u memoriji)

// kompletan model koji sadrži sve vrhove i trokute iz jedne .obj datoteke
struct ObjModel {
	std::vector<Vec3> vertices; // v x y z --> lista vrhova
	std::vector<Tri3> faces;    // f v1 v2 v3 (1-based u .obj -> 0-based ovdje) --> lista trokuta
};

// Ucitava iskljucivo linije "v x y z" i "f v1 v2 v3" (bez tekstura/normali i bez slash formata).
// Ako imas .obj s dodatnim stvarima, one se ignoriraju; "f" MORA biti tocno tri indeksa.
static bool loadOBJ_strict_v_f(const std::string& path, ObjModel& out) {    // prima putanju do .obj datoteke i referencu na ObjModel u koji sprema podatke
	                                                                        // vraca true ako je uspjelo ucitavanje, false inace
	std::ifstream file(path);   // otvaranje datoteke za čitanje
	if (!file.is_open()) {      // provjera je li datoteka uspješno otvorena
        std::cerr << "[Zadatak 1] Greska: ne mogu otvoriti datoteku: " << path << "\n";
        return false;
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(file, line)) {  // čita jednu po jednu liniju iz datoteke dok ne dođe do kraja
        ++lineNo;
        if (line.empty()) continue;     // preskače znakove koji su prazni
		if (line[0] == '#') continue;   // preskače znakove koji započinju s '#'

		std::istringstream iss(line);   // koristi string stream za parsiranje linije
                                        // napravi objekt koji će čitati iz stringa (teksta) kao da je to datoteka ili tipkovnica
                                        // ona omogućuje da se čitaju podaci iz stringa pomoću operatora >>
        std::string tag;                // deklarira se varijabla tag tipa std::string, koja će sadržavati prvu riječ u liniji
		iss >> tag;                     // operator >> čita prvi komad teksta do razmaka i sprema ga u varijablu tag

        if (tag == "v") {
            // ocekivano: v x y z --> ako linija počinje s "v", čitaju se tri broja x, y, z
			Vec3 v{};       // inicijalizira se novi objekt tipa Vec3 za pohranu koordinata vrha
			if (!(iss >> v.x >> v.y >> v.z)) {  //  pokušavaju se pročitati tri float broja iz linije
                std::cerr << "[Zadatak 1] Upozorenje: neispravna v linija u " << path << " (red " << lineNo << ")\n";
                continue;
            }
            out.vertices.push_back(v);  // podaci se spremaju kao novi element u out.vertices
        }
        else if (tag == "f") {
            // ocekivano: f v1 v2 v3 (tocno tri indeksa tj. cijela broja, bez slash-a)
            int a, b, c;
			if (!(iss >> a >> b >> c)) {    // pokušavaju se pročitati tri cijela broja iz linije
                std::cerr << "[Zadatak 1] Upozorenje: neispravna f linija (mora biti tri cijela indeksa) u " << path << " (red " << lineNo << ")\n";
                continue;
            }
			if (a <= 0 || b <= 0 || c <= 0) {   // provjera jesu li indeksi pozitivni
                std::cerr << "[Zadatak 1] Upozorenje: indeksi u .obj moraju biti 1-based pozitivni (red " << lineNo << ")\n";
                continue;
            }
            // u memoriji cuvamo 0-based
            // .obj datoteke broje vrhove od 1, a u C++ indeksiranje počinje od 0, pa se od svakog indeksa oduzima 1
			out.faces.push_back(Tri3{ (unsigned)(a - 1), (unsigned)(b - 1), (unsigned)(c - 1) });   // sprema se novi trokut u out.faces
        }
        else {
            // sve ostalo ignoriramo po zadatku
        }
    }

    // ISPIS REZULTATA ZADATAK 1
    std::cout << "=== ZADATAK 1: REZULTAT UCITAVANJA OBJ ===\n";
    std::cout << "Datoteka: " << path << "\n";
    std::cout << "Broj vrhova:   " << out.vertices.size() << "\n";
    std::cout << "Broj trokuta:  " << out.faces.size() << "\n\n";

	// ispis svih vrhova i trokuta u isti format kao u .obj (radi provjere ispravnosti ucitavanja)
    for (size_t i = 0; i < out.vertices.size(); ++i) {
        const auto& v = out.vertices[i];
        std::cout << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }
    for (size_t i = 0; i < out.faces.size(); ++i) {
        const auto& f = out.faces[i];
        // ispisi opet u 1-based formatu radi provjere
        std::cout << "f " << (f.v1 + 1) << " " << (f.v2 + 1) << " " << (f.v3 + 1) << "\n";
    }
    std::cout << "=== KRAJ ZADATKA 1 ===\n\n";

    return true;
}

// ----------------------------
// Shader helperi (Zadatak 2)
// ----------------------------
// ove funkcije kompiliraju vertex i fragment shadere(#version 330 core, GLSL jezik), ispisuju greške ako ih ima, vraćaju OpenGL ID programa koji se kasnije koristi za crtanje
// vertex shader – prima poziciju vrha(aPos) i šalje je u GPU pipeline
// fragment shader – određuje boju svakog piksela trokuta

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
    // ----------------------------
    // originalni kod GLFW/GLAD INIT
    // ----------------------------
    // pokreće GLFW i određuje da se koristi OpenGL verzija 3.3 Core.
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // za macOS

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", NULL, NULL);    // stvara prozor dimenzija WIDTH x HEIGHT
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

	glfwMakeContextCurrent(window);     // postavlja stvoreni prozor kao trenutni kontekst za OpenGL naredbe
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);      //  registrira funkciju koja se poziva kad se promijeni veličina prozora

	// učitava sve OpenGL funkcije preko GLAD-a
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    // ----------------------------
    // ZADATAK 1: UČITAJ OBJ I ISPIŠI
    // ----------------------------
	ObjModel model; // objekt u koji ćemo učitati model
    const std::string objPath = "triangle.obj"; // promijeni po potrebi (stavi datoteku pokraj .exe)
    bool ok = loadOBJ_strict_v_f(objPath, model);  //   učitava .obj datoteku u model
	// ispisuje upozorenje ako učitavanje nije uspjelo ili je bez podataka
    if (!ok) {
        std::cerr << "[Zadatak 1] Ucitavanje nije uspjelo ili je bez podataka.\n";
        // nastavlja dalje (prozor ce se otvoriti, ali se nece crtati ako nema podataka)
    }

    // ----------------------------
    // ZADATAK 2: PRIPREMA PODATAKA ZA RENDER
    // ----------------------------
    // flatten vertices u float niz (x y z ...)
    // pretvaranje u nizove za OpenGL --> OpenGL očekuje da su podaci o vrhovima složeni u jedan kontinuirani niz (x, y, z, x, y, z ...), pa se vector<Vec3> pretvara u vector<float>
    std::vector<float> vertexData;
    vertexData.reserve(model.vertices.size() * 3);
    for (const auto& v : model.vertices) {
        vertexData.push_back(v.x);
        vertexData.push_back(v.y);
        vertexData.push_back(v.z);
    }
    // indeksi za EBO
    std::vector<unsigned int> indices;
    indices.reserve(model.faces.size() * 3);
    for (const auto& f : model.faces) {
        indices.push_back(f.v1);
        indices.push_back(f.v2);
        indices.push_back(f.v3);
    }

    // Shaderi
    const char* vsSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0); // ocekuje se da su koordinate vec u clip-spaceu [-1,1]
        }
    )";

    const char* fsSrc = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.9, 0.7, 0.2, 1.0);
        }
    )";

    // kompajlira i povezuje dva shader program
    // program je ID koji OpenGL koristi da zna koje shadere izvršava
    GLuint program = makeProgram(vsSrc, fsSrc);
    if (!program) {
        std::cerr << "Ne mogu kreirati shader program.\n";
        glfwTerminate();
        return -1;
    }

    // VAO (Vertex Array Object) – pamti konfiguraciju vrhova
    // VBO (Vertex Buffer Object) – čuva stvarne koordinate vrhova
    // EBO (Element Buffer Object) – čuva indekse trokuta (koji vrhovi čine koji trokut)
    GLuint VAO = 0, VBO = 0, EBO = 0;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // povezivanje podataka
    glBindVertexArray(VAO);
    // u GL_ARRAY_BUFFER šalje sve vrhove (x, y, z)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertexData.size() * sizeof(float),
        vertexData.empty() ? nullptr : vertexData.data(),
        GL_STATIC_DRAW);

	// u GL_ELEMENT_ARRAY_BUFFER šalje indekse trokuta (kojim redoslijedom ih treba crtati)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.empty() ? nullptr : indices.data(),
        GL_STATIC_DRAW);

    // konfiguracija atributa vrhova
    // “Atribut broj 0” ima 3 vrijednosti tipa float (x, y, z) zaredom --> to omogućuje GPU-u da zna kako čitati podatke iz memorije
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // ----------------------------
    // originalni kod --> RENDER PETLJA — dopunjeno crtanjem
    // ----------------------------
	while (!glfwWindowShouldClose(window))  // glavna petlja koja se vrti dok se prozor ne zatvori
    {
		processInput(window);               // provjerava je li pritisnuta tipka Escape

        // briše se stari sadržaj prozora i postavlja boju pozadine
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

		// crtanje modela ako ima podataka
        if (!indices.empty() && !vertexData.empty()) {
			glUseProgram(program);  // aktivira shader program
			glBindVertexArray(VAO); // aktivira konfiguraciju vrhova
			glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0); // crta trokute koristeći indekse iz EBO
			glBindVertexArray(0);   // deaktivira VAO nakon crtanja
        }

		// zamjena prednjeg i stražnjeg bafera te provjera događaja (tipkovnica, miš, prozor)
		glfwSwapBuffers(window);    // zamjenjuje prednji i stražnji buffer (prikazuje novi frame na ekranu)
		glfwPollEvents();           // provjerava događaje (tipkovnica, miš, prozor) 
    }

    // ciscenje memorije
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(program);

    glfwTerminate();    // gasi GLFW i zatvara sve prozore
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)   // funkcija koja se poziva kad se promijeni veličina prozora
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)   // funkcija koja provjerava je li pritisnuta tipka Escape, ako DA onda se zatvara prozor
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}