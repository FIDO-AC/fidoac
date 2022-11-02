#include "ffi.h"

#include "ffi_serialization.hpp"

#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_pp.hpp>

namespace libff
{

namespace ffi
{

// Generic functions to be used by the entry points

template<typename ppT>
bool g1_add(
    const void *a_g1,
    size_t a_g1_size,
    const void *b_g1,
    size_t b_g1_size,
    void *out_g1,
    size_t out_g1_size)
{
    libff::G1<ppT> a;
    libff::G1<ppT> b;
    if (group_element_read(a, a_g1, a_g1_size) &&
        group_element_read(b, b_g1, b_g1_size)) {
        const libff::G1<ppT> output = a + b;
        return group_element_write(output, out_g1, out_g1_size);
    }

    return false;
}

template<typename ppT>
bool g1_mul(
    const void *p_g1,
    size_t p_g1_size,
    const void *s_fr,
    size_t s_fr_size,
    void *out_g1,
    size_t out_g1_size)
{
    libff::G1<ppT> p;
    libff::Fr<ppT> s;
    if (group_element_read(p, p_g1, p_g1_size) &&
        field_element_read(s, s_fr, s_fr_size)) {
        const libff::G1<ppT> output = s * p;
        return group_element_write(output, out_g1, out_g1_size);
    }

    return false;
}

template<typename ppT>
bool pairing(
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
    size_t h_g2_size)
{
    libff::G1<ppT> a;
    libff::G2<ppT> b;
    libff::G1<ppT> c;
    libff::G2<ppT> d;
    libff::G1<ppT> e;
    libff::G2<ppT> f;
    libff::G1<ppT> g;
    libff::G2<ppT> h;

    if (group_element_read(a, a_g1, a_g1_size) &&
        group_element_read(b, b_g2, b_g2_size) &&
        group_element_read(c, c_g1, c_g1_size) &&
        group_element_read(d, d_g2, d_g2_size) &&
        group_element_read(e, e_g1, e_g1_size) &&
        group_element_read(f, f_g2, f_g2_size) &&
        group_element_read(g, g_g1, g_g1_size) &&
        group_element_read(h, h_g2, h_g2_size)) {
        // Use the double miller loop, to compute the pairing for two
        // pairs at a time, and apply the final_exponentiation to the
        // product.

        // miller(a,b).miller(c,d)
        libff::GT<ppT> miller_abcd = ppT::double_miller_loop(
            ppT::precompute_G1(a),
            ppT::precompute_G2(b),
            ppT::precompute_G1(c),
            ppT::precompute_G2(d));
        // miller(e,f).miller(g,h)
        libff::GT<ppT> miller_efgh = ppT::double_miller_loop(
            ppT::precompute_G1(e),
            ppT::precompute_G2(f),
            ppT::precompute_G1(g),
            ppT::precompute_G2(h));

        // e(a,b).e(c,d).e(e,f).e(g,h)
        libff::GT<ppT> product =
            ppT::final_exponentiation(miller_abcd * miller_efgh);
        return libff::GT<ppT>::one() == product;
    }

    return false;
}

} // namespace ffi

} // namespace libff

// BLS12-377 entry points

extern "C" bool bls12_377_init()
{
    libff::bls12_377_pp::init_public_params();
    return true;
}

extern "C" bool bls12_377_g1_add(
    const void *a_g1,
    size_t a_g1_size,
    const void *b_g1,
    size_t b_g1_size,
    void *out_g1,
    size_t out_g1_size)
{
    return libff::ffi::g1_add<libff::bls12_377_pp>(
        a_g1, a_g1_size, b_g1, b_g1_size, out_g1, out_g1_size);
}

extern "C" bool bls12_377_g1_mul(
    const void *p_g1,
    size_t p_g1_size,
    const void *s_fr,
    size_t s_fr_size,
    void *out_g1,
    size_t out_g1_size)

{
    return libff::ffi::g1_mul<libff::bls12_377_pp>(
        p_g1, p_g1_size, s_fr, s_fr_size, out_g1, out_g1_size);
}

extern "C" bool bls12_377_pairing(
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
    size_t h_g2_size)
{
    return libff::ffi::pairing<libff::bls12_377_pp>(
        a_g1,
        a_g1_size,
        b_g2,
        b_g2_size,
        c_g1,
        c_g1_size,
        d_g2,
        d_g2_size,
        e_g1,
        e_g1_size,
        f_g2,
        f_g2_size,
        g_g1,
        g_g1_size,
        h_g2,
        h_g2_size);
}

// BW6-761 entry points

extern "C" bool bw6_761_init()
{
    libff::bw6_761_pp::init_public_params();
    return true;
}

extern "C" bool bw6_761_g1_add(
    const void *a_g1,
    size_t a_g1_size,
    const void *b_g1,
    size_t b_g1_size,
    void *out_g1,
    size_t out_g1_size)
{
    return libff::ffi::g1_add<libff::bw6_761_pp>(
        a_g1, a_g1_size, b_g1, b_g1_size, out_g1, out_g1_size);
}

extern "C" bool bw6_761_g1_mul(
    const void *p_g1,
    size_t p_g1_size,
    const void *s_fr,
    size_t s_fr_size,
    void *out_g1,
    size_t out_g1_size)

{
    return libff::ffi::g1_mul<libff::bw6_761_pp>(
        p_g1, p_g1_size, s_fr, s_fr_size, out_g1, out_g1_size);
}

extern "C" bool bw6_761_pairing(
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
    size_t h_g2_size)
{
    return libff::ffi::pairing<libff::bw6_761_pp>(
        a_g1,
        a_g1_size,
        b_g2,
        b_g2_size,
        c_g1,
        c_g1_size,
        d_g2,
        d_g2_size,
        e_g1,
        e_g1_size,
        f_g2,
        f_g2_size,
        g_g1,
        g_g1_size,
        h_g2,
        h_g2_size);
}
