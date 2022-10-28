/** @file
 *****************************************************************************

 Declaration of interfaces for final exponentiation gadgets.

 The gadgets verify final exponentiation for Weiersrass curves with embedding
 degrees 4 and 6.

 *****************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_MNT_MNT_FINAL_EXPONENTIATION_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_MNT_MNT_FINAL_EXPONENTIATION_HPP_

#include "libsnark/gadgetlib1/gadgets/fields/exponentiation_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/pairing_params.hpp"

#include <libff/algebra/curves/mnt/mnt4/mnt4_pp.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_pp.hpp>
#include <libff/algebra/curves/public_params.hpp>
#include <memory>

namespace libsnark
{

/**
 * Gadget for final exponentiation with embedding degree 4.
 */
template<typename ppT>
class mnt4_final_exp_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;

    Fqk_variable<ppT> el;
    std::shared_ptr<Fqk_variable<ppT>> one;
    std::shared_ptr<Fqk_variable<ppT>> el_inv;
    std::shared_ptr<Fqk_variable<ppT>> el_q_3;
    std::shared_ptr<Fqk_variable<ppT>> el_q_3_minus_1;
    std::shared_ptr<Fqk_variable<ppT>> alpha;
    std::shared_ptr<Fqk_variable<ppT>> beta;
    std::shared_ptr<Fqk_variable<ppT>> beta_q;
    std::shared_ptr<Fqk_variable<ppT>> el_inv_q_3;
    std::shared_ptr<Fqk_variable<ppT>> el_inv_q_3_minus_1;
    std::shared_ptr<Fqk_variable<ppT>> inv_alpha;
    std::shared_ptr<Fqk_variable<ppT>> inv_beta;
    std::shared_ptr<Fqk_variable<ppT>> w1;
    std::shared_ptr<Fqk_variable<ppT>> w0;
    std::shared_ptr<Fqk_variable<ppT>> result;

    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_el_inv;
    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_el_q_3_minus_1;
    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_beta;
    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_el_inv_q_3_minus_1;
    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_inv_beta;

    std::shared_ptr<exponentiation_gadget<
        FqkT<ppT>,
        Fp6_2over3_variable,
        Fp6_2over3_mul_gadget,
        Fp6_2over3_cyclotomic_sqr_gadget,
        libff::mnt6_q_limbs>>
        compute_w1;
    std::shared_ptr<exponentiation_gadget<
        FqkT<ppT>,
        Fp6_2over3_variable,
        Fp6_2over3_mul_gadget,
        Fp6_2over3_cyclotomic_sqr_gadget,
        libff::mnt6_q_limbs>>
        compute_w0;
    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_result;

    pb_variable<FieldT> result_is_one;

    mnt4_final_exp_gadget(
        protoboard<FieldT> &pb,
        const Fqk_variable<ppT> &el,
        const pb_variable<FieldT> &result_is_one,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/**
 * Gadget for final exponentiation with embedding degree 6.
 */
template<typename ppT>
class mnt6_final_exp_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;

    Fqk_variable<ppT> el;
    std::shared_ptr<Fqk_variable<ppT>> one;
    std::shared_ptr<Fqk_variable<ppT>> el_inv;
    std::shared_ptr<Fqk_variable<ppT>> el_q_2;
    std::shared_ptr<Fqk_variable<ppT>> el_q_2_minus_1;
    std::shared_ptr<Fqk_variable<ppT>> el_q_3_minus_q;
    std::shared_ptr<Fqk_variable<ppT>> el_inv_q_2;
    std::shared_ptr<Fqk_variable<ppT>> el_inv_q_2_minus_1;
    std::shared_ptr<Fqk_variable<ppT>> w1;
    std::shared_ptr<Fqk_variable<ppT>> w0;
    std::shared_ptr<Fqk_variable<ppT>> result;

    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_el_inv;
    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_el_q_2_minus_1;
    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_el_inv_q_2_minus_1;

    std::shared_ptr<exponentiation_gadget<
        FqkT<ppT>,
        Fp4_variable,
        Fp4_mul_gadget,
        Fp4_cyclotomic_sqr_gadget,
        libff::mnt4_q_limbs>>
        compute_w1;
    std::shared_ptr<exponentiation_gadget<
        FqkT<ppT>,
        Fp4_variable,
        Fp4_mul_gadget,
        Fp4_cyclotomic_sqr_gadget,
        libff::mnt4_q_limbs>>
        compute_w0;
    std::shared_ptr<Fqk_mul_gadget<ppT>> compute_result;

    pb_variable<FieldT> result_is_one;

    mnt6_final_exp_gadget(
        protoboard<FieldT> &pb,
        const Fqk_variable<ppT> &el,
        const pb_variable<FieldT> &result_is_one,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

} // namespace libsnark

#include "libsnark/gadgetlib1/gadgets/pairing/mnt/mnt_final_exponentiation.tcc"

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_MNT_MNT_FINAL_EXPONENTIATION_HPP_
