#ifndef BW6_761_PAIRING_HPP_
#define BW6_761_PAIRING_HPP_

#include <libff/algebra/curves/bw6_761/bw6_761_init.hpp>
#include <vector>

namespace libff
{

/* final exponentiation */

bw6_761_GT bw6_761_final_exponentiation(const bw6_761_Fq6 &elt);

/* ate pairing */

struct bw6_761_ate_G1_precomp {
    bw6_761_Fq PX;
    bw6_761_Fq PY;

    bool operator==(const bw6_761_ate_G1_precomp &other) const;
    friend std::ostream &operator<<(
        std::ostream &out, const bw6_761_ate_G1_precomp &prec_P);
    friend std::istream &operator>>(
        std::istream &in, bw6_761_ate_G1_precomp &prec_P);
};

struct bw6_761_ate_ell_coeffs {
    bw6_761_Fq ell_0;
    bw6_761_Fq ell_VW;
    bw6_761_Fq ell_VV;

    bool operator==(const bw6_761_ate_ell_coeffs &other) const;
    friend std::ostream &operator<<(
        std::ostream &out, const bw6_761_ate_ell_coeffs &dc);
    friend std::istream &operator>>(
        std::istream &in, bw6_761_ate_ell_coeffs &dc);
};

struct bw6_761_ate_G2_precomp_iteration {
    bw6_761_Fq QX;
    bw6_761_Fq QY;
    std::vector<bw6_761_ate_ell_coeffs> coeffs;

    bool operator==(const bw6_761_ate_G2_precomp_iteration &other) const;
    friend std::ostream &operator<<(
        std::ostream &out, const bw6_761_ate_G2_precomp_iteration &prec_Q);
    friend std::istream &operator>>(
        std::istream &in, bw6_761_ate_G2_precomp_iteration &prec_Q);
};

struct bw6_761_ate_G2_precomp {
    bw6_761_ate_G2_precomp_iteration precomp_1;
    bw6_761_ate_G2_precomp_iteration precomp_2;

    bool operator==(const bw6_761_ate_G2_precomp &other) const;
    friend std::ostream &operator<<(
        std::ostream &out, const bw6_761_ate_G2_precomp &prec_Q);
    friend std::istream &operator>>(
        std::istream &in, bw6_761_ate_G2_precomp &prec_Q);
};

bw6_761_ate_G1_precomp bw6_761_ate_precompute_G1(const bw6_761_G1 &P);
bw6_761_ate_G2_precomp bw6_761_ate_precompute_G2(const bw6_761_G2 &Q);

bw6_761_Fq6 bw6_761_ate_miller_loop(
    const bw6_761_ate_G1_precomp &prec_P, const bw6_761_ate_G2_precomp &prec_Q);

bw6_761_Fq6 bw6_761_ate_double_miller_loop(
    const bw6_761_ate_G1_precomp &prec_P1,
    const bw6_761_ate_G2_precomp &prec_Q1,
    const bw6_761_ate_G1_precomp &prec_P2,
    const bw6_761_ate_G2_precomp &prec_Q2);

bw6_761_Fq6 bw6_761_ate_pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q);
bw6_761_GT bw6_761_ate_reduced_pairing(
    const bw6_761_G1 &P, const bw6_761_G2 &Q);

/* choice of pairing */

typedef bw6_761_ate_G1_precomp bw6_761_G1_precomp;
typedef bw6_761_ate_G2_precomp bw6_761_G2_precomp;

bw6_761_G1_precomp bw6_761_precompute_G1(const bw6_761_G1 &P);

bw6_761_G2_precomp bw6_761_precompute_G2(const bw6_761_G2 &Q);

bw6_761_Fq6 bw6_761_miller_loop(
    const bw6_761_G1_precomp &prec_P, const bw6_761_G2_precomp &prec_Q);

bw6_761_Fq6 bw6_761_double_miller_loop(
    const bw6_761_ate_G1_precomp &prec_P1,
    const bw6_761_ate_G2_precomp &prec_Q1,
    const bw6_761_ate_G1_precomp &prec_P2,
    const bw6_761_ate_G2_precomp &prec_Q2);

bw6_761_Fq6 bw6_761_pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q);

bw6_761_GT bw6_761_reduced_pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q);

} // namespace libff

#endif // BW6_761_PAIRING_HPP_
