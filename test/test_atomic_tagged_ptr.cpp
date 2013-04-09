#include "mpm/atomic_tagged_ptr.hpp"
#include "catch.hpp"

int x(12), y(13);

TEST_CASE("mpm/atomic_tagged_ptr/nullary_construct",
          "Nullary constructor should make a null pointer")
{
    mpm::atomic_tagged_ptr<int> ptr;
    uint16_t tag = -1;
    int* ptrval(ptr.get(tag));
    CHECK(ptrval == 0);
    CHECK(tag == 0);
}


TEST_CASE("mpm/atomic_tagged_ptr/tag_too_large",
          "Test that a tag size > 2 bytes does not compile")
{
    //doesn't compile
    //mpm::atomic_tagged_ptr<int, int32_t> ptr;
}


TEST_CASE("mpm/atomic_tagged_ptr/binary_construct",
          "Construct with a pointer and tag value")
{
    mpm::atomic_tagged_ptr<int> ptr(&x, x);
    uint16_t tag = -1;
    int* ptrval(ptr.get(tag));
    CHECK(ptrval == &x);
    CHECK(tag == x);
}


TEST_CASE("mpm/atomic_tagged_ptr/set",
          "Set the pointer and tag value via set()")
{
    mpm::atomic_tagged_ptr<int> ptr;
    ptr.set(&x, x);
    uint16_t tag = -1;
    int* ptrval(ptr.get(tag));
    CHECK(ptrval == &x);
    CHECK(tag == x);
}


TEST_CASE("mpm/atomic_tagged_ptr/cas_failure_for_pointer",
          "Attempting a CAS with the wrong pointer fails")
{
    mpm::atomic_tagged_ptr<int> ptr(&x, x);
    CHECK_FALSE(ptr.compare_and_swap(&y, &y, x, y)); //expected ptr does not match
}


TEST_CASE("mpm/atomic_tagged_ptr/cas_failure_for_tag",
          "Attempting a CAS with the wrong tag fails")
{
    mpm::atomic_tagged_ptr<int> ptr(&x, x);
    CHECK_FALSE(ptr.compare_and_swap(&x, &y, y, y)); //expected tag does not match
}


TEST_CASE("mpm/atomic_tagged_ptr/cas_success",
          "CAS works with the right expected values")
{
    mpm::atomic_tagged_ptr<int> ptr(&x, x);
    REQUIRE(ptr.compare_and_swap(&x, &y, x, y));
    uint16_t tag = -1;
    int* ptrval(ptr.get(tag));
    CHECK(ptrval == &y);
    CHECK(tag == y);

}
