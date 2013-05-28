#pragma once

namespace mpm {
#define CAS(val, expected, new_val) \
    __sync_bool_compare_and_swap(val, expected, new_val)

#define EXCHG(storage, value) \
    __sync_lock_test_and_set(storage, value)
}
