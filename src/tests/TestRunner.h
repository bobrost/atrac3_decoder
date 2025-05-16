#pragma once

#include <string>
#include <vector>
#include <functional>

struct TestResult {
  TestResult(bool passed_) : passed(passed_), message("") {}
  TestResult(const std::string& message_) : passed(false), message(message_) {}
  TestResult(const char* message_) : passed(false), message(message_) {}
  operator bool() const { return passed; }
  bool passed = false;
  std::string message;
};

class TestRunner {
  public:
    using TestFunction = std::function<TestResult()>;
    void add(const std::string& name, TestFunction fn);
    void clear();
    bool runAll();
  private:
    struct TestEntry {
      std::string name;
      TestFunction fn;
    };
    std::vector<TestEntry> _tests;
};
