#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define LOG(logLevel, message) std::cout << #logLevel << ": " << message << std::endl
#define PRECISION 2000

enum LogLevel
{
    INFO,
    DEBUG,
    CRITICAL
};

// STRUCT / CLASS DECLARATIONS
struct Vertex
{
    glm::vec2 position;
};

struct Point
{
    glm::vec2 xy;
};

struct Line
{
    Vertex p1;
    Vertex p2;
};

// FUCNTION DECLARATIONS
void            GLFWKeyCallback(    GLFWwindow* window, int key, int scancode, int action, int mods);
void            GLFWScrollCallback( GLFWwindow* window, double xoffset, double yoffset);
void            FrameKeyCallback(   GLFWwindow* window);
unsigned int    CreateShaderProgram(const std::string& shader_name);
void            SetUniformMat4(     unsigned int program, const char* name, const glm::mat4& mat);
void            SetUniformVec4(     unsigned int program, const char* name, const glm::vec4& vec);          
void            BindShaderProgram(  unsigned int program);
void            BindVertexArray(    unsigned int vao);
size_t          PushData(           size_t count, Vertex* data, size_t& offset);
float           Normalize(           float x) { return x / fabs(x); }
// Function
constexpr float Function(float x);
float GetFunctionExtremum(float(*func)(float), float left, float right);

std::ostream& operator<<(std::ostream& stream, const glm::vec3& vec)
{
    stream << vec.x << ", " << vec.y << ", " << vec.z;
    return stream;
}

// GLOBAL VARIABLES
GLFWwindow* Window = nullptr;
int Width = 1280;
int Height = 720;

float Aspect_Ratio = static_cast<float>(Width) / Height;

constexpr float Left_Border = -8.0f;
constexpr float Right_Border = 2.0f; 
constexpr float Fov = 45.0f;
constexpr size_t Data_Size = PRECISION + 256;

float internal_height = GetFunctionExtremum(Function, Left_Border, Right_Border);
float Magnification = internal_height / tan(glm::radians(Fov / 2.0f)) + 0.1f;

float X_Offset = (Left_Border + Right_Border) / 2.0f;
float Y_Offset = 0.0f;

