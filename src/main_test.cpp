#include "tests/TestRunner.h"

void addQmfTests(TestRunner&);
void addDctTests(TestRunner&);
void addAtracDecodeTests(TestRunner&);

int main() {
  TestRunner runner;
  addQmfTests(runner);
  addDctTests(runner);
  addAtracDecodeTests(runner);
  bool ok = runner.runAll();
  return (ok ? 0 : -1);
}
