#include "UserPrompt.h"
#include <iostream>


std::string getUserPrompt() {
    std::string prompt;
    std::cout << "Enter a prompt to modify the terrain (e.g., 'increase roughness', 'lower height'): ";
    std::getline(std::cin, prompt);
    return prompt;
}