int main()
{
    if (!glfwInit())
    {
        LOG(CRITICAL, "Couldn't initialize GLFW");
        return -1;
    }

    Window = glfwCreateWindow(Width, Height, "Lab 17 AP", nullptr, nullptr);
    if (!Window)
    {
        LOG(CRITICAL, "Couldn't create a window");
        return -1;
    }

    // GLFW AND OPENGL CONTEXT STUFF
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwMakeContextCurrent(Window);

    if (glewInit() != GLEW_OK)
    {
        LOG(CRITICAL, "Couldn't initialize GLEW");
        return -1;
    }

    // GLFW CALLBACKS & STUFF
    glfwSetScrollCallback(Window, GLFWScrollCallback);
    glfwSetKeyCallback(Window, GLFWKeyCallback);
    glfwSwapInterval(1);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::array<Vertex, PRECISION> graph_array;
    // float x = Left_Border;
    float delta = (fabs(Left_Border) + fabs(Right_Border)) / graph_array.size();
    std::generate(graph_array.begin(), graph_array.end(), [&, x = Left_Border]() mutable -> Vertex {
        float y = Function(x);
        x += delta;
        return {glm::vec2(x, y)};
    });

    // OPENGL DATA
    unsigned int vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    unsigned int vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * Data_Size, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

    unsigned int basic_shader = CreateShaderProgram("BasicShader");

    glm::vec3 camera_position(0.0f, 0.0f, Magnification);
    glm::vec3 look_position(0.0f);
    glm::vec3 plot_origin(0.0f);
    glm::vec3 camera_direction = look_position - camera_position;
    glm::vec3 to_origin = plot_origin - camera_position;

    glm::mat4 projection = glm::perspective(glm::radians(Fov), Aspect_Ratio, 0.1f, 1000.0f);
    glm::mat4 view(1.0f);
    glm::mat4 model(1.0f);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    Line x = { glm::vec2(-1.0f, 0.0f), glm::vec2(1.0f, 0.0f) };
    Line y = { glm::vec2(0.0f, 1.0), glm::vec2(0.0f, -1.0f) };
    glm::vec<2, double> cursor_pos;

    // RENDER LOOP
    while (!glfwWindowShouldClose(Window))
    {
        // Clear color to draw next frame without artifacts from the previous
        // Main render stuff starts here
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

        FrameKeyCallback(Window);

        glfwGetCursorPos(Window, &cursor_pos.x, &cursor_pos.y);

        BindVertexArray(vao);
        BindShaderProgram(basic_shader);

        size_t offset = 0;
        size_t prev_offset;
        view = glm::lookAt(camera_position, look_position, glm::vec3(0.0f, 1.0f, 0.0f));

        float scale_y = camera_position.z * tan(glm::radians(Fov / 2.0f));
        float scale_x = scale_y * Aspect_Ratio;

        prev_offset = PushData(graph_array.size(), graph_array.data(), offset);
        model = glm::mat4(1.0f);
        SetUniformMat4(basic_shader, "uModel", model);
        SetUniformMat4(basic_shader, "uView", view);
        SetUniformMat4(basic_shader, "uProjection", projection);
        SetUniformVec4(basic_shader, "uColor", glm::vec4(0.5f, 1.0f, 0.4f, 1.0f));
        glDrawArrays(GL_LINE_STRIP, prev_offset, graph_array.size());

        prev_offset = PushData(2, reinterpret_cast<Vertex*>(&x), offset);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(camera_position.x, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(scale_x, 1.0f, 1.0f));
        SetUniformMat4(basic_shader, "uModel", model);
        SetUniformVec4(basic_shader, "uColor", glm::vec4(0.7f));
        glDrawArrays(GL_LINES, prev_offset, 2);

        prev_offset = PushData(2, reinterpret_cast<Vertex*>(&y), offset);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, camera_position.y, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, scale_y, 1.0f));
        SetUniformMat4(basic_shader, "uModel", model);
        glDrawArrays(GL_LINES, prev_offset, 2);

        Line line = {glm::vec2(1.0f, 0.1f), glm::vec2(1.0f, -0.1f)};
        float current_position_x = plot_origin.x;
        while (current_position_x < camera_position.x + scale_x)
        {
            prev_offset = PushData(2, reinterpret_cast<Vertex*>(&line), offset);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(current_position_x, 0.0f, 0.0f));
            if (fabs(camera_position.y) > scale_y)
            {
                const float& camera_y = camera_position.y;
                model = glm::translate(model, glm::vec3(0.0f, camera_y - Normalize(camera_y) * scale_y, 0.0f));
            }
            SetUniformMat4(basic_shader, "uModel", model);
            
            current_position_x += 1.0f;
            glDrawArrays(GL_LINES, prev_offset, 2);
        }

        line = {glm::vec2(-1.0f, 0.1f), glm::vec2(-1.0f, -0.1f)};
        current_position_x = plot_origin.x;
        while (current_position_x > camera_position.x - scale_x)
        {
            prev_offset = PushData(2, reinterpret_cast<Vertex*>(&line), offset);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(current_position_x, 0.0f, 0.0f));
            if (fabs(camera_position.y) > scale_y)
            {
                const float& camera_y = camera_position.y;
                model = glm::translate(model, glm::vec3(0.0f, camera_y - Normalize(camera_y) * scale_y, 0.0f));
            }
            SetUniformMat4(basic_shader, "uModel", model);

            current_position_x -= 1.0f;
            glDrawArrays(GL_LINES, prev_offset, 2);
        }

        line = {glm::vec2(0.1f, 1.0f), glm::vec2(-0.1f, 1.0f)};
        float current_position_y = plot_origin.y;
        while (current_position_y < camera_position.y + scale_y)
        {
            prev_offset = PushData(2, reinterpret_cast<Vertex*>(&line), offset);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, current_position_y, 0.0f));
            if (fabs(camera_position.x) > scale_x)
            {
                const float& camera_x = camera_position.x;
                model = glm::translate(model, glm::vec3(camera_x - Normalize(camera_x) * scale_x, 0.0f, 0.0f));
            }
            SetUniformMat4(basic_shader, "uModel", model);

            current_position_y += 1.0f;
            glDrawArrays(GL_LINES, prev_offset, 2);
        }

        line = {glm::vec2(0.1f, -1.0f), glm::vec2(-0.1f, -1.0f)};
        current_position_y = plot_origin.y;
        while (current_position_y > camera_position.y - scale_y)
        {
            prev_offset = PushData(2, reinterpret_cast<Vertex*>(&line), offset);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, current_position_y, 0.0f));
            if (fabs(camera_position.x) > scale_x)
            {
                const float& camera_x = camera_position.x;
                model = glm::translate(model, glm::vec3(camera_x - Normalize(camera_x) * scale_x, 0.0f, 0.0f));
            }
            SetUniformMat4(basic_shader, "uModel", model);

            current_position_y -= 1.0f;
            glDrawArrays(GL_LINES, prev_offset, 2);
        }

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Graph 2D Plotter");
        ImGui::Text("Cursor position = (X: %lf, Y: %lf)", 
            X_Offset + (cursor_pos.x - (Width / 2.0f)) / (Width / 2.0f) * scale_x, 
            Y_Offset + (-cursor_pos.y + (Height / 2.0f)) / (Height / 2.0f) * scale_y
        );
        ImGui::End();

        ImGui::Render();
        glfwGetWindowSize(Window, &Width, &Height);
        glViewport(0, 0, Width, Height);

        Aspect_Ratio = static_cast<float>(Width) / Height;
        camera_position = glm::vec3(X_Offset, Y_Offset, Magnification);
        look_position = glm::vec3(X_Offset, Y_Offset, 0.0f);
        camera_direction = look_position - camera_position;
        to_origin = plot_origin - camera_position;

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers with each other and listen for the incoming events, add those to event queue
        glfwSwapBuffers(Window);
    }

    glfwTerminate();
    return 0;
}

