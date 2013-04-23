//
// Copyright © 2010-2013 SimpliVity Corporation
// All Rights Reserved
//

#pragma once

#include "mpm/atomic.hpp"
#include "mpm/util.hpp"

namespace mpm {

/// \brief An intrusive lock-free MPSC queue
///
/// Values are NEVER copied into this datastructure, their lifetimes must
/// be managed externally. An instance of type T can be inserted into this
/// datastructure if it meets one of two criteria:
///  (1) the free functions mpm_intrusive_lockfree_mpsc_queue_get_next(const T volatile&):T* and
///      mpm_intrusive_lockfree_mpsc_queue_set_next(T volatile&, T*):void exist
///      in the same namespace as T, or
///  (2) T publicly extends mpm::intrusive_lockfree_mpsc_queue_entry<T>
template <typename T>
class intrusive_lockfree_mpsc_queue
{
public:

    typedef T value_type;
    typedef T* pointer;
    typedef T* volatile volatile_pointer;
    typedef T& reference;

    intrusive_lockfree_mpsc_queue();

    void push(reference value);
    pointer pop();

private:
    MPM_DISALLOW_COPY_AND_ASSIGN(intrusive_lockfree_mpsc_queue);

    value_type m_stub;
    volatile_pointer m_head;
    pointer m_tail;
};


template <typename T>
intrusive_lockfree_mpsc_queue<T>::intrusive_lockfree_mpsc_queue() :
    m_head(&m_stub), m_tail(&m_stub)
{
    mpm_intrusive_lockfree_mpsc_queue_set_next(m_stub, static_cast<pointer>(0));
}


template <typename T>
void
intrusive_lockfree_mpsc_queue<T>::push(reference value)
{
    mpm_intrusive_lockfree_mpsc_queue_set_next(value, static_cast<pointer>(0));
    pointer prev(MPM_EXCHG(&m_head, &value));
    mpm_intrusive_lockfree_mpsc_queue_set_next(*prev, &value);
}


template <typename T>
typename intrusive_lockfree_mpsc_queue<T>::pointer
intrusive_lockfree_mpsc_queue<T>::pop()
{
    pointer tail = m_tail;
    pointer next(
        mpm_intrusive_lockfree_mpsc_queue_get_next(*tail));

    if (tail == &m_stub)
    {
        if (0 == next)
            return 0;
        m_tail = next;
        tail = next;
        next = mpm_intrusive_lockfree_mpsc_queue_get_next(*next);
    }
    if (next)
    {
        m_tail = next;
        return tail;
    }
    T* head = m_head;
    if (tail != head)
        return 0;
    push(m_stub);
    next = mpm_intrusive_lockfree_mpsc_queue_get_next(*tail);
    if (next)
    {
        m_tail = next;
        return tail;
    }
    return 0;
}


template <typename T>
inline void mpm_intrusive_lockfree_mpsc_queue_set_next(
        T volatile& entry, T* next)
{
    entry.next = next;
}


template <typename T>
inline T* mpm_intrusive_lockfree_mpsc_queue_get_next(const T volatile& entry)
{
    return entry.next;
}


template <typename T>
class intrusive_lockfree_mpsc_queue_entry
{
    friend void mpm_intrusive_lockfree_mpsc_queue_set_next<>(T volatile&, T*);
    friend T* mpm_intrusive_lockfree_mpsc_queue_get_next<>(const T volatile&);
    T* next;
};

}
