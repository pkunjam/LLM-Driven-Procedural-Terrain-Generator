#include <curl/curl.h>
#include "UserPrompt.h"
#include <iostream>
#include <string>

// Define terrain parameters
extern float baseFrequency;
extern float baseAmplitude;
extern int numOctaves;

// Function to get user input
std::string getUserPrompt() {
    std::string prompt;
    std::cout << "Enter a prompt to modify the terrain (e.g., 'increase roughness', 'lower height'): ";
    std::getline(std::cin, prompt);
    return prompt;
}

// Callback function to handle the response from the API
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s)
{
    s->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// Function to send a prompt to an LLM API (like GPT-4)
std::string sendPromptToLLM(const std::string &prompt)
{
    CURL *curl;
    CURLcode res;
    std::string responseString;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
        // Set the API endpoint for Cohere's text generation
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.cohere.ai/generate");

        // Set the POST request headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "Authorization: Bearer uVe9e2pzntvOTR4fcptKQvMbBL6z9LVl2FIE8JFW"); // Replace with your API key
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // System message to provide context about the project and codebase
        std::string systemMessage = "You are helping a developer modify terrain generation parameters in a C++ OpenGL project. "
                                    "The developer has defined commands like 'increase roughness', 'lower height', and 'reset'. "
                                    "When the developer asks a question, respond ONLY with one of these commands. "
                                    "No additional information is required in your response.";

        // Set the request body with the system message and user prompt, using Cohere's format
        std::string jsonBody = "{"
                               "\"model\": \"command-xlarge-nightly\","
                               "\"prompt\": \"" +
                               systemMessage + " User: " + prompt + "\","
                                                                    "\"max_tokens\": 50"
                                                                    "}";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());

        // Set the callback to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        // Perform the request
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();

    return responseString;
}

// Function to modify terrain parameters based on user prompt
void handleUserPrompt(const std::string &userPrompt)
{
    // Send the prompt to the LLM and get the response
    std::string llmResponse = sendPromptToLLM(userPrompt);

    // Print the full LLM response for debugging purposes
    std::cout << "LLM Response: " << llmResponse << std::endl;

    // Look for key phrases in the response
    if (llmResponse.find("increase roughness") != std::string::npos)
    {
        baseFrequency += 0.1f;
        std::cout << "Increasing roughness: Frequency is now " << baseFrequency << std::endl;
    }
    else if (llmResponse.find("lower height") != std::string::npos)
    {
        baseAmplitude -= 0.1f;
        std::cout << "Lowering terrain height: Amplitude is now " << baseAmplitude << std::endl;
    }
    else if (llmResponse.find("more detail") != std::string::npos)
    {
        numOctaves += 1;
        std::cout << "Increasing terrain detail: Octaves is now " << numOctaves << std::endl;
    }
    else if (llmResponse.find("reset") != std::string::npos)
    {
        baseFrequency = 0.4f;
        baseAmplitude = 0.5f;
        numOctaves = 4;
        std::cout << "Resetting terrain parameters to default values." << std::endl;
    }
    else
    {
        std::cout << "LLM Command not recognized or understood: " << llmResponse << std::endl;
    }
}
