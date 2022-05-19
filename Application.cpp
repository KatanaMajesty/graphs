#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define PRECISION 2000
#define LOG(logLevel, message) std::cout << #logLevel << ": " << message << std::endl

enum LogLevel { INFO, DEBUG, CRITICAL };
struct Vertex { glm::vec4 position; };
struct Line { Vertex p1, p2; };

// Function declarations
constexpr float             Function            (float x);
constexpr float             Normalize           (float x) { return x / fabs(x); }
std::pair<float, float>     GetFunctionExtremum (float(*func)(float), float left, float right);
void                        GLFWKeyCallback     (GLFWwindow* window, int key, int scancode, int action, int mods);
void                        GLFWScrollCallback  (GLFWwindow* window, double xoffset, double yoffset);
void                        FrameKeyCallback    (GLFWwindow* window);
void                        ImGuiInitialize     (GLFWwindow* window);
void                        SetUniformMat4      (unsigned int program, const char* name, const glm::mat4& mat);
void                        SetUniformVec4      (unsigned int program, const char* name, const glm::vec4& vec);
size_t                      PushData            (size_t count, Vertex* data, size_t& offset);
unsigned int                CreateShaderProgram (const std::string& shader_name);

// Global variables
GLFWwindow* Window = nullptr;
int Width = 1280;
int Height = 720;

float Aspect_Ratio = static_cast<float>(Width) / Height;

// Function and camera information
constexpr float Left_Border = -20.0f;
constexpr float Right_Border = 5.0f; 
constexpr float Fov = 45.0f;

// Vertex Buffer capacity, that will be multiplied by sizeof(Vertex)
constexpr size_t Data_Size = PRECISION + 256;

// This variable stores function's minimum and maximum
std::pair<float, float> Extremum = GetFunctionExtremum(Function, Left_Border, Right_Border);
// This variable stores function's maximum
const float Func_Maximum = Extremum.second;
// Magnification stands for camera's Z coordinate
float Magnification = fabs(Func_Maximum) / tan(glm::radians(Fov / 2.0f)) + 0.1f;

