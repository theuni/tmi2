#include <test/hashed_non_unique.h>
#include <test/hashed_unique.h>
#include <test/ordered_unique.h>
#include <test/ordered_non_unique.h>

int main()
{
    test_hashed_non_unique();
    test_hashed_unique();
    test_ordered_unique();
    test_ordered_non_unique();
}
