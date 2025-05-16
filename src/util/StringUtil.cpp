#include "StringUtil.h"

// Printf-style formatting to output to a std::string
std::string string_format(const char* format, ...) {
  // Initialize a variable argument list
  va_list args;
  va_start(args, format);

  // Use a buffer to estimate the size of the formatted string
  std::vector<char> buffer(1024);
  int result = vsnprintf(buffer.data(), buffer.size(), format, args);

  // If the buffer is too small, resize and reformat
  if (result >= static_cast<int>(buffer.size())) {
    buffer.resize(result + 1); // Add space for the null terminator
    result = vsnprintf(buffer.data(), buffer.size(), format, args);
  }

  va_end(args);

  // Check for errors during formatting
  if (result < 0) {
    throw std::runtime_error("Error formatting string");
  }

  // Return the formatted string
  return std::string(buffer.data(), static_cast<size_t>(result));
}

std::string stringify(const std::vector<std::string>& values) {
  std::string result;
  for (const std::string& v : values) {
    if (!result.empty()) {
      result += ", ";
    }
    result += v;
  }
  return result;
}
