#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include "PerlinNoise.cpp"
#include "ArcballCamera.cpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstdlib>
#include <curl/curl.h>
#include "json.hpp" // For nlohmann::json
#include <sstream>

// ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Add ImGui variables for chat interface
static ImGuiTextBuffer chatHistory;
static char inputBuffer[256] = "";
static bool scrollToBottom = false;

// Global variables for terrain parameters
int numOctaves = 4;
float persistence = 0.5f;
float lacunarity = 2.0f;
float baseAmplitude = 0.5f;
float baseFrequency = 0.4f;

// Conversation history for LLM context
std::vector<nlohmann::json> conversationHistory;

// Terrain data structures
std::vector<float> vertices;
std::vector<unsigned int> indices;
std::vector<float> normals;

// Terrain dimensions
int width = 500;
int height = 500;

float waterLevel = 0.5f;         // Initial water level
unsigned int waterVAO, waterVBO; // Water plane buffers

unsigned int VAO, VBO, EBO;

// Global Variables
ArcballCamera camera(glm::vec3(0.0f, 0.5f, 0.0f), 2.0f, -90.0f, -20.0f);
PerlinNoise perlin;
bool leftMousePressed = false;
bool rightMousePressed = false;
float lastX = 400.0f, lastY = 300.0f;

// Function Prototypes
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset);
void generateAdvancedTerrain(int width, int height, std::vector<float> &vertices, std::vector<unsigned int> &indices, std::vector<float> &normals);
unsigned int createShaderProgram();
unsigned int loadTexture(const char *path);
void setupBuffers(unsigned int &VAO, unsigned int &VBO, unsigned int &EBO, const std::vector<float> &vertices, const std::vector<float> &normals, const std::vector<unsigned int> &indices);
void checkOpenGLError();
unsigned int compileShader(const char *shaderSource, GLenum shaderType);
unsigned int loadCubemap(std::vector<std::string> faces);
void generateWaterPlane(std::vector<float> &waterVertices, float width, float depth);
void setupWaterBuffers(unsigned int &waterVAO, unsigned int &waterVBO, const std::vector<float> &waterVertices);
unsigned int createWaterShaderProgram();
unsigned int createSkyboxShaderProgram();

// Light position
glm::vec3 lightPos(0.0f, 2.0f, 5.0f);

// TerrainParameters structure
struct TerrainParameters
{
    int numOctaves;
    float persistence;
    float lacunarity;
    float baseAmplitude;
    float baseFrequency;
};

// Stack to keep track of terrain parameter history for undo functionality
std::stack<TerrainParameters> terrainStateHistory;

void updateTerrain(int width, int height,
                   std::vector<float>& vertices,
                   std::vector<unsigned int>& indices,
                   std::vector<float>& normals,
                   int numOctaves,
                   float persistence,
                   float lacunarity,
                   float baseAmplitude,
                   float baseFrequency)
{
    // Save current state before changing
    TerrainParameters currentParams = {
        ::numOctaves,
        ::persistence,
        ::lacunarity,
        ::baseAmplitude,
        ::baseFrequency
    };
    terrainStateHistory.push(currentParams);

    // Update global parameters
    ::numOctaves = numOctaves;
    ::persistence = persistence;
    ::lacunarity = lacunarity;
    ::baseAmplitude = baseAmplitude;
    ::baseFrequency = baseFrequency;

    // Clear existing data
    vertices.clear();
    indices.clear();
    normals.clear();

    // Generate terrain with new parameters
    generateAdvancedTerrain(width, height, vertices, indices, normals);

    // Update buffers and reload data to GPU if necessary
    setupBuffers(VAO, VBO, EBO, vertices, normals, indices);
}

nlohmann::json functionDefinitions = nlohmann::json::array({
    {
        {"name", "updateTerrain"},
        {"description", "Updates terrain parameters and regenerates the terrain."},
        {"parameters",
            {
                {"type", "object"},
                {"properties",
                    {
                        {"numOctaves", {
                            {"type", "integer"},
                            {"description", "Number of noise octaves (controls detail level)."},
                            {"minimum", 1},
                            {"maximum", 10}
                        }},
                        {"persistence", {
                            {"type", "number"},
                            {"description", "Amplitude decay factor (controls smoothness)."},
                            {"minimum", 0.1},
                            {"maximum", 1.0}
                        }},
                        {"lacunarity", {
                            {"type", "number"},
                            {"description", "Frequency increase factor (controls feature density)."},
                            {"minimum", 1.0},
                            {"maximum", 4.0}
                        }},
                        {"baseAmplitude", {
                            {"type", "number"},
                            {"description", "Base amplitude for terrain height (controls hill height)."},
                            {"minimum", 0.1},
                            {"maximum", 5.0}
                        }},
                        {"baseFrequency", {
                            {"type", "number"},
                            {"description", "Base frequency for terrain features (controls feature size)."},
                            {"minimum", 0.1},
                            {"maximum", 5.0}
                        }}
                    }},
                {"required", nlohmann::json::array({"numOctaves", "persistence", "lacunarity", "baseAmplitude", "baseFrequency"})}
            }}
    }
});


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string getAPIKey()
{
    const char* apiKey = std::getenv("OPENAI_API_KEY");
    if (!apiKey)
    {
        std::cerr << "Error: OPENAI_API_KEY environment variable not set." << std::endl;
        exit(1);
    }
    return std::string(apiKey);
}

