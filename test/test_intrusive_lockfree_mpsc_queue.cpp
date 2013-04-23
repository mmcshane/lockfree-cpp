#include "mpm/intrusive_lockfree_mpsc_queue.hpp"
#include "catch.hpp"
#include <pthread.h>
#include <vector>

namespace {

    struct entry : mpm::intrusive_lockfree_mpsc_queue_entry<entry>
    {
        entry() throw() : value(0) {}
        entry(int _value) : value(_value) {}

        unsigned int value;
    };

    bool entry_ptr_less(entry* lhs, entry* rhs)
    {
        return lhs->value < rhs->value;
    }

    struct producer_data
    {
        mpm::intrusive_lockfree_mpsc_queue<entry>* queue;
        pthread_barrier_t* start_barrier;
        pthread_barrier_t* producers_done_barrier;
        std::vector<entry>* entries;
        int start;
        int producers;
    };


    void* producer(void* in)
    {
        producer_data* data(static_cast<producer_data*>(in));
        pthread_barrier_wait(data->start_barrier);
        for(unsigned int i = data->start; i < data->entries->size(); i += data->producers)
            data->queue->push(data->entries->at(i));
        pthread_barrier_wait(data->producers_done_barrier);
        return 0;
    }


    struct consumer_data
    {
        mpm::intrusive_lockfree_mpsc_queue<entry>* queue;
        entry* poison;
        std::vector<entry*>* consumed;
        pthread_barrier_t* start_barrier;
    };


    void* consumer(void* in)
    {
        consumer_data* data(static_cast<consumer_data*>(in));
        pthread_barrier_wait(data->start_barrier);
        entry* popped(NULL);
        while(true)
        {
            popped = data->queue->pop();
            if(popped == data->poison)
                return 0;
            if(popped)
               data->consumed->push_back(popped);
        }
        return 0;
    }
}

TEST_CASE("mpm/intrusive_lockfree_mpsc_queue/push_pop",
          "Simple push and pop")
{
    entry e0(0), e1(1), e2(2);
    mpm::intrusive_lockfree_mpsc_queue<entry> queue;

    queue.push(e0);
    queue.push(e1);
    queue.push(e2);

    CHECK(0 == queue.pop()->value);
    CHECK(1 == queue.pop()->value);
    CHECK(2 == queue.pop()->value);

    CHECK(0 == queue.pop());
}


TEST_CASE("mpm/intrusive_lockfree_mpsc_queue/go_like_hell",
          "Concurrent pushing and popping")
{
    static int nthreads = 8;
    static int nentries = 100000;

    mpm::intrusive_lockfree_mpsc_queue<entry> queue;
    std::vector<entry> entries(nentries);
    for(int i = 0; i < nentries; i++)
        entries[i].value = i;

    pthread_barrier_t start_barrier, producers_done_barrier;
    REQUIRE(0 == pthread_barrier_init(&start_barrier, NULL, nthreads + 1)); //producers + consumer
    REQUIRE(0 == pthread_barrier_init(&producers_done_barrier, NULL, nthreads + 1)); //producers + consumer
    entry consumer_poison;
    std::vector<entry*> consumed;
    consumed.reserve(entries.size());

    pthread_t producer_threads[nthreads];
    producer_data producer_data_arr[nthreads];
    for(int i = 0; i < nthreads; i++)
    {
        producer_data_arr[i].queue = &queue;
        producer_data_arr[i].start_barrier = &start_barrier;
        producer_data_arr[i].producers_done_barrier = &producers_done_barrier;
        producer_data_arr[i].entries = &entries;
        producer_data_arr[i].start = i;
        producer_data_arr[i].producers = nthreads;
        REQUIRE(0 == pthread_create(
            &producer_threads[i], NULL, &producer, &producer_data_arr[i]));
    }

    pthread_t consumer_thread;
    consumer_data consumer_d = { &queue, &consumer_poison, &consumed, &start_barrier };
    REQUIRE(0 == pthread_create(&consumer_thread, NULL, &consumer, &consumer_d));

    //block until producers finish
    pthread_barrier_wait(&producers_done_barrier);

    //producers have stopped, signal the consumer
    queue.push(consumer_poison);

    //join all the producers
    for(int i = 0; i < nthreads; i++)
        REQUIRE(0 == pthread_join(producer_threads[i], NULL));

    //consumer will exit when it pops poison
    REQUIRE(0 == pthread_join(consumer_thread, NULL));

    //all child threads have completed

    REQUIRE(entries.size() == consumed.size());

    std::sort(consumed.begin(), consumed.end(), &entry_ptr_less);
    for(unsigned int i = 0; i < consumed.size(); i++)
        CHECK(i == consumed[i]->value);
}

