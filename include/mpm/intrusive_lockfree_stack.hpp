#pragma once

#include "mpm/atomic.hpp"
#include "mpm/atomic_tagged_ptr.hpp"
#include "mpm/lockfree_exchanger.hpp"
#include "mpm/util.hpp"
#include <cassert>
#include <cstddef>


namespace mpm {


template <std::size_t Slots, unsigned int Timeout, unsigned int Attempts>
struct elimination_opts
{
    /// the number of elimination slots to use
    static const std::size_t slots = Slots;

    /// how long to stay in each elimination attempt; expressed as a number of
    /// times to spin in the elimination loop
    static const unsigned int timeout = Timeout;

    /// The number of times to select and participate in an elimination slot
    /// before going back to the central datastructure
    static const unsigned int attempts = Attempts;
};


typedef elimination_opts<0, 0, 0> disable_elimination;


/// \brief an intrusive lock-free stack
///
/// The key feature of this implementation is that it uses an elimination array
/// as a means of back-off when contention is detected. The elimination
/// approach is taken from
/// http://citeseer.ist.psu.edu/viewdoc/summary?doi=10.1.1.156.8728
///
/// Values are NEVER copied into this datastructure, their lifetimes must
/// be managed externally. An instance of type T can be inserted into this
/// datastructure if it meets one of two criteria:
///  (1) the free functions mpm_lfs_get_next(T&):T* and
///      mpm_lfs_set_next(T&, T*):void exist in the same namespace as T, or
///  (2) T extends mpm::intrusive_lockfree_stack_entry<T> (not necessary to extend
///      publicly)
template <typename T, typename EliminationOpts=elimination_opts<16, 500, 2> >
class intrusive_lockfree_stack
{
public:

    typedef T               value_type;
    typedef value_type&     reference;
    typedef value_type*     pointer;
    typedef EliminationOpts elimination_opts;

    intrusive_lockfree_stack();

    /// \brief Pushes a value onto the top of the stack
    /// The stack is unbounded so this function always succeeds. The
    /// implementation is guaranteed to be lock-free but not necessarily
    /// wait-free.
    ///
    /// param[in] value the value to put on the top of this stack
    void push(reference value);


    /// \brief Pops a value from the top of the stack.
    /// Does not block.
    ///
    /// \returns NULL if *this is empty, otherwise the value removed from the
    ///          top of the stack.
    pointer pop();

    /// \brief Clears this stack
    /// Does NOT free the memory associated with the entries in this stack
    void clear();


    /// \brief Checks to see if this stack is empty
    /// \returns true if the stack is empty, false otherwise
    bool empty() const;

private:
    MPM_DISALLOW_COPY_AND_ASSIGN(intrusive_lockfree_stack);
    MPM_STATIC_ASSERT(0 == (elimination_opts::slots &
                    (elimination_opts::slots - std::size_t(1u))));

    enum pop_result { CAS_FAILED, EMPTY, SUCCESS };

    bool try_push(reference value);
    pop_result try_pop(pointer& out);
    bool eliminate_push(reference value);
    bool eliminate_pop(pointer& out);
    bool exchange(pointer ptr, pointer& out);

    //todo - pad out the array elements
    typedef lockfree_exchanger<T> /*__attribute__((aligned(64)))*/ padded_exchanger;
    typedef atomic_tagged_ptr<T> top_ptr;

    top_ptr m_top;
    padded_exchanger m_exchangers[elimination_opts::slots];
};


template <typename T, typename E>
intrusive_lockfree_stack<T, E>::intrusive_lockfree_stack()
{
}


template <typename T, typename E>
void
intrusive_lockfree_stack<T, E>::push(reference value)
{
    while(true)
    {
        if(try_push(value) || eliminate_push(value))
            return;
    }
}


template <typename T, typename E>
typename intrusive_lockfree_stack<T, E>::pointer
intrusive_lockfree_stack<T, E>::pop()
{
    pointer out(NULL);
    while(true)
    {
        switch(try_pop(out))
        {
            case SUCCESS : return out;
            case EMPTY   : return NULL;
            case CAS_FAILED :
                if(eliminate_pop(out)) return out;
                break;
            default:
                assert(false);
        }
    }
}


template <typename T, typename E>
void
intrusive_lockfree_stack<T, E>::clear()
{
    m_top.set(NULL, 0);
}


template <typename T, typename E>
bool
intrusive_lockfree_stack<T, E>::empty() const
{
    typename top_ptr::tag_type _;
    return NULL == m_top.get(_);
}


template <typename T, typename E>
bool
intrusive_lockfree_stack<T, E>::try_push(reference value)
{
    typename top_ptr::tag_type old_tag;
    pointer old_top(m_top.get(old_tag));
    mpm_lfs_set_next(value, old_top);
    return m_top.compare_and_swap(old_top, &value, old_tag, old_tag + 1);
}


template <typename T, typename E>
typename intrusive_lockfree_stack<T, E>::pop_result
intrusive_lockfree_stack<T, E>::try_pop(pointer& out)
{
    typename top_ptr::tag_type old_tag;
    out = m_top.get(old_tag);
    if(NULL == out)
        return EMPTY;
    pointer new_top(mpm_lfs_get_next(*out));
    return m_top.compare_and_swap(
            out, new_top, old_tag, old_tag + 1) ? SUCCESS : CAS_FAILED;
}


template <typename T, typename E>
bool
intrusive_lockfree_stack<T, E>::eliminate_push(reference value)
{
    pointer out(NULL);
    return exchange(&value, out) && out == NULL;
}


template <typename T, typename E>
bool
intrusive_lockfree_stack<T, E>::eliminate_pop(pointer& ptr)
{
    return exchange(NULL, ptr) && ptr;
}


namespace detail {

    inline unsigned int cycle_count_low_bits()
    {
        unsigned int lo, hi;
        __asm__ __volatile__("rdtsc":"=a"(lo), "=d"(hi));
        return lo;
    }

    //if either the number of elimination slots or the number of elimination
    //attempts is zero, compile the elimination out alltogether

    template <typename T, typename Elim, typename Arr>
    inline
    typename mpm::enable_if<
        Elim::slots == 0 || Elim::attempts == 0, bool>::type
    exchange(T*, T*&, Arr&)
    {
        return false;
    }


    template <typename T, typename Elim, typename Arr>
    typename mpm::disable_if<
        Elim::slots == 0 || Elim::attempts == 0, bool>::type
    exchange(T* p, T*& out, Arr& exchangers)
    {
        for(unsigned int attempts = 0; attempts < Elim::attempts; attempts++)
        {
            std::size_t index(detail::cycle_count_low_bits() % Elim::slots);
            if(exchangers[index].exchange(p, out, Elim::timeout))
                return true;
        }
        return false;
    }
}


template <typename T, typename E>
bool
intrusive_lockfree_stack<T, E>::exchange(pointer p, pointer& out)
{
    return detail::exchange<T, E>(p, out, m_exchangers);
}


template <typename T>
inline void mpm_lfs_set_next(T& entry, T* next)
{
    entry.mpm_intrusive_lockfree_stack_next = next;
}


template <typename T>
inline T* mpm_lfs_get_next(const T& entry)
{
    return entry.mpm_intrusive_lockfree_stack_next;
}


template <typename T>
class intrusive_lockfree_stack_entry
{
    friend void mpm_lfs_set_next<>(T&, T*);
    friend T* mpm_lfs_get_next<>(const T&);
    T* mpm_intrusive_lockfree_stack_next;
};

}