std::string buildSystemPrompt()
{
    std::string systemPrompt = R"(
You are an assistant integrated into a procedural terrain generation system built using C++ and OpenGL.
The system uses several terrain parameters, and the user input determines how the terrain is modified. Your task is to interpret natural
language inputs and adjust the terrain parameters accordingly, making moderate adjustments based on the user's intent.

The terrain is generated using Perlin noise. The parameters you need to adjust based on user input are:

- numOctaves (Integer): Controls the number of layers (octaves) of noise that are combined to generate the terrain. Higher values add more detail. Current value is )" + std::to_string(::numOctaves) + R"(.
- persistence (Float): Controls the amplitude decay of each octave. Lower values create smoother terrain. Current value is )" + std::to_string(::persistence) + R"(.
- lacunarity (Float): Controls the frequency increase between octaves. Higher values make the terrain features denser. Current value is )" + std::to_string(::lacunarity) + R"(.
- baseAmplitude (Float): Determines the overall height variation. Higher values create taller hills. Current value is )" + std::to_string(::baseAmplitude) + R"(.
- baseFrequency (Float): Controls the overall scale of the terrain features. Higher values make the features more frequent (smaller hills). Current value is )" + std::to_string(::baseFrequency) + R"(.

Remember the user's previous instructions and adjust parameters accordingly. If the user wants to revert changes or extend on previous commands, handle that appropriately.

When adjusting parameters, make moderate changes based on the user's input, unless the user explicitly requests significant changes. Avoid changing parameters by large amounts unless necessary.

You will extract terrain parameters from user input and call the updateTerrain function accordingly. Do not provide any explanations or additional text.
)";
    return systemPrompt;
}

void initializeConversationHistory()
{
    nlohmann::json systemMessage = {
        {"role", "system"},
        {"content", buildSystemPrompt()}
    };
    conversationHistory.push_back(systemMessage);
}

void truncateConversationHistory()
{
    // Define a maximum number of messages
    const size_t maxMessages = 20;

    if (conversationHistory.size() > maxMessages)
    {
        // Remove the oldest user and assistant messages, keeping the system prompt
        conversationHistory.erase(conversationHistory.begin() + 1, conversationHistory.begin() + 3);
    }
}

std::string sendOpenAIRequest(const std::string& userInput)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl)
    {
        std::string api_key = getAPIKey();
        std::string auth_header = "Authorization: Bearer " + api_key;
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Add the user's message to the conversation history
        conversationHistory.push_back({
            {"role", "user"},
            {"content", userInput}
        });

        // Prepare the JSON payload
        nlohmann::json jsonPayload;
        jsonPayload["model"] = "gpt-4";
        jsonPayload["messages"] = conversationHistory;

        // Add function definitions for function calling
        jsonPayload["functions"] = functionDefinitions;
        jsonPayload["function_call"] = "auto";

        // Convert JSON payload to string
        std::string payload = jsonPayload.dump();

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());

        // Set up callback to capture response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);

        // Clean up
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if(res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return "";
        }
    }

    return readBuffer;
}

nlohmann::json parseOpenAIResponse(const std::string& response)
{
    nlohmann::json jsonResponse = nlohmann::json::parse(response);
    auto message = jsonResponse["choices"][0]["message"];

    // Add the assistant's message to the conversation history
    conversationHistory.push_back(message);

    if (message.contains("function_call"))
    {
        std::string functionName = message["function_call"]["name"];
        std::string arguments = message["function_call"]["arguments"];
        nlohmann::json argsJson = nlohmann::json::parse(arguments);

        return { {"function_name", functionName}, {"arguments", argsJson} };
    }
    else
    {
        throw std::runtime_error("No function_call in response");
    }
}

