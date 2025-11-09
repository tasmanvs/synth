#include <iostream>
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"

int main()
{
    // Initialize Abseil logging
    absl::InitializeLog();
    
    // Set stderr threshold to INFO so logs are visible
    absl::SetStderrThreshold(absl::LogSeverity::kInfo);
    
    LOG(INFO) << "Starting hello_world application";
    
    std::string greeting = absl::StrCat("Hello", ", ", "World", "!");
    std::string formatted = absl::StrFormat("Message: %s", greeting);
    std::cout << formatted << std::endl;
    
    LOG(INFO) << "Greeting created: " << greeting;
    
    // Also demonstrate some Abseil string utilities
    std::cout << absl::StrFormat("Abseil version is working! 🎉") << std::endl;
    
    LOG(INFO) << "Application completed successfully";
    
    return 0;
}