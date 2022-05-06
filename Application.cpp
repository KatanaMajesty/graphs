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

// FUCNTION DECLARATIONS
void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void GLFWScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void FrameKeyCallback(GLFWwindow* window);
unsigned int CreateBuffer();
// program should be bound implicitly
unsigned int CreateShaderProgram(const std::string& shader_name);
void SetUniformMat4(unsigned int program, const char* name, const glm::mat4& mat);
void SetUniformVec4(unsigned int program, const char* name, const glm::vec4& vec);          
void BindShaderProgram(unsigned int program);
void BindVertexArray(unsigned int vao);
// Function
constexpr float Function(float x);
float GetFunctionExtremum(float(*func)(float), float left, float right);

// GLOBAL VARIABLES
GLFWwindow* Window = nullptr;
int Width = 1280;
int Height = 720;
// constexpr float Aspect_Ratio = static_cast<float>(Height) / Width;
float Aspect_Ratio = static_cast<float>(Width) / Height;
const char* Window_Title = "OpenGL Graph visualizer";
constexpr float Left_Border = -5.0f;
constexpr float Right_Border = 5.0f; 
constexpr float Fov = 45.0f;

float internal_height = GetFunctionExtremum(Function, Left_Border, Right_Border);
float internal_width = internal_height * Aspect_Ratio;
// the longest distance from origin to border
float significant_border = fabs(Left_Border) > fabs(Right_Border) ? fabs(Left_Border) : fabs(Right_Border);
float Magnification = internal_height * M_PI_2 * Aspect_Ratio;

glm::vec3 look_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, Magnification);
glm::vec3 camera_direction = look_position - camera_position;

float X_Offset = 0.0f;
float Y_Offset = 0.0f;

