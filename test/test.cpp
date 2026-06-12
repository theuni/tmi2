#include <test/hashed_non_unique.h>
#include <test/hashed_unique.h>
#include <test/ordered_non_unique.h>
#include <test/ordered_unique.h>

#include <test/stdset.h>
#include <test/std_unordered_set.h>

#include <string_view>

enum ExitStatus { Success = 0, Failure = 1 };

int main(int argc, char *argv[]) {
  if (argc < 2) {
    test_hashed_non_unique();
    test_hashed_unique();
    test_ordered_unique();
    test_ordered_non_unique();

    test_std_unordered();
    test_std_set();
    return ExitStatus::Success;
  }

  std::string_view name{argv[1]};
  if (name == "hashed_non_unique") {
    test_hashed_non_unique();
    return ExitStatus::Success;
  }
  if (name == "hashed_unique") {
    test_hashed_unique();
    return ExitStatus::Success;
  }
  if (name == "ordered_unique") {
    test_ordered_unique();
    return ExitStatus::Success;
  }
  if (name == "ordered_non_unique") {
    test_ordered_non_unique();
    return ExitStatus::Success;
  }
  if (name == "std_unordered") {
    test_std_unordered();
    return ExitStatus::Success;
  }
  if (name == "std_set") {
    test_std_set();
    return ExitStatus::Success;
  }
  return ExitStatus::Failure;
}