// Camera offset from (0, 0) on X, Y axes
float X_Offset = (Left_Border + Right_Border) / 2.0f;
float Y_Offset = (Extremum.first + Extremum.second) / 2.0f;

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        LOG(CRITICAL, "Couldn't initialize GLFW");
        return -1;
    }

    // Create a window
    Window = glfwCreateWindow(Width, Height, "Lab 17 AP", nullptr, nullptr);
    if (!Window)
    {
        LOG(CRITICAL, "Couldn't create a window");
        return -1;
    }

    // context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwMakeContextCurrent(Window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        LOG(CRITICAL, "Couldn't initialize GLEW");
        return -1;
    }

    // glfw callbacks
    glfwSetScrollCallback(Window, GLFWScrollCallback);
    glfwSetKeyCallback(Window, GLFWKeyCallback);
    glfwSwapInterval(1);

    // Function to create and initialize ImGui context
    ImGuiInitialize(Window);

    std::array<Vertex, PRECISION> graph_array;
    float delta = (fabs(Left_Border - Right_Border)) / graph_array.size();
    std::generate(graph_array.begin(), graph_array.end(), [&, x = Left_Border]() mutable -> Vertex {
        float y = Function(x);
        x += delta;
        return { glm::vec4(x, y, 0.0f, 1.0f) };
    });

    // OpenGL data
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

    // 3d camera vectors
    glm::vec3 camera_position(0.0f, 0.0f, Magnification);
    glm::vec3 look_position(0.0f);
    glm::vec3 plot_origin(0.0f);
    glm::vec3 camera_direction = look_position - camera_position;
    glm::vec3 to_origin = plot_origin - camera_position;

    // MVP matrices
    glm::mat4 projection = glm::perspective(glm::radians(Fov), Aspect_Ratio, 0.1f, 1000.0f);
    glm::mat4 view(1.0f);
    glm::mat4 model(1.0f);

    // X, Y axes
    Line x = { glm::vec4(-1.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) };
    Line y = { glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f, -1.0f, 0.0f, 1.0f) };

    // variable to store cursor position
    glm::vec<2, double> cursor_pos;

    // Render loop
    while (!glfwWindowShouldClose(Window))
    {
        // Clear color to draw next frame without artifacts from the previous
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

        // Per-Frame keyboard callback
        FrameKeyCallback(Window);

        glfwGetCursorPos(Window, &cursor_pos.x, &cursor_pos.y);

        glBindVertexArray(vao);
        glUseProgram(basic_shader);

        size_t offset = 0;
        size_t prev_offset;
        view = glm::lookAt(camera_position, look_position, glm::vec3(0.0f, 1.0f, 0.0f));

        // Count width and height from plot origin to the render border
        float scale_y = camera_position.z * tan(glm::radians(Fov / 2.0f));
        float scale_x = scale_y * Aspect_Ratio;

        // Push data into Vertex Buffer and draw 2D graph
        // PushData() function returns the offset, which will be used in glDrawArrays later
        prev_offset = PushData(graph_array.size(), graph_array.data(), offset);
        model = glm::mat4(1.0f);
        // Set uniforms
        SetUniformMat4(basic_shader, "uModel", model);
        SetUniformMat4(basic_shader, "uView", view);
        SetUniformMat4(basic_shader, "uProjection", projection);
        SetUniformVec4(basic_shader, "uColor", glm::vec4(0.5f, 1.0f, 0.4f, 1.0f));
        glDrawArrays(GL_LINE_STRIP, prev_offset, graph_array.size());

        // Same for PushData() function here, but I use reinterpret_cast<> 
        // in order to ensure the compiler that I point on the correct struct
        prev_offset = PushData(2, reinterpret_cast<Vertex*>(&x), offset);
        model = glm::mat4(1.0f);
        // "Stick" X axis to the camera's X coordinate in order to save memory usage
        model = glm::translate(model, glm::vec3(camera_position.x, 0.0f, 0.0f));
        // scale_x depends on camera's Z coordinate, thus on Magnification variable.
        // -> The more magnification variable, the more scale_x, so the longer X axis should be
        model = glm::scale(model, glm::vec3(scale_x, 1.0f, 1.0f));
        SetUniformMat4(basic_shader, "uModel", model);
        SetUniformVec4(basic_shader, "uColor", glm::vec4(0.7f));
        glDrawArrays(GL_LINES, prev_offset, 2);

        prev_offset = PushData(2, reinterpret_cast<Vertex*>(&y), offset);
        model = glm::mat4(1.0f);
        // Same to the X axis
        model = glm::translate(model, glm::vec3(0.0f, camera_position.y, 0.0f));
        // scale_y depends on camera's Z coordinate, thus on Magnification variable.
        // -> The more magnification variable, the more scale_y, so the longer Y axis should be
        model = glm::scale(model, glm::vec3(1.0f, scale_y, 1.0f));
        SetUniformMat4(basic_shader, "uModel", model);
        glDrawArrays(GL_LINES, prev_offset, 2);

        float current_position_x = plot_origin.x;
        size_t current_size = 0;
        bool is_first = true;
        // Draw delimeters while the current draw position is less than camera's current position
        while (current_position_x - 1.0f < camera_position.x + scale_x)
        {
            // Create a delimeter line
            Line line {
                glm::vec4(current_position_x, 0.1f, 0.0f, 1.0f), 
                glm::vec4(current_position_x, -0.1f, 0.0f, 1.0f)
            };
            // get it's current offset to be drawn at
            size_t current_offset = PushData(2, reinterpret_cast<Vertex*>(&line), offset);
            if (is_first)
            {
                // save the offset variable, if it's the first iteration
                prev_offset = current_offset;
                is_first = false;
            }
            // add line's vertex count and increment it's current position
            current_size += 2;
            current_position_x += 1.0f;
        }
        model = glm::mat4(1.0f);
        // check if X axis is out of our camera view and translate it with camera if true
        if (fabs(camera_position.y) > scale_y)
        {
            // A simple matrix to determine which direction the axis should move in order to follow the camera
            model = glm::translate(model, glm::vec3(0.0f, camera_position.y - Normalize(camera_position.y) * scale_y, 0.0f));
        }
        SetUniformMat4(basic_shader, "uModel", model);
        glDrawArrays(GL_LINES, prev_offset, current_size);

        // Absolutely the same loop as the previous one, but for another part of the plot
        current_position_x = plot_origin.x;
        current_size = 0;
        is_first = true;
        while (current_position_x > camera_position.x - scale_x)
        {
            Line line {
                glm::vec4(current_position_x, 0.1f, 0.0f, 1.0f), 
                glm::vec4(current_position_x, -0.1f, 0.0f, 1.0f)
            };
            size_t current_offset = PushData(2, reinterpret_cast<Vertex*>(&line), offset);
            if (is_first)
            {
                prev_offset = current_offset;
                is_first = false;
            }
            current_size += 2;
            current_position_x -= 1.0f;
        }
        model = glm::mat4(1.0f);
        if (fabs(camera_position.y) > scale_y)
        {
            model = glm::translate(model, glm::vec3(0.0f, camera_position.y - Normalize(camera_position.y) * scale_y, 0.0f));
        }
        SetUniformMat4(basic_shader, "uModel", model);
        glDrawArrays(GL_LINES, prev_offset, current_size);

        // Absolutely the same loop as the previous one, but for another part of the plot
        float current_position_y = plot_origin.y;
        current_size = 0;
        is_first = true;
        while (current_position_y < camera_position.y + scale_y)
        {
            Line line {
                glm::vec4(0.1f, current_position_y, 0.0f, 1.0f), 
                glm::vec4(-0.1f, current_position_y, 0.0f, 1.0f)
            };
            size_t current_offset = PushData(2, reinterpret_cast<Vertex*>(&line), offset);
            if (is_first)
            {
                prev_offset = current_offset;
                is_first = false;
            }
            current_position_y += 1.0f;
            current_size += 2;
        }
        model = glm::mat4(1.0f);
        if (fabs(camera_position.x) > scale_x)
        {
            model = glm::translate(model, glm::vec3(camera_position.x - Normalize(camera_position.x) * scale_x, 0.0f, 0.0f));
        }
        SetUniformMat4(basic_shader, "uModel", model);
        glDrawArrays(GL_LINES, prev_offset, current_size);

        // Absolutely the same loop as the previous one, but for another part of the plot
        current_position_y = plot_origin.y;
        current_size = 0;
        is_first = true;
        while (current_position_y > camera_position.y - scale_y)
        {
            Line line {
                glm::vec4(0.1f, current_position_y, 0.0f, 1.0f), 
                glm::vec4(-0.1f, current_position_y, 0.0f, 1.0f)
            };
            size_t current_offset = PushData(2, reinterpret_cast<Vertex*>(&line), offset);
            if (is_first)
            {
                prev_offset = current_offset;
                is_first = false;
            }
            current_position_y -= 1.0f;
            current_size += 2;
        }
        model = glm::mat4(1.0f);
        if (fabs(camera_position.x) > scale_x)
        {
            model = glm::translate(model, glm::vec3(camera_position.x - Normalize(camera_position.x) * scale_x, 0.0f, 0.0f));
        }
        SetUniformMat4(basic_shader, "uModel", model);
        glDrawArrays(GL_LINES, prev_offset, current_size);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Graph 2D Plotter");
        // In ImGui::Text function I translate (x, y) to ( (x - W/2) / W/2*s, (y - H/2) / H/2*s )
        // This will translate any coordinate from glViewport to (-1, 1) coordinate system
        // After this I'm able to translate coordinate from (-1, 1) space into current plot coordinate system
        // by multiplying on scale  
        ImGui::Text("Cursor position = (X: %lf, Y: %lf)", 
            X_Offset + (cursor_pos.x - (Width / 2.0f)) / (Width / 2.0f) * scale_x, 
            Y_Offset + (-cursor_pos.y + (Height / 2.0f)) / (Height / 2.0f) * scale_y
        );
        ImGui::End();

        // Render ImGui context
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Listen to events, get new window Width and Height
        glfwPollEvents();
        glfwGetWindowSize(Window, &Width, &Height);
        glViewport(0, 0, Width, Height);

        // Reassign variables to ensure all the calculations are correct
        Aspect_Ratio = static_cast<float>(Width) / Height;
        camera_position = glm::vec3(X_Offset, Y_Offset, Magnification);
        look_position = glm::vec3(X_Offset, Y_Offset, 0.0f);
        camera_direction = look_position - camera_position;
        to_origin = plot_origin - camera_position;

        // Swap buffers with each other and listen for the incoming events, add those to event queue
        glfwSwapBuffers(Window);
    }

    glfwTerminate();
    return 0;
}

