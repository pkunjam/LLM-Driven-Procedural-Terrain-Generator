// ChatInterface.cpp
#include <glad/glad.h>    // Must come first
#include <GLFW/glfw3.h>   // Then GLFW
#include "ChatInterface.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <curl/curl.h>

// We declare two external functions (implemented in main.cpp) that allow the chat module
// to trigger terrain updates.
extern void updateTerrainExternally(int numOctaves, float persistence, float lacunarity, float baseAmplitude, float baseFrequency);
extern void undoTerrainChangeExternally();

// A simple WriteCallback for libcurl.
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

ChatInterface::ChatInterface()
    : scrollToBottom(false)
{
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
    // Initialize function definitions (as in your original main.cpp).
    functionDefinitions = nlohmann::json::array({
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
                }
            }
        }
    });
    
    initializeConversationHistory();
}

ChatInterface::~ChatInterface()
{
}

void ChatInterface::initialize(GLFWwindow* window)
{
    // Initialize ImGui.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Allocate a new ImGuiTextBuffer (or you can simply use a std::string).
    pChatHistory = new ImGuiTextBuffer();
}

void ChatInterface::render()
{
    // Start a new ImGui frame.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Once);
    
    ImGui::Begin("Terrain Assistant", nullptr, ImGuiWindowFlags_NoCollapse);
    
    ImGui::BeginChild("ChatHistory", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(pChatHistory->begin());
    if (scrollToBottom)
        ImGui::SetScrollHereY(1.0f);
    scrollToBottom = false;
    ImGui::EndChild();
    
    ImGui::Separator();
    
    ImGui::PushItemWidth(-40);
    if (ImGui::InputText("##Input", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (strlen(inputBuffer) > 0)
        {
            pChatHistory->append("User: ");
            pChatHistory->append(inputBuffer);
            pChatHistory->append("\n\n");
            
            std::string userInput(inputBuffer);
            
            if (userInput == "undo" || userInput == "revert")
            {
                undoTerrainChange();
                conversationHistory.push_back({{"role", "user"}, {"content", userInput}});
                conversationHistory.push_back({{"role", "assistant"}, {"content", "Reverted to previous terrain state."}});
            }
            else
            {
                std::string response = sendOpenAIRequest(userInput);
                if (!response.empty())
                {
                    try
                    {
                        nlohmann::json functionCall = parseOpenAIResponse(response);
                        invokeTerrainFunction(functionCall);
                    }
                    catch (const std::exception& e)
                    {
                        pChatHistory->append("Assistant: Error - ");
                        pChatHistory->append(e.what());
                        pChatHistory->append("\n");
                        scrollToBottom = true;
                    }
                }
            }
            memset(inputBuffer, 0, sizeof(inputBuffer));
            scrollToBottom = true;
        }
    }
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    if (ImGui::Button("Send"))
    {
        if (strlen(inputBuffer) > 0)
        {
            pChatHistory->append("User: ");
            pChatHistory->append(inputBuffer);
            pChatHistory->append("\n");
            
            std::string userInput(inputBuffer);
            if (userInput == "undo" || userInput == "revert")
            {
                undoTerrainChange();
                conversationHistory.push_back({{"role", "user"}, {"content", userInput}});
                conversationHistory.push_back({{"role", "assistant"}, {"content", "Reverted to previous terrain state."}});
            }
            else
            {
                std::string response = sendOpenAIRequest(userInput);
                if (!response.empty())
                {
                    try
                    {
                        nlohmann::json functionCall = parseOpenAIResponse(response);
                        invokeTerrainFunction(functionCall);
                    }
                    catch (const std::exception& e)
                    {
                        pChatHistory->append("Assistant: Error - ");
                        pChatHistory->append(e.what());
                        pChatHistory->append("\n");
                        scrollToBottom = true;
                    }
                }
            }
            memset(inputBuffer, 0, sizeof(inputBuffer));
            scrollToBottom = true;
        }
    }
    
    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ChatInterface::cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (pChatHistory) { delete pChatHistory; pChatHistory = nullptr; }
}

std::string ChatInterface::getAPIKey()
{
    const char* apiKey = std::getenv("OPENAI_API_KEY");
    if (!apiKey)
    {
        std::cerr << "Error: OPENAI_API_KEY environment variable not set." << std::endl;
        exit(1);
    }
    return std::string(apiKey);
}

std::string ChatInterface::buildSystemPrompt()
{
    std::string systemPrompt = R"(
You are an assistant integrated into a procedural terrain generation system built using C++ and OpenGL.
The system uses several terrain parameters, and the user input determines how the terrain is modified. Your task is to interpret natural
language inputs and adjust the terrain parameters accordingly, making moderate adjustments based on the user's intent.

The terrain is generated using Perlin noise. The parameters you need to adjust based on user input are:

- numOctaves (Integer): Controls the number of noise octaves.
- persistence (Float): Controls the amplitude decay.
- lacunarity (Float): Controls the frequency increase.
- baseAmplitude (Float): Determines the overall height variation.
- baseFrequency (Float): Controls the overall scale of terrain features.

When adjusting parameters, make moderate changes based on the user's input, unless the user explicitly requests significant changes. Avoid changing parameters by large amounts unless necessary.
You will extract terrain parameters from user input and call the updateTerrain function accordingly. Do not provide any explanations or additional text.

)";
    return systemPrompt;
}

void ChatInterface::initializeConversationHistory()
{
    nlohmann::json systemMessage = {
        {"role", "system"},
        {"content", buildSystemPrompt()}
    };
    conversationHistory.push_back(systemMessage);
}

void ChatInterface::truncateConversationHistory()
{
    const size_t maxMessages = 20;
    if (conversationHistory.size() > maxMessages)
    {
        conversationHistory.erase(conversationHistory.begin() + 1, conversationHistory.begin() + 3);
    }
}

std::string ChatInterface::sendOpenAIRequest(const std::string& userInput)
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
        
        conversationHistory.push_back({{"role", "user"}, {"content", userInput}});
        
        nlohmann::json jsonPayload;
        jsonPayload["model"] = "o3-mini";
        jsonPayload["reasoning_effort"] = "high";
        jsonPayload["messages"] = conversationHistory;
        jsonPayload["functions"] = functionDefinitions;
        jsonPayload["function_call"] = "auto";
        
        std::string payload = jsonPayload.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        res = curl_easy_perform(curl);
        
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

nlohmann::json ChatInterface::parseOpenAIResponse(const std::string& response)
{
    nlohmann::json jsonResponse = nlohmann::json::parse(response);
    auto message = jsonResponse["choices"][0]["message"];
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

void ChatInterface::invokeTerrainFunction(const nlohmann::json& functionCall)
{
    std::string functionName = functionCall["function_name"];
    nlohmann::json args = functionCall["arguments"];
    
    if (functionName == "updateTerrain")
    {
        int newNumOctaves   = args.value("numOctaves", 4);
        float newPersistence  = args.value("persistence", 0.5f);
        float newLacunarity   = args.value("lacunarity", 2.0f);
        float newBaseAmplitude = args.value("baseAmplitude", 0.5f);
        float newBaseFrequency = args.value("baseFrequency", 0.4f);
        
        // (You can add any delta–limiting or clamping here as in your original code.)
        updateTerrainExternally(newNumOctaves, newPersistence, newLacunarity, newBaseAmplitude, newBaseFrequency);
        
        std::ostringstream oss;
        oss << "Assistant: Terrain parameters updated.\n\n";
        oss << "Current Terrain Parameters:\n";
        oss << "Number of Octaves: " << newNumOctaves << "\n";
        oss << "Persistence: " << newPersistence << "\n";
        oss << "Lacunarity: " << newLacunarity << "\n";
        oss << "Base Amplitude: " << newBaseAmplitude << "\n";
        oss << "Base Frequency: " << newBaseFrequency << "\n";
        
        pChatHistory->append(oss.str().c_str());
        scrollToBottom = true;
    }
    else
    {
        std::cerr << "Unknown function called: " << functionName << std::endl;
    }
}

void ChatInterface::undoTerrainChange()
{
    undoTerrainChangeExternally();
    pChatHistory->append("Assistant: Reverted to previous terrain state.\n");
    scrollToBottom = true;
}