void invokeTerrainFunction(const nlohmann::json& functionCall)
{
    std::string functionName = functionCall["function_name"];
    nlohmann::json args = functionCall["arguments"];

    if (functionName == "updateTerrain")
    {
        // Extract parameters with default values
        int newNumOctaves = args.value("numOctaves", ::numOctaves);
        float newPersistence = args.value("persistence", ::persistence);
        float newLacunarity = args.value("lacunarity", ::lacunarity);
        float newBaseAmplitude = args.value("baseAmplitude", ::baseAmplitude);
        float newBaseFrequency = args.value("baseFrequency", ::baseFrequency);

        // Limit the changes to reasonable amounts
        int deltaNumOctaves = newNumOctaves - ::numOctaves;
        if (deltaNumOctaves > 2) newNumOctaves = ::numOctaves + 2;
        if (deltaNumOctaves < -2) newNumOctaves = ::numOctaves - 2;

        float deltaPersistence = newPersistence - ::persistence;
        if (deltaPersistence > 0.2f) newPersistence = ::persistence + 0.2f;
        if (deltaPersistence < -0.2f) newPersistence = ::persistence - 0.2f;

        float deltaLacunarity = newLacunarity - ::lacunarity;
        if (deltaLacunarity > 0.5f) newLacunarity = ::lacunarity + 0.5f;
        if (deltaLacunarity < -0.5f) newLacunarity = ::lacunarity - 0.5f;

        float deltaBaseAmplitude = newBaseAmplitude - ::baseAmplitude;
        if (deltaBaseAmplitude > 0.5f) newBaseAmplitude = ::baseAmplitude + 0.5f;
        if (deltaBaseAmplitude < -0.5f) newBaseAmplitude = ::baseAmplitude - 0.5f;

        float deltaBaseFrequency = newBaseFrequency - ::baseFrequency;
        if (deltaBaseFrequency > 0.5f) newBaseFrequency = ::baseFrequency + 0.5f;
        if (deltaBaseFrequency < -0.5f) newBaseFrequency = ::baseFrequency - 0.5f;

        // Ensure parameters are within valid ranges
        newNumOctaves = std::clamp(newNumOctaves, 1, 10);
        newPersistence = std::clamp(newPersistence, 0.1f, 1.0f);
        newLacunarity = std::clamp(newLacunarity, 1.0f, 4.0f);
        newBaseAmplitude = std::clamp(newBaseAmplitude, 0.1f, 5.0f);
        newBaseFrequency = std::clamp(newBaseFrequency, 0.1f, 5.0f);

        // Call the updateTerrain function
        updateTerrain(width, height, vertices, indices, normals,
                      newNumOctaves, newPersistence, newLacunarity,
                      newBaseAmplitude, newBaseFrequency);

        // Prepare a string with the updated parameter values
        std::ostringstream oss;
        oss << "Assistant: Terrain parameters updated.\n\n";
        oss << "Current Terrain Parameters:\n\n";
        oss << "Number of Octaves: " << ::numOctaves << "\n";
        oss << "Persistence: " << ::persistence << "\n";
        oss << "Lacunarity: " << ::lacunarity << "\n";
        oss << "Base Amplitude: " << ::baseAmplitude << "\n";
        oss << "Base Frequency: " << ::baseFrequency << "\n";

        // Append the parameter values to the chat history
        chatHistory.append(oss.str().c_str());
        scrollToBottom = true;
    }

    else
    {
        std::cerr << "Unknown function called: " << functionName << std::endl;
    }
}

void undoTerrainChange()
{
    if (!terrainStateHistory.empty())
    {
        TerrainParameters previousParams = terrainStateHistory.top();
        terrainStateHistory.pop();

        // Update parameters to previous state
        ::numOctaves = previousParams.numOctaves;
        ::persistence = previousParams.persistence;
        ::lacunarity = previousParams.lacunarity;
        ::baseAmplitude = previousParams.baseAmplitude;
        ::baseFrequency = previousParams.baseFrequency;

        // Regenerate the terrain
        vertices.clear();
        indices.clear();
        normals.clear();
        generateAdvancedTerrain(width, height, vertices, indices, normals);

        // Update buffers and reload data to GPU if necessary
        setupBuffers(VAO, VBO, EBO, vertices, normals, indices);

        // Provide feedback to the user
        chatHistory.append("Assistant: Reverted to previous terrain state.\n");
        scrollToBottom = true;
    }
    else
    {
        chatHistory.append("Assistant: No previous terrain state to revert to.\n");
        scrollToBottom = true;
    }
}

// Function to initialize ImGui
void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls

    // Set up Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

