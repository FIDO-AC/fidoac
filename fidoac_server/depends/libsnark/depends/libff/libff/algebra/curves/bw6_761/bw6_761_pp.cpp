#include <libff/algebra/curves/bw6_761/bw6_761_pp.hpp>

namespace libff
{

void bw6_761_pp::init_public_params() { init_bw6_761_params(); }

bw6_761_GT bw6_761_pp::final_exponentiation(const bw6_761_Fq6 &elt)
{
    return bw6_761_final_exponentiation(elt);
}

bw6_761_G1_precomp bw6_761_pp::precompute_G1(const bw6_761_G1 &P)
{
    return bw6_761_precompute_G1(P);
}

bw6_761_G2_precomp bw6_761_pp::precompute_G2(const bw6_761_G2 &Q)
{
    return bw6_761_precompute_G2(Q);
}

bw6_761_Fq6 bw6_761_pp::miller_loop(
    const bw6_761_G1_precomp &prec_P, const bw6_761_G2_precomp &prec_Q)
{
    return bw6_761_miller_loop(prec_P, prec_Q);
}

bw6_761_Fq6 bw6_761_pp::double_miller_loop(
    const bw6_761_G1_precomp &prec_P1,
    const bw6_761_G2_precomp &prec_Q1,
    const bw6_761_G1_precomp &prec_P2,
    const bw6_761_G2_precomp &prec_Q2)
{
    return bw6_761_double_miller_loop(prec_P1, prec_Q1, prec_P2, prec_Q2);
}

bw6_761_Fq6 bw6_761_pp::pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q)
{
    return bw6_761_pairing(P, Q);
}

bw6_761_Fq6 bw6_761_pp::reduced_pairing(
    const bw6_761_G1 &P, const bw6_761_G2 &Q)
{
    return bw6_761_reduced_pairing(P, Q);
}

} // namespace libff
