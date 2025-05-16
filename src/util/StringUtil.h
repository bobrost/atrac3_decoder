#pragma once
#include <string>
#include <vector>

// Printf-style formatting to output to a std::string
std::string string_format(const char* format, ...);

std::string stringify(const std::vector<std::string>& values);
