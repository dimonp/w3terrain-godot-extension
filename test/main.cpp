#include "gtest/gtest.h"

// Explicitly define the main function, since linking to GTest::gtest_main doesn't work well with MSVC
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv); // Parses command-line flags
  return RUN_ALL_TESTS(); // Runs all registered tests
}