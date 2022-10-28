/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_VERIFIERS_KZG10_VERIFIER_GADGET_TCC_
#define LIBSNARK_GADGETLIB1_GADGETS_VERIFIERS_KZG10_VERIFIER_GADGET_TCC_

#include "libsnark/gadgetlib1/gadgets/verifiers/kzg10_verifier_gadget.hpp"

namespace libsnark
{

template<typename ppT>
kzg10_srs_variable<ppT>::kzg10_srs_variable(
    protoboard<libff::Fr<ppT>> &pb,
    const size_t max_degree,
    const std::string &annotation_prefix)
    : alpha_g2(pb, FMT(annotation_prefix, " alpha_g2"))

{
    alpha_powers_g1.reserve(max_degree + 1);
    for (size_t i = 0; i < max_degree + 1; ++i) {
        alpha_powers_g1.emplace_back(
            pb, FMT(annotation_prefix, " alpha_powers_g1[%zu]", i));
    }
}

template<typename ppT>
void kzg10_srs_variable<ppT>::generate_r1cs_witness(
    const typename kzg10<npp>::srs &srs)
{
    assert(srs.alpha_powers_g1.size() == alpha_powers_g1.size());

    for (size_t i = 0; i < srs.alpha_powers_g1.size(); ++i) {
        alpha_powers_g1[i].generate_r1cs_witness(srs.alpha_powers_g1[i]);
    }
    alpha_g2.generate_r1cs_witness(srs.alpha_g2);
}

template<typename ppT>
kzg10_verifier_gadget<ppT>::kzg10_verifier_gadget(
    protoboard<libff::Fr<ppT>> &pb,
    const kzg10_srs_variable<ppT> &srs,
    const kzg10_commitment_variable<ppT> &commitment,
    pb_linear_combination<libff::Fr<ppT>> i,
    pb_linear_combination<libff::Fr<ppT>> poly_eval,
    const kzg10_witness_variable<ppT> &witness,
    pb_variable<libff::Fr<ppT>> result,
    const std::string &annotation_prefix)
    : gadget<libff::Fr<ppT>>(pb, annotation_prefix)

    , i_in_G2(pb, FMT(annotation_prefix, " i_in_G2"))
    , compute_i_in_G2(
          pb,
          i,
          G2_variable<ppT>(
              pb,
              libff::G2<other_curve<ppT>>::one(),
              FMT(annotation_prefix, " one_G2")),
          i_in_G2,
          FMT(annotation_prefix, " compute_i_in_G2"))

    , B(pb, FMT(annotation_prefix, " B"))
    , compute_B(
          pb,
          srs.alpha_g2,
          -i_in_G2.value,
          B,
          FMT(annotation_prefix, " compute_B"))

    , poly_eval_in_G1(pb, FMT(annotation_prefix, " poly_eval_in_G1"))
    , compute_poly_eval_in_G1(
          pb,
          poly_eval,
          G1_variable<ppT>(
              pb,
              libff::G1<other_curve<ppT>>::one(),
              FMT(annotation_prefix, " one_G1")),
          poly_eval_in_G1,
          FMT(annotation_prefix, " compute_poly_eval_in_G1"))

    , C(pb, FMT(annotation_prefix, " C"))
    , compute_C(
          pb,
          commitment,
          -poly_eval_in_G1.value,
          C,
          FMT(annotation_prefix, " compute_C"))

    , A_precomp()
    , compute_A_precomp(
          pb, witness, A_precomp, FMT(annotation_prefix, " compute_A_precomp"))
    , B_precomp()
    , compute_B_precomp(
          pb, B, B_precomp, FMT(annotation_prefix, " compute_B_precomp"))
    , C_precomp()
    , compute_C_precomp(
          pb, C, C_precomp, FMT(annotation_prefix, " compute_C_precomp"))
    , D_precomp(
          pb,
          libff::G2<other_curve<ppT>>::one(),
          FMT(annotation_prefix, " D_precomp"))

    , check_result(pb_variable_allocate<FieldT>(
          pb, FMT(annotation_prefix, " check_result")))
    , check_pairing_equality(
          pb,
          A_precomp,
          B_precomp,
          C_precomp,
          D_precomp,
          check_result,
          FMT(annotation_prefix, " check_pairing_equality"))

    , group_elements_non_zero(pb_variable_allocate<FieldT>(
          pb, FMT(annotation_prefix, " group_elements_non_zero")))
    , result(result)
{
}

template<typename ppT>
void kzg10_verifier_gadget<ppT>::generate_r1cs_constraints()
{
    compute_i_in_G2.generate_r1cs_constraints();
    compute_B.generate_r1cs_constraints();
    compute_poly_eval_in_G1.generate_r1cs_constraints();
    compute_C.generate_r1cs_constraints();
    compute_A_precomp.generate_r1cs_constraints();
    compute_B_precomp.generate_r1cs_constraints();
    compute_C_precomp.generate_r1cs_constraints();
    check_pairing_equality.generate_r1cs_constraints();

    // group_elements_non_zero =
    //   (1 - i_in_G2.is_identity) * (1 - poly_eval_in_G1.is_identity)
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(
            FieldT::one() - i_in_G2.is_identity,
            FieldT::one() - poly_eval_in_G1.is_identity,
            group_elements_non_zero),
        FMT(this->annotation_prefix, " compute_group_elements_non_zero"));

    // result = group_elements_non_zero * check_result
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(group_elements_non_zero, check_result, result),
        FMT(this->annotation_prefix, " compute_result"));
}

template<typename ppT> void kzg10_verifier_gadget<ppT>::generate_r1cs_witness()
{
    compute_i_in_G2.generate_r1cs_witness();
    // compute_B.B = -i_in_G2.value. Evaluate the result of the negation.
    compute_B.B.Y->evaluate();
    compute_B.generate_r1cs_witness();
    compute_poly_eval_in_G1.generate_r1cs_witness();
    // compute_C.B = -poly_eval_in_G1.value.  Evaluate the result of negation.
    compute_C.B.Y.evaluate(this->pb);
    compute_C.generate_r1cs_witness();
    compute_A_precomp.generate_r1cs_witness();
    compute_B_precomp.generate_r1cs_witness();
    compute_C_precomp.generate_r1cs_witness();
    check_pairing_equality.generate_r1cs_witness();

    const FieldT group_elements_non_zero_val =
        (FieldT::one() - this->pb.lc_val(i_in_G2.is_identity)) *
        (FieldT::one() - this->pb.lc_val(poly_eval_in_G1.is_identity));
    const FieldT result_val =
        group_elements_non_zero_val * this->pb.val(check_result);

    this->pb.val(group_elements_non_zero) = group_elements_non_zero_val;
    this->pb.val(result) = result_val;
}

} // namespace libsnark

#endif // LIBSNARK_GADGETLIB1_GADGETS_VERIFIERS_KZG10_VERIFIER_GADGET_TCC_
