#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

class CommandLineOptionsParser {
  public:
    using StringArray = std::vector<std::string>;
    using CstrFunction = std::function<void(const char*)>;
    using VoidFunction = std::function<void()>;

    bool parse(int argn, char** argv);
    void printHelp();

    // Add a flag with parameter
    void add(const StringArray& flags, std::string& targetParameter, const std::string& helpDescription);
    // Add a no-parameter flag that triggers a callback
    void add(const StringArray& flags, VoidFunction callback, const std::string& helpDescription);

  private:
    void addHelp(const StringArray& flags, const std::string& helpDescription);
    struct Option {
      bool hasParam = false;
      CstrFunction paramCallback;
      VoidFunction noParamCallback;
    };
    std::vector<std::string> _help;
    std::unordered_map<std::string, Option> _options;
};
