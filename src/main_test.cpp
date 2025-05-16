#include "tests/TestRunner.h"

void addQmfTests(TestRunner&);
void addDctTests(TestRunner&);
void addFftTests(TestRunner&);
void addAtracDecodeTests(TestRunner&);

int main() {
  TestRunner runner;
  addQmfTests(runner);
  addDctTests(runner);
  addFftTests(runner);
  addAtracDecodeTests(runner);
  bool ok = runner.runAll();
  return (ok ? 0 : -1);
}