// FUCNTION INITIALIZATIONS
size_t PushData(size_t count, Vertex* data, size_t& offset)
{
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(sizeof(Vertex) * offset), sizeof(Vertex) * count, reinterpret_cast<void*>(data));
    size_t prev_offset = offset;
    offset += count;
    return prev_offset;
}

void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE and action == GLFW_RELEASE)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void GLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset == -1)
    {
        if (Magnification < (internal_height * M_PI_2 * Aspect_Ratio) * 3.0f)
            Magnification += 0.05f * (Magnification);
    }
    else if (yoffset == 1)
    {
        if (Magnification > 2.0f + 0.05f)
            Magnification -= 0.05f * (Magnification);
    }
}

void FrameKeyCallback(GLFWwindow* window)
{
    int w_state = glfwGetKey(window, GLFW_KEY_W);
    if (w_state == GLFW_PRESS)
    {
        // Y_Offset -= 0.1f * Magnification / 10.0f;
        Y_Offset += 0.2f * Magnification / 10.0f;
    }
    int a_state = glfwGetKey(window, GLFW_KEY_A);
    if (a_state == GLFW_PRESS)
    {
        // X_Offset += 0.1f * Magnification / 10.0f;
        X_Offset -= 0.2f * Magnification / 10.0f;
    }
    int s_state = glfwGetKey(window, GLFW_KEY_S);
    if (s_state == GLFW_PRESS)
    {
        // Y_Offset += 0.1f * Magnification / 10.0f;
        Y_Offset -= 0.2f * Magnification / 10.0f;
    }
    int d_state = glfwGetKey(window, GLFW_KEY_D);
    if (d_state == GLFW_PRESS)
    {
        // X_Offset -= 0.1f * Magnification / 10.0f;
        X_Offset += 0.2f * Magnification / 10.0f;
    }
}

unsigned int CreateShaderProgram(const std::string& shader_name)
{
    std::string vertStr;
    std::string fragStr;
    std::ifstream vertI;
    std::ifstream fragI;

    std::string vertName = std::string("../data/" + shader_name + ".vert"); 
    std::string fragName = std::string("../data/" + shader_name + ".frag"); 

    vertI.exceptions(std::ios_base::failbit | std::ios_base::badbit);
    fragI.exceptions(std::ios_base::failbit | std::ios_base::badbit);

    try
    {
        std::stringstream vss;
        std::stringstream fss;

        vertI.open(vertName.c_str());
        fragI.open(fragName.c_str());

        vss << vertI.rdbuf();
        fss << fragI.rdbuf();

        vertI.close();
        fragI.close();

        vertStr = vss.str();
        fragStr = fss.str();
    }
    catch (std::ifstream::failure& exception)
    {
        LOG(CRITICAL, "Couldn't parse shader " + shader_name + " (" + exception.what() + ")");
    }

    unsigned int program = glCreateProgram();
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vertSource = vertStr.c_str();
    const char* fragSource = fragStr.c_str();

    glShaderSource(vertex, 1, &vertSource, nullptr);
    glShaderSource(fragment, 1, &fragSource, nullptr);
    
    glCompileShader(vertex);
    glCompileShader(fragment);

    int success;
    char log[256];

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(vertex, 256, nullptr, log);
        LOG(CRITICAL, log);
    }

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(fragment, 256, nullptr, log);
        LOG(CRITICAL, log);
    }

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetProgramInfoLog(program, 256, nullptr, log);
        LOG(CRITICAL, log);
    }

    return program;
}

void SetUniformMat4(unsigned int program, const char* name, const glm::mat4& mat)
{
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void SetUniformVec4(unsigned int program, const char* name, const glm::vec4& vec)
{
    glUniform4fv(glGetUniformLocation(program, name), 1, glm::value_ptr(vec));
}      

void BindShaderProgram(unsigned int program)
{
    glUseProgram(program);
}

void BindVertexArray(unsigned int vao)
{
    glBindVertexArray(vao);
}

constexpr float Function(float x)
{
    return fabs(sin(x)) + fabs(cos(x));
    // return x*x;
    // return sin(x);
}

float GetFunctionExtremum(float(*func)(float), float left, float right)
{
    constexpr size_t precision = PRECISION;
    float delta = (fabs(left) + fabs(right)) / precision;
    float min = 0.0f, max = 0.0f;

    for (; left < right; left += delta)
    {
        float y = func(left);
        if (y > max)
            max = y;
        else if (y < min)
            min = y;
    }

    return fabs(min) > fabs(max) ? min : max;
}