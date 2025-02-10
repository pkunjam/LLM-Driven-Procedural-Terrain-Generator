#pragma once
#include <GLFW/glfw3.h>  // Add this at the top of ChatInterface.h

#ifndef CHATINTERFACE_H
#define CHATINTERFACE_H

#include <string>
#include <vector>
#include "json.hpp"  // nlohmann::json

// Forward declarations for ImGui types (if needed)
struct ImGuiTextBuffer;

class ChatInterface
{
public:
    ChatInterface();
    ~ChatInterface();

    // Initializes ImGui and conversation history.
    void initialize(GLFWwindow* window);
    
    // Renders the chat window and processes user input.
    void render();
    
    // Cleans up ImGui resources.
    void cleanup();
    

private:
    // ImGui state variables.
    ImGuiTextBuffer* pChatHistory; // You can use an ImGuiTextBuffer (or std::string) to store the chat text.
    char inputBuffer[256];
    bool scrollToBottom;
    
    // Conversation history for LLM context.
    std::vector<nlohmann::json> conversationHistory;
    
    // Function definitions for function-calling.
    nlohmann::json functionDefinitions;
    
    // Internal helper functions.
    std::string sendOpenAIRequest(const std::string& userInput);
    nlohmann::json parseOpenAIResponse(const std::string& response);
    void invokeTerrainFunction(const nlohmann::json& functionCall);
    void undoTerrainChange();
    
    std::string getAPIKey();
    std::string buildSystemPrompt();
    void initializeConversationHistory();
    void truncateConversationHistory();
};

#endif // CHATINTERFACE_H