// Function to render the ImGui chat interface
void renderChatInterface() {
    // Start a new ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Set window size and position for the chat interface
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Once);

    // Create chat interface window
    ImGui::Begin("Terrain Assistant", nullptr, ImGuiWindowFlags_NoCollapse);

    // Chat history with scrollbar
    ImGui::BeginChild("ChatHistory", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(chatHistory.begin());
    if (scrollToBottom)
        ImGui::SetScrollHereY(1.0f);  // Scroll to bottom if needed
    scrollToBottom = false;
    ImGui::EndChild();

    // Separator
    ImGui::Separator();

    // Input text box
    ImGui::PushItemWidth(-40);
    if (ImGui::InputText("##Input", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (strlen(inputBuffer) > 0)
        {
            // Append the user input to the chat history
            chatHistory.append("User: ");
            chatHistory.append(inputBuffer);
            chatHistory.append("\n\n");

            std::string userInput = std::string(inputBuffer);

            // Check for undo command
            if (userInput == "undo" || userInput == "revert")
            {
                undoTerrainChange();

                // Optionally, add the undo command to the conversation history
                conversationHistory.push_back({
                    {"role", "user"},
                    {"content", userInput}
                });
                conversationHistory.push_back({
                    {"role", "assistant"},
                    {"content", "Reverted to previous terrain state."}
                });
            }
            else
            {
                // Send inputBuffer to OpenAI for processing
                std::string response = sendOpenAIRequest(userInput);

                // Parse and invoke terrain modification functions
                if (!response.empty())
                {
                    try
                    {
                        nlohmann::json functionCall = parseOpenAIResponse(response);
                        invokeTerrainFunction(functionCall);
                    }
                    catch (const std::exception& e)
                    {
                        chatHistory.append("Assistant: Error - ");
                        chatHistory.append(e.what());
                        chatHistory.append("\n");
                        scrollToBottom = true;
                    }
                }
            }

            // Clear input buffer after processing
            strcpy(inputBuffer, "");
            scrollToBottom = true;
        }
    }
    ImGui::PopItemWidth();

    // Send button
    ImGui::SameLine();
    if (ImGui::Button("Send"))
    {
        if (strlen(inputBuffer) > 0)
        {
            // Append the user input to the chat history
            chatHistory.append("User: ");
            chatHistory.append(inputBuffer);
            chatHistory.append("\n");

            std::string userInput = std::string(inputBuffer);

            // Check for undo command
            if (userInput == "undo" || userInput == "revert")
            {
                undoTerrainChange();

                // Optionally, add the undo command to the conversation history
                conversationHistory.push_back({
                    {"role", "user"},
                    {"content", userInput}
                });
                conversationHistory.push_back({
                    {"role", "assistant"},
                    {"content", "Reverted to previous terrain state."}
                });
            }
            else
            {
                // Send inputBuffer to OpenAI for processing
                std::string response = sendOpenAIRequest(userInput);

                // Parse and invoke terrain modification functions
                if (!response.empty())
                {
                    try
                    {
                        nlohmann::json functionCall = parseOpenAIResponse(response);
                        invokeTerrainFunction(functionCall);
                    }
                    catch (const std::exception& e)
                    {
                        chatHistory.append("Assistant: Error - ");
                        chatHistory.append(e.what());
                        chatHistory.append("\n");
                        scrollToBottom = true;
                    }
                }
            }

            // Clear input buffer after processing
            strcpy(inputBuffer, "");
            scrollToBottom = true;
        }
    }

    ImGui::End(); // End of chat interface

    // Render ImGui frame
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Cleanup function for ImGui
void cleanupImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


// Main Function
int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // GLFW Configuration
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW Window
    GLFWwindow *window = glfwCreateWindow(1920, 1080, "LLM-Driven Terrain Generator", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set the viewport
    glViewport(0, 0, 1920, 1080);

    // Register Input Callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Configure Global OpenGL State
    glEnable(GL_DEPTH_TEST);

    // Load Textures
    unsigned int grassTexture = loadTexture("../resources/textures/grass.png");
    unsigned int rockTexture = loadTexture("../resources/textures/rock.png");
    unsigned int snowTexture = loadTexture("../resources/textures/snow.jpg");

    std::cout << "Grass texture ID: " << grassTexture << std::endl;
    std::cout << "Rock texture ID: " << rockTexture << std::endl;
    std::cout << "Snow texture ID: " << snowTexture << std::endl;

    // Load Skybox Cubemap Textures
    std::vector<std::string> faces = {
        "../resources/textures/right.jpg",
        "../resources/textures/left.jpg",
        "../resources/textures/top.jpg",
        "../resources/textures/bottom.jpg",
        "../resources/textures/front.jpg",
        "../resources/textures/back.jpg"
    };

    unsigned int cubemapTexture = loadCubemap(faces);

    // Skybox vertices
    float skyboxVertices[] = {
        // positions
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f
    };

    // Create VAO and VBO for the skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    unsigned int skyboxShaderProgram = createSkyboxShaderProgram();
    glUseProgram(skyboxShaderProgram);
    glUniform1i(glGetUniformLocation(skyboxShaderProgram, "skybox"), 0); // Set the skybox sampler uniform

    // Generate Advanced Terrain Grid
    generateAdvancedTerrain(width, height, vertices, indices, normals);

    std::cout << "Vertices generated: " << vertices.size() << std::endl;
    std::cout << "Normals generated: " << normals.size() << std::endl;
    std::cout << "Indices generated: " << indices.size() << std::endl;


    // Create Shader Program
    unsigned int shaderProgram = createShaderProgram();

    // Set texture uniforms
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "grassTexture"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "rockTexture"), 1);
    glUniform1i(glGetUniformLocation(shaderProgram, "snowTexture"), 2);

    // Setup Buffers
    setupBuffers(VAO, VBO, EBO, vertices, normals, indices);

    // Generate Water Plane
    std::vector<float> waterVertices;
    
    // Adjust width and depth as needed
    generateWaterPlane(waterVertices, 1.0f, 1.0f); 

    // Create Water Shader Program
    unsigned int waterShaderProgram = createWaterShaderProgram();

    // Setup Water Buffers
    setupWaterBuffers(waterVAO, waterVBO, waterVertices);

    // Initialize ImGui
    initImGui(window);

    // Initialize conversation history
    initializeConversationHistory();

    // Main Render Loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Clear Screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Check if ImGui is capturing mouse and keyboard events
        ImGuiIO& io = ImGui::GetIO();
        
        // If ImGui is not capturing the mouse or keyboard, process the camera and scene inputs
        if (!io.WantCaptureMouse && !io.WantCaptureKeyboard) {
            processInput(window);  // Only process input if ImGui is not active
        }

        // Enable blending for transparent water rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Use Shader Program
        glUseProgram(shaderProgram);

        // Bind Textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, rockTexture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, snowTexture);

        // View/Projection Transformations
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = projection * view * model;

        // Send MVP Matrix to Shader
        int mvpLoc = glGetUniformLocation(shaderProgram, "transform");
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // Pass Lighting Information to Shader
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera.GetCameraPosition()));

        // Bind VAO and Draw the Grid
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // // Render Water Plane
        // glUseProgram(waterShaderProgram);

        // // Set the transformation matrix for the water shader
        // int waterTransformLoc = glGetUniformLocation(waterShaderProgram, "transform");
        // glUniformMatrix4fv(waterTransformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        // // Set the water color (RGBA)
        // glUniform4f(glGetUniformLocation(waterShaderProgram, "waterColor"), 0.0f, 0.3f, 0.6f, 0.5f); // Adjust color and transparency as desired

        // float timeValue = glfwGetTime(); // Get the elapsed time
        // glUniform1f(glGetUniformLocation(waterShaderProgram, "time"), timeValue);

        // glUniform3fv(glGetUniformLocation(waterShaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        
        // glm::vec3 viewPosition = camera.GetCameraPosition();
        // glUniform3fv(glGetUniformLocation(waterShaderProgram, "viewPos"), 1, glm::value_ptr(viewPosition));

        // // Bind Water VAO and draw
        // glBindVertexArray(waterVAO);
        // glDrawArrays(GL_TRIANGLES, 0, 6);

        // Render the skybox (disable depth testing so it's drawn behind everything else)
        glDepthFunc(GL_LEQUAL); // Change depth function so skybox passes depth test
        glUseProgram(skyboxShaderProgram);

        // Remove translation from the view matrix
        glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix())); // Extract rotation
        glm::mat4 skyboxProjection = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(skyboxProjection));

        // Render the skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        // Restore default depth function
        glDepthFunc(GL_LESS);

        //Render the chat interface
        renderChatInterface();

        // Swap Buffers and Poll Events
        glfwSwapBuffers(window);

        // Check for OpenGL errors
        checkOpenGLError();
    }

    // Cleanup ImGui resources
    cleanupImGui();

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Delete water resources
    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteProgram(waterShaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}

