#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include "TestRunner.h"

void TestRunner::add(const std::string& name, TestFunction fn) {
  _tests.push_back({name, fn});
}

void TestRunner::clear() {
  _tests.clear();
}

bool TestRunner::runAll() {
  constexpr const char* kCheckMark = "  ✓"; // U+2713
  constexpr const char* kXMark = "✗  "; // U+2717
  int numPassed = 0;
  int numFailed = 0;
  int total = static_cast<int>(_tests.size());
  
  printf("Running %d tests...\n\n", total);
  
  for (const auto& test : _tests) {
    try {
      auto start = std::chrono::high_resolution_clock::now();
      TestResult result = test.fn();
      auto end = std::chrono::high_resolution_clock::now();
      auto durationUs = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

      constexpr const char* kUnitsUs = "μs";
      constexpr const char* kUnitsMs = "ms";
      uint64_t duration = durationUs.count();
      const char* units = kUnitsUs;
      if (duration > 1000) {
        duration /= 1000;
        units = kUnitsMs;
      }
      
      if (result) {
        numPassed++;
        printf("%s %s (%lld%s)\n", kCheckMark, test.name.c_str(), duration, units);
      } else {
        numFailed++;
        printf("%s %s (%s) (%lld%s)\n", kXMark, test.name.c_str(), result.message.c_str(), duration, units);
      }
    } catch (const std::exception& e) {
      printf("%s %s (Exception: %s)\n", kXMark, test.name.c_str(), e.what());
    }
  }  
  printf("\nTest Results: %d passed, %d failed\n", numPassed, numFailed);
  
  if (numFailed == 0) {
    printf("All tests passed!\n");
  } else {
    printf(" FAILURE: %d test(s) failed.\n", numFailed);
  }
  return (numFailed == 0);
}
