#include <iostream>
#include <chrono>
#include <unistd.h>
#include <thread>
#include <pthread.h>
#include <vector>
#include "gtest/gtest.h"
#include "rand_mat_funct.h"



//TEST(TestSuiteName, TestName) {
//  ... test body ...
//}




TEST(TestParams, TestNonZero) {
	ASSERT_GT(calculate_step(10), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