// Process Input
void processInput(GLFWwindow *window)
{
    ImGuiIO& io = ImGui::GetIO();

    // Skip input processing if ImGui is capturing the mouse or keyboard
    if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
        return;  // Do not process input for the scene when ImGui is active
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Get camera direction vectors
    glm::vec3 cameraFront = camera.GetCameraFront();                                        // Forward direction of the camera
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f))); // Right direction relative to camera
    glm::vec3 up(0.0f, 1.0f, 0.0f);                                                         // Global up direction

    // Camera speed for movement
    float cameraSpeed = 0.05f;

    // Move camera forward (W)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera.Target += cameraFront * cameraSpeed;
    }
    // Move camera backward (S)
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera.Target -= cameraFront * cameraSpeed;
    }
    // Move camera left (A)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera.Target -= right * cameraSpeed;
    }
    // Move camera right (D)
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera.Target += right * cameraSpeed;
    }
}

// Mouse Movement Callback
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();

    // Skip mouse input processing if ImGui is capturing the mouse
    if (io.WantCaptureMouse) {
        return;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    if (leftMousePressed)
    {
        camera.ProcessMouseMovement(xOffset, yOffset, true);
    }
    if (rightMousePressed)
    {
        camera.ProcessMousePan(xOffset, yOffset);
    }
}

