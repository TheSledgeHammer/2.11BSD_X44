

SRCS=   atomic_add_16_cas.c atomic_add_32_cas.c atomic_add_32_nv_cas.c atomic_add_64_cas.c      \
        atomic_add_64_nv_cas.c atomic_add_8_cas.c atomic_and_16_cas.c atomic_and_16_nv_cas.c    \
        atomic_and_32_cas.c atomic_and_32_nv_cas.c atomic_and_64_cas.c atomic_and_64_nv_cas.c   \
        atomic_and_8_cas.c atomic_and_8_nv_cas.c atomic_c11_compare_exchange_cas_16.c           \
        atomic_c11_compare_exchange_cas_32.c atomic_c11_compare_exchange_cas_8.c                \
        atomic_cas_16_cas.c atomic_cas_32_cas.c atomic_cas_64_cas.c atomic_cas_8_cas.c          \
        atomic_cas_by_cas32.c                                                                   \
        atomic_dec_32_add.c atomic_dec_32_cas.c atomic_dec_32_nv_add.c atomic_dec_32_nv_cas.c   \
        atomic_dec_64_add.c atomic_dec_64_cas.c atomic_dec_64_nv_add.c atomic_dec_64_nv_cas.c   \
        atomic_inc_32_add.c atomic_inc_32_cas.c atomic_inc_32_nv_add.c atomic_inc_32_nv_cas.c   \
        atomic_inc_64_add.c atomic_inc_64_cas.c atomic_inc_64_nv_add.c atomic_inc_64_nv_cas.c   \
        atomic_init_cas.c atomic_init_testset.c atomic_is_lock_free.c atomic_load.c             \
        atomic_nand_16_cas.c atomic_nand_32_cas.c atomic_nand_64_cas.c atomic_nand_8_cas.c      \
        atomic_op_namespace.h                                                                   \
        atomic_or_16_cas.c atomic_or_32_cas.c atomic_or_32_nv_cas.c atomic_or_64_cas.c          \
        atomic_or_64_nv_cas.c atomic_or_8_cas.c atomic_store.c                                  \
        atomic_sub_16_cas.c atomic_sub_32_cas.c atomic_sub_64_cas.c atomic_sub_8_cas.c          \
        atomic_swap_16_cas.c atomic_swap_32_cas.c atomic_swap_64_cas.c atomic_swap_8_cas.c      \
        atomic_xor_16_cas.c atomic_xor_32_cas.c atomic_xor_64_cas.c atomic_xor_8_cas.c          \
        membar_ops_nop.c
