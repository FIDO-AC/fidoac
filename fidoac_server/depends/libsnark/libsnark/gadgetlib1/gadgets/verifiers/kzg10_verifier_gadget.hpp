/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_VERIFIERS_KZG10_VERIFIER_GADGET_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_VERIFIERS_KZG10_VERIFIER_GADGET_HPP_

#include "libsnark/gadgetlib1/gadgets/curves/weierstrass_g1_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/curves/weierstrass_g2_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/pairing_checks.hpp"
#include "libsnark/gadgetlib1/protoboard.hpp"
#include "libsnark/polynomial_commitments/kzg10.hpp"

/// Reference:
/// - [KZG10]:
///   Title: "Constant-Size Commitments to Polynomials and Their Applications"
///   eprint: https://www.iacr.org/archive/asiacrypt2010/6477178/6477178.pdf
///
/// The native implementation can be found in:
///   libsnark/polynomial_commitments/kzg10.{hpp,tcc}

namespace libsnark
{

/// The SRS for the KZG10 scheme, as protoboard variables. Names match those
/// used in the native implementation.
template<typename ppT> class kzg10_srs_variable
{
public:
    using npp = other_curve<ppT>;

    std::vector<G1_variable<ppT>> alpha_powers_g1;
    G2_variable<ppT> alpha_g2;

    kzg10_srs_variable(
        protoboard<libff::Fr<ppT>> &pb,
        const size_t max_degree,
        const std::string &annotation_prefix);

    void generate_r1cs_witness(const typename kzg10<npp>::srs &srs);
};

/// The polynomial for the KZG10 scheme, as protoboard variables. This is just
/// a G1_variable (see the native implementation).
template<typename ppT> using kzg10_commitment_variable = G1_variable<ppT>;

/// The witness for the evaluation of a polynomial, as protoboard variables.
/// This is also a single G1_variable (see the native implementation).
template<typename ppT> using kzg10_witness_variable = G1_variable<ppT>;

/// Uses a nested pairing (via a pairing selector) to implement the
/// verification step of [KZG10]. See the native implementation for details.
///
/// TODO: this gadget does not currently support either the evaluation point i
/// or the result of polynomial evaluation being zero. This could potentially
/// be addressed by further use of gadgets that handle variable_or_identity
/// inputs.
template<typename ppT>
class kzg10_verifier_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;

    // From kzg10.tcc:  We verify the equality:
    //
    //   \psi(\alpha) (\alpha - i) = \phi(\alpha) - \phi(i)
    //                             = commit - phi_i
    //
    // via the pairing equality:
    //
    //   e([\psi(\alpha)]_1, [\alpha - i]_2) = e(commit - [phi_i]_1, [1]_2)
    //
    // We use check_e_equals_e_gadget to check that:
    //   e(A, B) = e(C, D)
    // where:
    //   A = witness
    //   B = srs.alpha_g2 - i * G2::one()
    //   C = commit - poly_eval * G1::one()
    //   D = G2::one()

    // B = srs.alpha_g2 - i * G2::one()
    G2_variable_or_identity<ppT> i_in_G2;
    G2_mul_by_scalar_gadget<ppT> compute_i_in_G2;
    G2_variable<ppT> B;
    G2_add_gadget<ppT> compute_B;

    // C = commit - poly_eval * G1::one()
    G1_variable_or_identity<ppT> poly_eval_in_G1;
    G1_mul_by_scalar_gadget<ppT> compute_poly_eval_in_G1;
    G1_variable<ppT> C;
    G1_add_gadget<ppT> compute_C;

    // Pairing computation
    G1_precomputation<ppT> A_precomp;
    precompute_G1_gadget<ppT> compute_A_precomp;
    G2_precomputation<ppT> B_precomp;
    precompute_G2_gadget<ppT> compute_B_precomp;
    G1_precomputation<ppT> C_precomp;
    precompute_G1_gadget<ppT> compute_C_precomp;
    // D_precomp is computed from (constant) G2::one(), and baked into the
    // circuit, saving a few constraints.
    G2_precomputation<ppT> D_precomp;

    pb_variable<libff::Fr<ppT>> check_result;
    check_e_equals_e_gadget<ppT> check_pairing_equality;

    // group_elements_non_zero =
    //   (1 - i_in_G2.is_zero) * (1 - poly_eval_in_G1.is_zero)
    pb_variable<libff::Fr<ppT>> group_elements_non_zero;

    // result = group_elements_non_zero * check_result
    pb_variable<libff::Fr<ppT>> result;

    kzg10_verifier_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const kzg10_srs_variable<ppT> &srs,
        const kzg10_commitment_variable<ppT> &commitmennt,
        pb_linear_combination<libff::Fr<ppT>> i,
        pb_linear_combination<libff::Fr<ppT>> poly_eval,
        const kzg10_witness_variable<ppT> &witness,
        pb_variable<libff::Fr<ppT>> result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

} // namespace libsnark

#include "libsnark/gadgetlib1/gadgets/verifiers/kzg10_verifier_gadget.tcc"

#endif // LIBSNARK_GADGETLIB1_GADGETS_VERIFIERS_KZG10_VERIFIER_GADGET_HPP_