// Mouse Button Callback
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();

    // Skip mouse button input if ImGui is capturing the mouse
    if (io.WantCaptureMouse) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            leftMousePressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            leftMousePressed = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            rightMousePressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            rightMousePressed = false;
        }
    }
}

// Scroll Callback
void scroll_callback(GLFWwindow *window, double xOffset, double yOffset)
{
    camera.ProcessMouseScroll(yOffset);
}

// Generate Advanced Terrain with Multiple Layers of Perlin Noise
void generateAdvancedTerrain(int width, int height, std::vector<float> &vertices, std::vector<unsigned int> &indices, std::vector<float> &normals)
{
    float scale = 2.0f / (std::max(width, height) - 1);

    // Clear previous normals
    normals.resize(width * height * 3, 0.0f); // x, y, z normals for each vertex

    for (int z = 0; z < height; ++z)
    {
        for (int x = 0; x < width; ++x)
        {
            float xPos = (x * scale) - 0.5f;
            float zPos = (z * scale) - 0.5f;

            float heightValue = 0.0f;
            float amplitude = baseAmplitude;
            float frequency = baseFrequency;

            for (int octave = 0; octave < numOctaves; ++octave)
            {
                heightValue += amplitude * perlin.noise(xPos * frequency, zPos * frequency, numOctaves, persistence);
                amplitude *= persistence;
                frequency *= lacunarity;
            }

            vertices.push_back(xPos);
            vertices.push_back(heightValue); // Height
            vertices.push_back(zPos);

            // Texture coordinates
            vertices.push_back(static_cast<float>(x) / (width - 1));
            vertices.push_back(static_cast<float>(z) / (height - 1));

            // If not at the last row/column, generate indices for this quad
            if (x < width - 1 && z < height - 1)
            {
                int topLeft = z * width + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * width + x;
                int bottomRight = bottomLeft + 1;

                // Triangle 1
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // Triangle 2
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
    }

    // Calculate normals by averaging adjacent triangle normals
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        // Get vertex indices for this triangle
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        // Get vertex positions
        glm::vec3 v0(vertices[5 * idx0], vertices[5 * idx0 + 1], vertices[5 * idx0 + 2]);
        glm::vec3 v1(vertices[5 * idx1], vertices[5 * idx1 + 1], vertices[5 * idx1 + 2]);
        glm::vec3 v2(vertices[5 * idx2], vertices[5 * idx2 + 1], vertices[5 * idx2 + 2]);

        // Calculate two edges
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        // Calculate normal using cross product
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        // Add normal to each vertex of the triangle
        normals[3 * idx0] += normal.x;
        normals[3 * idx0 + 1] += normal.y;
        normals[3 * idx0 + 2] += normal.z;

        normals[3 * idx1] += normal.x;
        normals[3 * idx1 + 1] += normal.y;
        normals[3 * idx1 + 2] += normal.z;

        normals[3 * idx2] += normal.x;
        normals[3 * idx2 + 1] += normal.y;
        normals[3 * idx2 + 2] += normal.z;
    }

    // Normalize the normals for each vertex
    for (int i = 0; i < width * height; ++i)
    {
        glm::vec3 normal(normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]);
        normal = glm::normalize(normal);
        normals[3 * i] = normal.x;
        normals[3 * i + 1] = normal.y;
        normals[3 * i + 2] = normal.z;
    }
}

void generateWaterPlane(std::vector<float> &waterVertices, float width, float depth)
{
    waterVertices = {
        -0.5f * width, waterLevel, -0.5f * depth,
        0.5f * width, waterLevel, -0.5f * depth,
        -0.5f * width, waterLevel, 0.5f * depth,

        0.5f * width, waterLevel, -0.5f * depth,
        0.5f * width, waterLevel, 0.5f * depth,
        -0.5f * width, waterLevel, 0.5f * depth};
}

