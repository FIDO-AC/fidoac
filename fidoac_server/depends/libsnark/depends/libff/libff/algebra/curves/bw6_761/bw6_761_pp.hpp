#ifndef BW6_761_PP_HPP_
#define BW6_761_PP_HPP_

#include <libff/algebra/curves/bw6_761/bw6_761_g1.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_g2.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_init.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_pairing.hpp>
#include <libff/algebra/curves/public_params.hpp>

namespace libff
{

class bw6_761_pp
{
public:
    typedef bw6_761_Fr Fp_type;
    typedef bw6_761_G1 G1_type;
    typedef bw6_761_G2 G2_type;
    typedef bw6_761_G1_precomp G1_precomp_type;
    typedef bw6_761_G2_precomp G2_precomp_type;
    typedef bw6_761_Fq Fq_type;
    typedef bw6_761_Fq3 Fqe_type;
    typedef bw6_761_Fq6 Fqk_type;
    typedef bw6_761_GT GT_type;

    // static const bool has_affine_pairing = true;

    static void init_public_params();
    static bw6_761_GT final_exponentiation(const bw6_761_Fq6 &elt);
    static bw6_761_G1_precomp precompute_G1(const bw6_761_G1 &P);
    static bw6_761_G2_precomp precompute_G2(const bw6_761_G2 &Q);
    static bw6_761_Fq6 miller_loop(
        const bw6_761_G1_precomp &prec_P, const bw6_761_G2_precomp &prec_Q);
    static bw6_761_Fq6 double_miller_loop(
        const bw6_761_G1_precomp &prec_P1,
        const bw6_761_G2_precomp &prec_Q1,
        const bw6_761_G1_precomp &prec_P2,
        const bw6_761_G2_precomp &prec_Q2);
    static bw6_761_Fq6 pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q);
    static bw6_761_Fq6 reduced_pairing(
        const bw6_761_G1 &P, const bw6_761_G2 &Q);
};

} // namespace libff

#endif // BW6_761_PP_HPP_
