#pragma once

namespace mpm {

    namespace detail {
        template <bool> struct STATIC_ASSERTION_FAILURE;
        template <> struct STATIC_ASSERTION_FAILURE<true> { enum { value = 1 }; };
        template <int> struct static_assert_helper{};
    }


    template <bool, typename T=void> struct enable_if{};
    template <typename T> struct enable_if<true, T> { typedef T type; };


    template <bool, typename T=void> struct disable_if{};
    template <typename T> struct disable_if<false, T> { typedef T type; };
}

#define MPM_JOIN(X, Y) __MPM_DO_JOIN(X, Y)
#define __MPM_DO_JOIN(X, Y) X##Y

#define MPM_STATIC_ASSERT(expr) \
    typedef detail::static_assert_helper<sizeof(detail::STATIC_ASSERTION_FAILURE<(bool)(expr)>)> \
            MPM_JOIN(Line_, __LINE__)

#define MPM_CAS(val, expected, new_val) \
    __sync_bool_compare_and_swap(val, expected, new_val);

#define MPM_EXCHG(storage, value) \
    __sync_lock_test_and_set(storage, value);

#define MPM_DISALLOW_COPY_AND_ASSIGN(Type) \
    Type(const Type&); \
    void operator=(const Type&)
