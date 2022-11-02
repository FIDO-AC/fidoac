#ifndef BW6_761_INIT_HPP_
#define BW6_761_INIT_HPP_

#include <libff/algebra/curves/public_params.hpp>
#include <libff/algebra/fields/fp.hpp>
#include <libff/algebra/fields/fp3.hpp>
#include <libff/algebra/fields/fp6_2over3.hpp>

namespace libff
{

const mp_size_t bw6_761_r_bitcount = 377;
const mp_size_t bw6_761_q_bitcount = 761;

const mp_size_t bw6_761_r_limbs =
    (bw6_761_r_bitcount + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS;
const mp_size_t bw6_761_q_limbs =
    (bw6_761_q_bitcount + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS;

extern bigint<bw6_761_r_limbs> bw6_761_modulus_r;
extern bigint<bw6_761_q_limbs> bw6_761_modulus_q;

typedef Fp_model<bw6_761_r_limbs, bw6_761_modulus_r> bw6_761_Fr;
typedef Fp_model<bw6_761_q_limbs, bw6_761_modulus_q> bw6_761_Fq;
typedef Fp3_model<bw6_761_q_limbs, bw6_761_modulus_q> bw6_761_Fq3;
typedef Fp6_2over3_model<bw6_761_q_limbs, bw6_761_modulus_q> bw6_761_Fq6;
typedef bw6_761_Fq6 bw6_761_GT;

// Parameters for the curve E/Fq : y^2 = x^3 + b
extern bw6_761_Fq bw6_761_coeff_b;
// Parameters for the twist E'/Fq: y^2 = x^3 + b * xi
extern bw6_761_Fq bw6_761_twist;
extern bw6_761_Fq bw6_761_twist_coeff_b;

// parameters for pairing
extern bigint<bw6_761_q_limbs> bw6_761_ate_loop_count1;
extern bigint<bw6_761_q_limbs> bw6_761_ate_loop_count2;
extern bool bw6_761_ate_is_loop_count_neg;
extern bigint<bw6_761_q_limbs> bw6_761_final_exponent_z;
extern bool bw6_761_final_exponent_is_z_neg;

void init_bw6_761_params();

class bw6_761_G1;
class bw6_761_G2;

} // namespace libff

#endif // BW6_761_INIT_HPP_