unsigned int compileShader(const char *shaderSource, GLenum shaderType)
{
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }
    return shader;
}

// Create Shader Program
unsigned int createShaderProgram()
{
    const char *vertexShaderSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;         // Position attribute
        layout(location = 1) in vec2 aTexCoord;    // Texture coordinate attribute
        layout(location = 2) in vec3 aNormal;      // Normal attribute

        out vec2 TexCoords;        // Pass texture coordinates to fragment shader
        out vec3 FragPos;          // Pass fragment position to fragment shader
        out vec3 Normal;           // Pass normal to fragment shader

        uniform mat4 transform;    // MVP matrix (combined model, view, projection)
        uniform mat4 model;        // Model matrix for normal transformation

        void main()
        {
            // Transform the vertex position
            FragPos = vec3(model * vec4(aPos, 1.0));
            
            // Correct the normals based on model transformation (transpose inverse)
            Normal = mat3(transpose(inverse(model))) * aNormal;

            // Pass through texture coordinates
            TexCoords = aTexCoord;

            // Apply the transform matrix (MVP) to compute final position
            gl_Position = transform * vec4(aPos, 1.0);
        }


    )glsl";

    const char *fragmentShaderSource = R"glsl(
        
        #version 330 core
        in vec2 TexCoords;         // Incoming texture coordinates
        in vec3 FragPos;           // Incoming fragment position (world space)
        in vec3 Normal;            // Incoming normal vector

        out vec4 FragColor;        // Final color output

        uniform vec3 lightPos;     // Light source position
        uniform vec3 viewPos;      // Camera position
        uniform sampler2D grassTexture;  // Grass texture
        uniform sampler2D rockTexture;   // Rock texture
        uniform sampler2D snowTexture;   // Snow texture

        void main()
        {
            // Phong lighting components
            vec3 ambientLight = vec3(0.3); // Ambient light strength

            // Normalized vectors for lighting calculations
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            vec3 viewDir = normalize(viewPos - FragPos);

            // Diffuse lighting (Lambert's cosine law)
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0);

            // Specular lighting (Phong reflection)
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = 0.5 * spec * vec3(1.0); // Specular strength

            // Combine ambient, diffuse, and specular lighting
            vec3 lighting = ambientLight + diffuse + specular;

            // Texture blending based on height
            vec4 grassColor = texture(grassTexture, TexCoords);
            vec4 rockColor = texture(rockTexture, TexCoords);
            vec4 snowColor = texture(snowTexture, TexCoords);

            // Basic height-based blending
            vec4 baseColor;
            if (FragPos.y < 0.3)
                baseColor = grassColor;
            else if (FragPos.y < 0.6)
                baseColor = mix(grassColor, rockColor, (FragPos.y - 0.3) / 0.3);
            else
                baseColor = mix(rockColor, snowColor, (FragPos.y - 0.6) / 0.4);
                

            // Final color is the product of lighting and base color
            FragColor = vec4(lighting, 1.0) * baseColor;
            // FragColor = vec4(normalize(Normal) * 0.5 + 0.5, 1.0); // This will map the normals to RGB colors

        }

        
    )glsl";

    unsigned int vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);

    // Shader Program linking
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Water Shader Program
unsigned int createWaterShaderProgram()
{
    const char *waterVertexShaderSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec3 aPos;

        uniform mat4 transform;
        uniform float time;

        out vec3 FragPos;    // To pass fragment position to fragment shader
        out vec3 Normal;     // To pass normal to fragment shader

        void main() {
            // Add a sine wave effect to simulate waves
            vec3 position = aPos;
            float waveAmplitude = 0.1; // Adjust amplitude of the waves
            float waveFrequency = 20.0; // Adjust frequency of the waves

            // Simulate water waves using sine function
            float wave = waveAmplitude * sin(position.x * waveFrequency + time) * cos(position.z * waveFrequency + time);
            position.y += wave;

            // Calculate partial derivatives to estimate the normal
            float waveDerivativeX = waveAmplitude * waveFrequency * cos(position.x * waveFrequency + time) * cos(position.z * waveFrequency + time);
            float waveDerivativeZ = -waveAmplitude * waveFrequency * sin(position.x * waveFrequency + time) * sin(position.z * waveFrequency + time);

            // Approximate the normal using the cross product of the partial derivatives
            vec3 tangentX = vec3(1.0, waveDerivativeX, 0.0); // Partial derivative in x direction
            vec3 tangentZ = vec3(0.0, waveDerivativeZ, 1.0); // Partial derivative in z direction
            Normal = normalize(cross(tangentZ, tangentX)); // Compute the normal by taking the cross product

            // Pass transformed position to fragment shader
            FragPos = vec3(transform * vec4(position, 1.0));
            gl_Position = transform * vec4(position, 1.0);
        }
    )glsl";

    const char *waterFragmentShaderSource = R"glsl(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;

        uniform vec4 waterColor;
        uniform vec3 viewPos;  // Camera position for Fresnel effect
        uniform vec3 lightPos; // Light position

        void main() {
            // Fresnel effect based on the angle between view and surface normal
            float fresnel = dot(normalize(viewPos - FragPos), normalize(Normal));
            fresnel = clamp(1.0 - fresnel, 0.0, 1.0);  // Control strength of reflection

            // Ambient lighting
            float ambientStrength = 0.2;
            vec3 ambient = ambientStrength * waterColor.rgb;

            // Diffuse lighting
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(Normal, lightDir), 0.0);
            vec3 diffuse = diff * waterColor.rgb;

            // Specular lighting
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, Normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * vec3(1.0); // White specular highlights

            // Final color with Fresnel effect and lighting
            vec3 finalColor = ambient + diffuse + specular;
            FragColor = vec4(mix(finalColor, vec3(1.0), fresnel * 0.2), 0.25); // Adjust alpha for transparency
        }
    )glsl";

    // Compile Vertex Shader
    unsigned int waterVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(waterVertexShader, 1, &waterVertexShaderSource, NULL);
    glCompileShader(waterVertexShader);

    // Check for Vertex Shader compilation errors
    int success;
    char infoLog[512];
    glGetShaderiv(waterVertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(waterVertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Compile Fragment Shader
    unsigned int waterFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(waterFragmentShader, 1, &waterFragmentShaderSource, NULL);
    glCompileShader(waterFragmentShader);

    // Check for Fragment Shader compilation errors
    glGetShaderiv(waterFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(waterFragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
    }

    // Link Shaders to Create the Shader Program
    unsigned int waterShaderProgram = glCreateProgram();
    glAttachShader(waterShaderProgram, waterVertexShader);
    glAttachShader(waterShaderProgram, waterFragmentShader);
    glLinkProgram(waterShaderProgram);

    // Check for Linking errors
    glGetProgramiv(waterShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(waterShaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    // Clean up Shaders as they're now linked into the program
    glDeleteShader(waterVertexShader);
    glDeleteShader(waterFragmentShader);

    return waterShaderProgram; // Return the water shader program ID
}

// Skybox Shader Program
unsigned int createSkyboxShaderProgram()
{
    const char *skyboxVertexShaderSource = R"glsl(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 TexCoords;

        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
            TexCoords = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww;  // Ignore the z component, always behind
        }
    )glsl";

    const char *skyboxFragmentShaderSource = R"glsl(
        #version 330 core
        in vec3 TexCoords;
        out vec4 FragColor;

        uniform samplerCube skybox;

        void main()
        {
            FragColor = texture(skybox, TexCoords);
        }
    )glsl";

    unsigned int vertexShader = compileShader(skyboxVertexShaderSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(skyboxFragmentShaderSource, GL_FRAGMENT_SHADER);

    // Link shaders into program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Load Texture
unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Setup Buffers
void setupBuffers(unsigned int &VAO, unsigned int &VBO, unsigned int &EBO, const std::vector<float> &vertices, const std::vector<float> &normals, const std::vector<unsigned int> &indices)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Create an interleaved vertex buffer (positions + normals + texture coordinates)
    std::vector<float> interleavedData;
    for (size_t i = 0; i < vertices.size() / 5; ++i)
    {
        interleavedData.push_back(vertices[5 * i]);     // x
        interleavedData.push_back(vertices[5 * i + 1]); // y
        interleavedData.push_back(vertices[5 * i + 2]); // z
        interleavedData.push_back(vertices[5 * i + 3]); // u (texture coordinate)
        interleavedData.push_back(vertices[5 * i + 4]); // v (texture coordinate)
        interleavedData.push_back(normals[3 * i]);      // normal x
        interleavedData.push_back(normals[3 * i + 1]);  // normal y
        interleavedData.push_back(normals[3 * i + 2]);  // normal z
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), &interleavedData[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0); // Position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float))); // TexCoords
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(5 * sizeof(float))); // Normals
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void setupWaterBuffers(unsigned int &waterVAO, unsigned int &waterVBO, const std::vector<float> &waterVertices)
{
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);

    glBindVertexArray(waterVAO);

    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, waterVertices.size() * sizeof(float), &waterVertices[0], GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// Check for OpenGL Errors
void checkOpenGLError()
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error: " << err << std::endl;
    }
}