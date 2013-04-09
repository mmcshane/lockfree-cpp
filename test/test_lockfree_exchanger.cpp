#include "mpm/lockfree_exchanger.hpp"
#include "catch.hpp"
#include <pthread.h>


typedef mpm::lockfree_exchanger<int> int_exchanger;


struct exchange_data
{
    int_exchanger& exchanger;
    int_exchanger::ptr_type in;
    int_exchanger::ref_ptr_type out;
    unsigned int timeout;
};


void* exchange(void* thread_data)
{
    exchange_data* data(static_cast<exchange_data*>(thread_data));
    data->exchanger.exchange(data->in, data->out, data->timeout);
    return NULL;
}


TEST_CASE("mpm/lockfree_exchanger/successful_exchange",
          "Test a successful pointer exchange")
{
    int seven = 7;
    int eight = 8;
    int zero  = 0;

    int * main_thread_in  = &seven;
    int * main_thread_out = &zero;
    int * bg_thread_in    = &eight;
    int * bg_thread_out   = &zero;

    int_exchanger lfe;
    exchange_data edata = {lfe, bg_thread_in, bg_thread_out, 1000000000 };
    pthread_t thread;
    REQUIRE(0 == pthread_create(&thread, NULL, &exchange, &edata));

    REQUIRE(lfe.exchange(main_thread_in, main_thread_out, 1000000000));

    CHECK(7 == seven);
    CHECK(8 == eight);
    CHECK(0 == zero);

    CHECK(7 == *main_thread_in);
    CHECK(main_thread_in == bg_thread_out);
    CHECK(7 == *bg_thread_out);

    CHECK(8 == *bg_thread_in);
    CHECK(bg_thread_in == main_thread_out);
    CHECK(8 == *main_thread_out);
}


TEST_CASE("mpm/lockfree_exchanger/missed_connection",
          "Test a failed exchange")
{
    int main_thread_in = 7;
    int main_thread_out = 0;
    int* main_thread_out_ptr = &main_thread_out;

    int_exchanger lfe;
    CHECK_FALSE(lfe.exchange(&main_thread_in, main_thread_out_ptr, 1));

    CHECK(7 == main_thread_in);
    CHECK(0 == main_thread_out);
}
