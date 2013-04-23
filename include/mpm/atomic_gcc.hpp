#pragma once

#define MPM_CAS(val, expected, new_val) \
    __sync_bool_compare_and_swap(val, expected, new_val)

#define MPM_EXCHG(storage, value) \
    __sync_lock_test_and_set(storage, value)

