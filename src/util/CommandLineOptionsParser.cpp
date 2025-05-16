#include "CommandLineOptionsParser.h"
#include "StringUtil.h"
#include "Logging.h"

bool CommandLineOptionsParser::parse(int argn, char** argv) {
  constexpr const char* kLogCategory = "Options";
  for (int i=1; i<argn; ++i) {
    const char* curr = argv[i];
    const char* next = (i+1 < argn ? argv[i+1] : nullptr);
    auto iter = _options.find(curr);
    if (iter == _options.end()) {
      LogError(kLogCategory, "Unknown option: %s", curr);
      return false;
    }
    if (!iter->second.hasParam) {
      iter->second.noParamCallback();
    } else if (!next) {
      LogError(kLogCategory, "Option '%s' missing parameter", curr);
      return false;
    } else {
      iter->second.paramCallback(next);
      ++i;
    }
  }
  return true;
}

void CommandLineOptionsParser::printHelp() {
  printf("Usage:\n");
  for (const std::string& line : _help) {
    printf("  %s\n", line.c_str());
  }
}

void CommandLineOptionsParser::add(const StringArray& flags, std::string& targetParameter, const std::string& helpDescription) {
  addHelp(flags, helpDescription);
  Option o;
  o.hasParam = true;
  o.paramCallback = [&](const char* s){targetParameter = s;};
  for (const std::string& flag : flags) {
    _options[flag] = o;
  }
}

void CommandLineOptionsParser::add(const StringArray& flags, VoidFunction callback, const std::string& helpDescription) {
  addHelp(flags, helpDescription);
  Option o;
  o.hasParam = false;
  o.noParamCallback = callback;
  for (const std::string& flag : flags) {
    _options[flag] = o;
  }
}

void CommandLineOptionsParser::addHelp(const StringArray& flags, const std::string& helpDescription) {
  _help.push_back(string_format("%s : %s", stringify(flags).c_str(), helpDescription.c_str()));
}
