/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_FINAL_EXPONENTIATION_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_FINAL_EXPONENTIATION_HPP_

#include "libsnark/gadgetlib1/gadgets/curves/weierstrass_g1_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/curves/weierstrass_g2_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp12_2over3over2_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp2_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bw6_761_pairing_params.hpp"

#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>

namespace libsnark
{

template<typename ppT>
class bls12_377_final_exp_first_part_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using FqkT = libff::Fqk<other_curve<ppT>>;

    // Follows the implementation used in
    // libff::bls12_377_final_exponentiation_first_chunk() (see
    // clearmatics/libff/libff/algebra/curves/bls12_377/bls12_377_pairing.cpp),
    // which in turn follows:
    //   https://eprint.iacr.org/2016/130.pdf

    Fp12_2over3over2_variable<FqkT> _result;

    // A = elt^(q^6)
    // B = elt^(-1)
    Fp12_2over3over2_inv_gadget<FqkT> _compute_B;
    // C = A * B = elt^(q^6 - 1)
    Fp12_2over3over2_mul_gadget<FqkT> _compute_C;
    // D = C^(q^2) = elt^((q^6 - 1) * (q^2))
    // result = D * C = elt^((q^6 - 1) * (q^2 + 1))
    Fp12_2over3over2_mul_gadget<FqkT> _compute_D_times_C;

    bls12_377_final_exp_first_part_gadget(
        protoboard<FieldT> &pb,
        const Fp12_2over3over2_variable<FqkT> &in,
        const Fp12_2over3over2_variable<FqkT> &result,
        const std::string &annotation_prefix);

    const Fp12_2over3over2_variable<FqkT> &result() const;
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

template<typename ppT>
class bls12_377_exp_by_z_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using FqkT = libff::Fqk<other_curve<ppT>>;
    using cyclotomic_square = Fp12_2over3over2_cyclotomic_square_gadget<FqkT>;
    using multiply = Fp12_2over3over2_mul_gadget<FqkT>;
    using unitary_inverse = Fp12_2over3over2_cyclotomic_square_gadget<FqkT>;

    Fp12_2over3over2_variable<FqkT> _result;
    std::vector<std::shared_ptr<cyclotomic_square>> _squares;
    std::vector<std::shared_ptr<multiply>> _multiplies;
    std::shared_ptr<unitary_inverse> _inverse;

    bls12_377_exp_by_z_gadget(
        protoboard<FieldT> &pb,
        const Fp12_2over3over2_variable<FqkT> &in,
        const Fp12_2over3over2_variable<FqkT> &result,
        const std::string &annotation_prefix);

    const Fp12_2over3over2_variable<FqkT> &result() const;
    void generate_r1cs_constraints();
    void generate_r1cs_witness();

private:
    void initialize_z_neg(
        protoboard<FieldT> &pb,
        const Fp12_2over3over2_variable<FqkT> &in,
        const std::string &annotation_prefix);
    void initialize_z_pos(
        protoboard<FieldT> &pb,
        const Fp12_2over3over2_variable<FqkT> &in,
        const std::string &annotation_prefix);
};

template<typename ppT>
class bls12_377_final_exp_last_part_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using FqkT = libff::Fqk<other_curve<ppT>>;

    // Based on the implementation of
    // libff::bls12_377_final_exponentiation_last_chunk() (see
    // clearmatics/libff/libff/algebra/curves/bls12_377/bls12_377_pairing.cpp),
    // which follows Algorithm 1 described in Table 1 of
    // https://eprint.iacr.org/2016/130.pdf

    Fp12_2over3over2_variable<FqkT> _result;

    Fp12_2over3over2_cyclotomic_square_gadget<FqkT> _compute_in_squared;
    bls12_377_exp_by_z_gadget<ppT> _compute_B;
    Fp12_2over3over2_square_gadget<FqkT> _compute_C;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_D;
    bls12_377_exp_by_z_gadget<ppT> _compute_E;
    bls12_377_exp_by_z_gadget<ppT> _compute_F;
    bls12_377_exp_by_z_gadget<ppT> _compute_G;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_H;
    bls12_377_exp_by_z_gadget<ppT> _compute_I;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_K;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_L;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_N;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_P;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_R;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_T;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_U;
    Fp12_2over3over2_mul_gadget<FqkT> _compute_U_times_L;

    bls12_377_final_exp_last_part_gadget(
        protoboard<FieldT> &pb,
        const Fp12_2over3over2_variable<FqkT> &in,
        const Fp12_2over3over2_variable<FqkT> &result,
        const std::string &annotation_prefix);

    const Fp12_2over3over2_variable<FqkT> &result() const;
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

// Wrapper around final_exp gadgets with interface expected by the groth16
// gadgets. `result_is_one` is constrained to a boolean (0 or 1), and set in
// 'generate_r1cs_witness' based on the output value of the final
// exponentiation (if final exp == 1, `result_is_one` is set to 1, otherwise
// `result_is_one` is set to 0).
//
// Note that the constraints on the final exp output are ONLY enforced when
// `result_is_one` == 1. In otherwords, it is infeasible to generate valid
// inputs such that the final exp output is not equal to 1 and result_is_one ==
// 1. However, it IS possible to generate inputs such that final_exp == 1 but
// `result_is_one` == 0.
template<typename ppT>
class bls12_377_final_exp_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using FqkT = libff::Fqk<other_curve<ppT>>;

    bls12_377_final_exp_first_part_gadget<ppT> _compute_first_part;
    bls12_377_final_exp_last_part_gadget<ppT> _compute_last_part;
    pb_variable<FieldT> _result_is_one;

    bls12_377_final_exp_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const Fp12_2over3over2_variable<FqkT> &el,
        const pb_variable<FieldT> &result_is_one,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

} // namespace libsnark

#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_final_exponentiation.tcc"

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_FINAL_EXPONENTIATION_HPP_