// FUCNTION INITIALIZATIONS
size_t PushData(size_t count, Vertex* data, size_t& offset)
{
    // Copy data from *data pointer into currently bound Vertex Buffer
    glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(sizeof(Vertex) * offset), sizeof(Vertex) * count, reinterpret_cast<void*>(data));
    size_t prev_offset = offset;
    // increment current offset and return the previous one
    offset = (offset + count) % Data_Size;
    return prev_offset;
}

void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Press ESC to close the application
    if (key == GLFW_KEY_ESCAPE and action == GLFW_RELEASE)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void GLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Change Magnification with mouse wheel callback
    if (yoffset == -1)
    {
        if (Magnification < (fabs(Func_Maximum) * M_PI * Aspect_Ratio) * 3.0f)
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
    // Move around the plot
    int w_state = glfwGetKey(window, GLFW_KEY_W);
    if (w_state == GLFW_PRESS)
    {
        Y_Offset += 0.2f * Magnification / 10.0f;
    }
    int a_state = glfwGetKey(window, GLFW_KEY_A);
    if (a_state == GLFW_PRESS)
    {
        X_Offset -= 0.2f * Magnification / 10.0f;
    }
    int s_state = glfwGetKey(window, GLFW_KEY_S);
    if (s_state == GLFW_PRESS)
    {
        Y_Offset -= 0.2f * Magnification / 10.0f;
    }
    int d_state = glfwGetKey(window, GLFW_KEY_D);
    if (d_state == GLFW_PRESS)
    {
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

void ImGuiInitialize(GLFWwindow* window)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

constexpr float Function(float x)
{
    // return fabs(sin(x)) + fabs(cos(x));
    // return x*x;
    return sin(x) - 2.0f;
}

// Returns (min, max) pair
std::pair<float, float> GetFunctionExtremum(float(*func)(float), float left, float right)
{
    constexpr size_t precision = PRECISION;
    float delta = (fabs(left - right)) / precision;
    float min = right, max = left;

    for (; left < right; left += delta)
    {
        float y = func(left);
        if (y > max)
            max = y;
        else if (y < min)
            min = y;
    }

    return std::make_pair(min, max);
}