#include <iostream>
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"

int main()
{
    std::string greeting = absl::StrCat("Hello", ", ", "World", "!");
    std::string formatted = absl::StrFormat("Message: %s", greeting);
    std::cout << formatted << std::endl;
    
    // Also demonstrate some Abseil string utilities
    std::cout << absl::StrFormat("Abseil version is working! 🎉") << std::endl;
    
    return 0;
}