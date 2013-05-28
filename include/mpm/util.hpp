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


#define JOIN(X, Y) DO_JOIN(X, Y)
#define DO_JOIN(X, Y) X##Y


#define static_assert(expr, msg) \
    typedef detail::static_assert_helper<sizeof(detail::STATIC_ASSERTION_FAILURE<(bool)(expr)>)> \
            JOIN(Line_, __LINE__)


#define DISALLOW_COPY_AND_ASSIGN(Type) \
    Type(const Type&); \
    void operator=(const Type&)


#if defined (__x86_64__)
    struct rdtsc
    {
        typedef unsigned long long result_type;

        inline result_type operator()()
        {
            unsigned hi, lo;
            __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
            return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
        }
    };
#endif

}