int main()
{
    if (!glfwInit())
    {
        LOG(CRITICAL, "Couldn't initialize GLFW");
        return -1;
    }

    Window = glfwCreateWindow(Width, Height, Window_Title, nullptr, nullptr);
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

    // OPENGL DATA
    unsigned int graph_vao;
    glGenVertexArrays(1, &graph_vao);
    glBindVertexArray(graph_vao);

    std::array<Vertex, PRECISION> graph_array;
    float x = Left_Border;
    float delta = (fabs(Left_Border) + fabs(Right_Border)) / graph_array.size();
    std::generate(graph_array.begin(), graph_array.end(), [&]() -> Vertex {
        float y = Function(x);
        x += delta;
        return {glm::vec2(x, y)};
    });

    unsigned int graph_vbo;
    glGenBuffers(1, &graph_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, graph_vbo);
    glNamedBufferData(graph_vbo, sizeof(Vertex) * graph_array.size(), graph_array.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    unsigned int vao_lines;
    glGenVertexArrays(1, &vao_lines);
    glBindVertexArray(vao_lines);

    std::array<Vertex, 4> lines_array = {
        {
            // X axis:
            glm::vec2(1.0f,  0.0f),
            glm::vec2(-1.0f, 0.0f),
            // Y axis:
            glm::vec2(0.0f, 1.0f),
            glm::vec2(0.0f, -1.0f),
        }
    };

    unsigned int vbo_lines;
    glGenBuffers(1, &vbo_lines);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
    glNamedBufferData(vbo_lines, sizeof(Vertex) * lines_array.size(), lines_array.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    unsigned int basic_shader = CreateShaderProgram("BasicShader");

    // glm::mat4 projection = glm::perspective(glm::radians(45.0f), Aspect_Ratio, 1.0f, 100.0f);
    glm::mat4 projection = glm::perspective(glm::radians(Fov), Aspect_Ratio, 0.1f, 1000.0f);
    glm::mat4 model(1.0f);
    glm::mat4 view(1.0f);
    glm::vec3 eye(1.0f);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // RENDER LOOP
    while (!glfwWindowShouldClose(Window))
    {
        // Clear color to draw next frame without artifacts from the previous
        // Main render stuff starts here
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

        FrameKeyCallback(Window);

        model = glm::mat4(1.0f);
        view = glm::mat4(1.0f);
        eye = glm::vec3(0.0f, 0.0f, Magnification);
        float scale = glm::length(camera_direction) * tan(glm::radians(Fov));

        BindShaderProgram(basic_shader);

        BindVertexArray(vao_lines);
        // We don't want to change X & Y axes, thus we do not provide projection and view matrices for them
        view = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, Y_Offset, 0.0f));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        SetUniformMat4(basic_shader, "uProjection", projection);
        SetUniformMat4(basic_shader, "uView", view);
        SetUniformMat4(basic_shader, "uModel", model);
        SetUniformVec4(basic_shader, "uColor", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        glDrawArrays(GL_LINES, 0, 2);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(X_Offset, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

        SetUniformMat4(basic_shader, "uModel", model);
        glDrawArrays(GL_LINES, 2, 2);

        SetUniformMat4(basic_shader, "uProjection", projection);
        SetUniformMat4(basic_shader, "uView", view);
        BindVertexArray(vao_lines);

        float iter = 1.0f;

        do
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(iter + X_Offset, Y_Offset, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, 0.1f, 1.0f));
            SetUniformMat4(basic_shader, "uModel", model);
            glDrawArrays(GL_LINES, 2, 2); // y

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-iter + X_Offset, Y_Offset, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f, 0.1f, 1.0f));
            SetUniformMat4(basic_shader, "uModel", model);
            glDrawArrays(GL_LINES, 2, 2); // y

            iter += 1.0f;
        }
        while (iter <= glm::length(camera_direction) * tan(glm::radians(Fov)));

        iter = 1.0f;

        do
        {
            // X DELIMS -> so draw Y
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(X_Offset, iter + Y_Offset, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 1.0f, 1.0f));
            SetUniformMat4(basic_shader, "uModel", model);
            glDrawArrays(GL_LINES, 0, 2); // x

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(X_Offset, -iter + Y_Offset, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 1.0f, 1.0f));
            SetUniformMat4(basic_shader, "uModel", model);
            glDrawArrays(GL_LINES, 0, 2); // x

            iter += 1.0f;
        }
        while (iter <= glm::length(camera_direction) * tan(glm::radians(Fov)) / Aspect_Ratio);

        BindVertexArray(graph_vao);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(X_Offset, Y_Offset, 0.0f));
        SetUniformMat4(basic_shader, "uModel", model);
        SetUniformVec4(basic_shader, "uColor", glm::vec4(0.0, 0.7, 0.3, 1.0f));

        glDrawArrays(GL_LINE_STRIP, 0, graph_array.size());

        // Swap buffers with each other and listen for the incoming events, add those to event queue
        glfwSwapBuffers(Window);
        glfwPollEvents();
        glfwGetWindowSize(Window, &Width, &Height);
        glViewport(0, 0, Width, Height);

        Aspect_Ratio = static_cast<float>(Width) / Height;
        
        internal_height = GetFunctionExtremum(Function, Left_Border, Right_Border);
        internal_width = internal_height * Aspect_Ratio;

        look_position = glm::vec3(0.0f, 0.0f, 0.0f);
        camera_position = glm::vec3(0.0f, 0.0f, Magnification);
        camera_direction = look_position - camera_position;
    }

    glfwTerminate();
    return 0;
}

// FUCNTION INITIALIZATIONS
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
        Y_Offset -= 0.1f * Magnification / 10.0f;
    }
    int a_state = glfwGetKey(window, GLFW_KEY_A);
    if (a_state == GLFW_PRESS)
    {
        X_Offset += 0.1f * Magnification / 10.0f;
    }
    int s_state = glfwGetKey(window, GLFW_KEY_S);
    if (s_state == GLFW_PRESS)
    {
        Y_Offset += 0.1f * Magnification / 10.0f;
    }
    int d_state = glfwGetKey(window, GLFW_KEY_D);
    if (d_state == GLFW_PRESS)
    {
        X_Offset -= 0.1f * Magnification / 10.0f;
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