#pragma once

#include "mpm/atomic.hpp"
#include "mpm/atomic_tagged_ptr.hpp"
#include "mpm/util.hpp"
#include <cassert>

namespace mpm {

template <typename T>
class lockfree_exchanger
{
public:
    typedef T         value_type;
    typedef T*        ptr_type;
    typedef ptr_type& ref_ptr_type;

    lockfree_exchanger();

    bool exchange(ptr_type my_ptr, ref_ptr_type out_val, unsigned int timeout);

private:
    MPM_DISALLOW_COPY_AND_ASSIGN(lockfree_exchanger);
    enum {
        EMPTY,      //Exchanger has no threads attemptying to exchange ptrs

        WAITING,    //One thread has posted a ptr to the exchanger and it
                    //awaits a partner thread with which to exchange

        BUSY,       //Two threads have agreed to swap pointers but the exchange
                    //is not yet complete.
    };

    atomic_tagged_ptr<value_type> m_slot;
};


template <typename T>
lockfree_exchanger<T>::lockfree_exchanger() : m_slot(0, EMPTY)
{
}


template <typename T>
bool
lockfree_exchanger<T>::exchange(
        ptr_type my_ptr, ref_ptr_type out_val, unsigned int timeout)
{
    typename atomic_tagged_ptr<value_type>::tag_type tag;

    for(unsigned int attempts = 0; attempts < timeout; attempts++)
    {
        ptr_type existing_ptr(m_slot.get(tag));
        switch(tag)
        {
            case EMPTY:
                if(m_slot.compare_and_swap(
                            existing_ptr, my_ptr, EMPTY, WAITING))
                {
                    // slot was not already set so the tag gets set to
                    // WAITING and we spin until an exchange partner
                    // arrives
                    do
                    {
                        existing_ptr = m_slot.get(tag);
                        if(BUSY == tag)
                        {
                            //a partner has arrived so reset internal
                            //state and exchange pointers
                            m_slot.set(0, EMPTY);
                            out_val = existing_ptr;
                            return true;
                        }
                    } while(++attempts <= timeout);
                    //done spinning but didn't meet with an exchange partner
                    //try set the internal state back to empty
                    if(m_slot.compare_and_swap(my_ptr, 0, WAITING, EMPTY))
                    {
                        return false;
                    }
                    else
                    {
                        //done spinning but found an exchange partner while
                        //trying to set the state back to EMPTY.
                        out_val = m_slot.get(tag);
                        m_slot.set(0, EMPTY);
                        return true;
                    }
                }
                break;
            case WAITING:
                //another thread is already in here and no third thread has
                //yet transitioned this exchanger into the BUSY state
                if(m_slot.compare_and_swap(
                            existing_ptr, my_ptr, WAITING, BUSY))
                {
                    //the current thread and some other thread have agreed
                    //to swap pointers
                    out_val = existing_ptr;
                    return true;
                }
                break;
            case BUSY:
                //Two other threads are currently using this Exchanger to
                //swap pointers and the current thread is the third wheel.
                //The state will soon turn back to EMPTY (after the
                //pointers have been exchanged) so just break to the next
                //loop iteration.
                break;
            default:
                assert(false);
        }
    }
    return false;
}

}
