/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

// Reference:
// - [BCGMMW18]:
//      Title: "ZEXE: Enabling Decentralized Private Computation"
//      ePrint: https://eprint.iacr.org/2018/962.pdf

#ifndef BLS12_377_INIT_HPP_
#define BLS12_377_INIT_HPP_
#include <libff/algebra/curves/public_params.hpp>
#include <libff/algebra/fields/fp.hpp>
#include <libff/algebra/fields/fp12_2over3over2.hpp>
#include <libff/algebra/fields/fp2.hpp>
#include <libff/algebra/fields/fp6_3over2.hpp>

namespace libff
{

const mp_size_t bls12_377_r_bitcount = 253;
const mp_size_t bls12_377_q_bitcount = 377;

const mp_size_t bls12_377_r_limbs =
    (bls12_377_r_bitcount + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS;
const mp_size_t bls12_377_q_limbs =
    (bls12_377_q_bitcount + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS;

// Declare the r modulus from bw6_761_modulus_r. We must instantiate the field
// model templates using references to the SAME bigint, otherwise Fr<bw6_761_pp>
// and Fq<bls12_377_pp> are different types (see the Fp_model type parameters).
extern bigint<bls12_377_q_limbs> bw6_761_modulus_r;

extern bigint<bls12_377_r_limbs> bls12_377_modulus_r;
// Ideally, we would use a reference:
//
//   bigint<bls12_377_q_limbs> &bls12_377_modulus_q = bw6_761_modulus_r;
//
// but bls12_377_modulus_q cannot then be used as a template parameter. We are
// forced to use a macro. Note that bw6_761_modulus_r is initialized by both
// bw6_761_init() and bls12_377_init() (whether references or macros are used).
#define bls12_377_modulus_q bw6_761_modulus_r

typedef Fp_model<bls12_377_r_limbs, bls12_377_modulus_r> bls12_377_Fr;
typedef Fp_model<bls12_377_q_limbs, bls12_377_modulus_q> bls12_377_Fq;
typedef Fp2_model<bls12_377_q_limbs, bls12_377_modulus_q> bls12_377_Fq2;
typedef Fp6_3over2_model<bls12_377_q_limbs, bls12_377_modulus_q> bls12_377_Fq6;
typedef Fp12_2over3over2_model<bls12_377_q_limbs, bls12_377_modulus_q>
    bls12_377_Fq12;
typedef bls12_377_Fq12 bls12_377_GT;

// Parameters for Barreto-Lynn-Scott curve E/Fq : y^2 = x^3 + b
extern bls12_377_Fq bls12_377_coeff_b;
extern bigint<bls12_377_r_limbs> bls12_377_trace_of_frobenius;
// Parameters for twisted Barreto-Lynn-Scott curve E'/Fq2 : y^2 = x^3 + b/xi
extern bls12_377_Fq2 bls12_377_twist;
extern bls12_377_Fq2 bls12_377_twist_coeff_b;
extern bls12_377_Fq bls12_377_twist_mul_by_b_c0;
extern bls12_377_Fq bls12_377_twist_mul_by_b_c1;
extern bls12_377_Fq2 bls12_377_twist_mul_by_q_X;
extern bls12_377_Fq2 bls12_377_twist_mul_by_q_Y;

// Coefficient \beta in endomorphism (x, y) -> (\beta * x, y)
extern bls12_377_Fq bls12_377_g1_endomorphism_beta;
extern bigint<bls12_377_r_limbs> bls12_377_g1_safe_subgroup_check_c1;
extern bigint<bls12_377_r_limbs> bls12_377_g1_proof_of_safe_subgroup_w;
extern bls12_377_Fq bls12_377_g1_proof_of_safe_subgroup_non_member_x;
extern bls12_377_Fq bls12_377_g1_proof_of_safe_subgroup_non_member_y;

// Coefficients for G2 untwist-frobenius-twist
extern bls12_377_Fq12 bls12_377_g2_untwist_frobenius_twist_v;
extern bls12_377_Fq12 bls12_377_g2_untwist_frobenius_twist_w_3;
extern bls12_377_Fq12 bls12_377_g2_untwist_frobenius_twist_v_inverse;
extern bls12_377_Fq12 bls12_377_g2_untwist_frobenius_twist_w_3_inverse;

// Coefficients used in bls12_377_G2::mul_by_cofactor
extern bigint<bls12_377_r_limbs> bls12_377_g2_mul_by_cofactor_h2_0;
extern bigint<bls12_377_r_limbs> bls12_377_g2_mul_by_cofactor_h2_1;

// Parameters for pairing
extern bigint<bls12_377_q_limbs> bls12_377_ate_loop_count;
extern bool bls12_377_ate_is_loop_count_neg;
// The embedding degree (k) = 12
extern bigint<12 * bls12_377_q_limbs> bls12_377_final_exponent;
extern bigint<bls12_377_q_limbs> bls12_377_final_exponent_z;
extern bool bls12_377_final_exponent_is_z_neg;

void init_bls12_377_params();

class bls12_377_G1;
class bls12_377_G2;

} // namespace libff
#endif // BLS12_377_INIT_HPP_
