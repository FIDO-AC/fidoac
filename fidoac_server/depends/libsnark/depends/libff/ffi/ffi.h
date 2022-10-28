#ifndef __LIBFF_FFI_FFI_H__
#define __LIBFF_FFI_FFI_H__

#include <stddef.h>

// Externally visible entry points for FFI implementations.

#if __cplusplus
extern "C"
{
#endif

// BLS12-377 entry points.
//
// Fr elements must be 32 bytes.
// G1 elements must be 96 bytes
// G2 elements must be 192 bytes

bool bls12_377_init();

bool bls12_377_g1_add(
    const void *a_g1,
    size_t a_g1_size,
    const void *b_g1,
    size_t b_g1_size,
    void *out_g1,
    size_t out_g1_size);

bool bls12_377_g1_mul(
    const void *p_g1,
    size_t p_g1_size,
    const void *s_fr,
    size_t s_fr_size,
    void *out_g1,
    size_t out_g1_size);

bool bls12_377_pairing(
    const void *a_g1,
    size_t a_g1_size,
    const void *b_g2,
    size_t b_g2_size,
    const void *c_g1,
    size_t c_g1_size,
    const void *d_g2,
    size_t d_g2_size,
    const void *e_g1,
    size_t e_g1_size,
    const void *f_g2,
    size_t f_g2_size,
    const void *g_g1,
    size_t g_g1_size,
    const void *h_g2,
    size_t h_g2_size);

// BW6-761 entry points
//
// Fr elements must be 48 bytes
// G1 elements must be 192 bytes
// G2 elements must be 192 bytes

bool bw6_761_init();

bool bw6_761_g1_add(
    const void *a_g1,
    size_t a_g1_size,
    const void *b_g1,
    size_t b_g1_size,
    void *out_g1,
    size_t out_g1_size);

bool bw6_761_g1_mul(
    const void *p_g1,
    size_t p_g1_size,
    const void *s_fr,
    size_t s_fr_size,
    void *out_g1,
    size_t out_g1_size);

bool bw6_761_pairing(
    const void *a_g1,
    size_t a_g1_size,
    const void *b_g2,
    size_t b_g2_size,
    const void *c_g1,
    size_t c_g1_size,
    const void *d_g2,
    size_t d_g2_size,
    const void *e_g1,
    size_t e_g1_size,
    const void *f_g2,
    size_t f_g2_size,
    const void *g_g1,
    size_t g_g1_size,
    const void *h_g2,
    size_t h_g2_size);

#if __cplusplus
}
#endif

#endif // __LIBFF_FFI_FFI_H__
