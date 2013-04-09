#pragma once

#include "util.hpp"
#include <stdint.h>

namespace mpm {

    /// Packs a pointer and a short into 64 bits and allows for atomic read,
    /// write, and CAS on them.
    template <typename T, typename Tag=uint16_t>
    class atomic_tagged_ptr
    {
    public:

        typedef Tag tag_type;
        typedef T*  ptr_type;
        typedef T   value_type;

        atomic_tagged_ptr(ptr_type ptr=0, tag_type tag=0);

        ptr_type get(tag_type & tag_out) const;
        void set(ptr_type ptr, tag_type tag);
        bool compare_and_swap(ptr_type expected_ptr, ptr_type new_ptr,
                tag_type expected_tag, tag_type new_tag);

    private:
        MPM_DISALLOW_COPY_AND_ASSIGN(atomic_tagged_ptr);
        MPM_STATIC_ASSERT(sizeof(tag_type) <= 2);

        typedef uint64_t raw_value_type;

        raw_value_type pack(ptr_type ptr, tag_type tag) const;
        ptr_type unpack(raw_value_type raw, tag_type & tag_out) const;

        volatile raw_value_type m_raw_value;
    };


    template <typename T, typename Tag>
    atomic_tagged_ptr<T, Tag>::atomic_tagged_ptr(ptr_type ptr, tag_type tag) :
        m_raw_value(pack(ptr, tag))
    {
    }


    template <typename T, typename Tag>
    bool
    atomic_tagged_ptr<T, Tag>::compare_and_swap(ptr_type expected_ptr,
            ptr_type new_ptr, tag_type expected_tag, tag_type new_tag)
    {
        raw_value_type new_value(pack(new_ptr, new_tag));
        raw_value_type expected_value(pack(expected_ptr, expected_tag));
        return MPM_CAS(&m_raw_value, expected_value, new_value);
    }


    template <typename T, typename Tag>
    typename atomic_tagged_ptr<T, Tag>::ptr_type
    atomic_tagged_ptr<T, Tag>::get(tag_type & tag_out) const
    {
        return unpack(m_raw_value, tag_out);
    }


    template <typename T, typename Tag>
    void
    atomic_tagged_ptr<T, Tag>::set(ptr_type ptr, tag_type tag)
    {
        raw_value_type new_value(pack(ptr, tag));
        MPM_EXCHG(&m_raw_value, new_value);
    }


    template <typename T, typename Tag>
    typename atomic_tagged_ptr<T, Tag>::raw_value_type
    atomic_tagged_ptr<T, Tag>::pack(ptr_type ptr, tag_type tag) const
    {
        uint64_t ret = tag;
        ret = (ret << 48 | reinterpret_cast<uint64_t>(ptr));
        return ret;
    }


    template <typename T, typename Tag>
    typename atomic_tagged_ptr<T, Tag>::ptr_type
    atomic_tagged_ptr<T, Tag>::unpack(
            raw_value_type raw, tag_type & tag_out) const
    {
        tag_out = raw >> 48;
        return reinterpret_cast<ptr_type>(raw & 0x0000ffffffffffff);
    }

}
