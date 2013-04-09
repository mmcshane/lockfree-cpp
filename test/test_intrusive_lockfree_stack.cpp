#include "mpm/intrusive_lockfree_stack.hpp"
#include "catch.hpp"
#include <pthread.h>


struct entry : mpm::intrusive_lockfree_stack_entry<entry>
{
    int value;
    entry() : value(0) {}
    entry(int _value) : value(_value) {}
};


struct orthogonal_entry
{
    orthogonal_entry() : next(0), value(0) {}
    orthogonal_entry(int _value) : next(0), value(_value) {}
    orthogonal_entry* next;
    int value;
};


void mpm_lfs_set_next(orthogonal_entry& n, orthogonal_entry* next)
{
    n.next = next;
}


orthogonal_entry* mpm_lfs_get_next(orthogonal_entry const& n)
{
    return n.next;
}


struct pushpop_data
{
    //use a smaller array with more spins to make elimination more likely
    typedef mpm::intrusive_lockfree_stack<
        entry, mpm::elimination_opts<2, 10000, 1> > stack_type;

    pthread_barrier_t* barrier;
    unsigned int iterations;
    stack_type* stack;
};


void* pushpop(void* in)
{
    pushpop_data* data(static_cast<pushpop_data*>(in));
    pthread_barrier_wait(data->barrier);

    for(unsigned int i = 0; i < data->iterations; i++)
    {
        entry* popped(data->stack->pop());
        data->stack->push(*popped);
    }
    return NULL;
}


template <typename OutputIterator, typename Stack>
OutputIterator drain(Stack & stack, OutputIterator out)
{
    typename Stack::pointer ptr;
    while((ptr = stack.pop()))
        *out++ = ptr;
    return out;
}



TEST_CASE("mpm/intrusive_lockfree_stack/push_pop",
          "A stack must have LIFO nature")
{
    mpm::intrusive_lockfree_stack<entry> lfs;
    entry zero(0), one(1), two(2), three(3), four(4);
    lfs.push(four);
    lfs.push(three);
    lfs.push(two);
    lfs.push(one);
    lfs.push(zero);

    for(int i = 0; i < 5; i++)
    {
        entry* popped(lfs.pop());
        REQUIRE(popped);
        CHECK(popped->value == i);
    }
}


TEST_CASE("mpm/intrusive_lockfree_stack/pop_empty",
          "Popping an empty stack should return NULL")
{
    mpm::intrusive_lockfree_stack<entry> lfs;
    entry* popped(lfs.pop());
    CHECK(popped == 0);
}


TEST_CASE("mpm/intrusive_lockfree_stack/orthogonal",
          "Entry type using the ADL functions")
{
    mpm::intrusive_lockfree_stack<orthogonal_entry> lfs;
    orthogonal_entry oe(12);
    lfs.push(oe);
    orthogonal_entry* popped(lfs.pop());
    REQUIRE(popped);
    CHECK(12 == popped->value);
}


TEST_CASE("mpm/intrusive_lockfree_stack/clear_empty",
          "Test the behavior of clear()/empty()")
{
    mpm::intrusive_lockfree_stack<entry> lfs;
    CHECK(lfs.empty());
    entry node1, node2;

    lfs.push(node1);
    lfs.push(node2);

    CHECK_FALSE(lfs.empty());

    lfs.clear();
    CHECK(lfs.empty());
}


TEST_CASE("mpm/intrusive_lockfree_stack/skip_elimination",
          "Run with elimination turned off")
{
    mpm::intrusive_lockfree_stack<entry, mpm::disable_elimination> lfs;
    entry sn(1);
    lfs.push(sn);
    CHECK(&sn == lfs.pop());
    CHECK_FALSE(lfs.pop());
}


TEST_CASE("mpm/intrusive_lockfree_stack/go_like_hell",
          "Concurrent threads pushing & popping")
{
    static const int nthreads = 8;
    static const int nodes_per_thread = 5;

    pushpop_data::stack_type stack;
    unsigned int entry_count(nthreads * nodes_per_thread);
    pushpop_data::stack_type::value_type entries[entry_count];
    for(unsigned int i = 0; i < entry_count; i++)
        stack.push(entries[i]);

    pthread_barrier_t barrier;
    pthread_t threads[nthreads];
    pushpop_data pushpop_data_arr[nthreads];
    REQUIRE(0 == pthread_barrier_init(&barrier, NULL, nthreads));
    for(int i = 0; i < nthreads; i++)
    {
        pushpop_data_arr[i].barrier = &barrier;
        pushpop_data_arr[i].iterations = 1000000;
        pushpop_data_arr[i].stack = &stack;
        REQUIRE(0 == pthread_create(
            &threads[i], NULL, &pushpop, &pushpop_data_arr[i]));
    }

    for(int i = 0; i < nthreads; i++)
        pthread_join(threads[i], NULL);

    std::vector<typename pushpop_data::stack_type::pointer> popped_ptrs;
    drain(stack, std::back_inserter(popped_ptrs));
    CHECK(entry_count == popped_ptrs.size());
}


