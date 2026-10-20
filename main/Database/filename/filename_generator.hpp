#pragma once
#include <string>
#include <typeinfo>
#ifndef _MSC_VER
#include <cxxabi.h> // Needed for demangling class names on GCC/Clang (ESP32 uses GCC)
#endif
namespace file {


// Helper template to automatically generate a unique filename based on the class type
template <typename T>
std::string generateClassFileName(const T& instance) {
    int status = 0;
    const char* mangledName = typeid(instance).name();
    std::string className;

#if defined(_MSC_VER)
    // MSVC already returns a readable name
    className = mangledName;
#else
    // GCC / Clang (ESP32) returns a mangled name that needs demangling
    char* demangled = abi::__cxa_demangle(mangledName, nullptr, nullptr, &status);
    if (status == 0 && demangled) {
        className = demangled;
        free(demangled);
    } else {
        className = mangledName;
    }
#endif

    // Replace characters that are invalid or messy in file systems (like "::" or whitespace)
    // e.g., "file::fileVector" becomes "file_fileVector"
    for (char& c : className) {
        if (c == ':' || c == ' ' || c == '<' || c == '>' || c == '*') {
            c = '_';
        }
    }

    return className + ".json";
}












}