/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MEMBERSHIP_CHECK_GADGETS_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MEMBERSHIP_CHECK_GADGETS_HPP_

#include "libsnark/gadgetlib1/gadgets/curves/weierstrass_g1_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/curves/weierstrass_g2_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp12_2over3over2_gadgets.hpp"

#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>

namespace libsnark
{

/// Curve equation (via the generic G1_checker_gadget) and subgroup membership
/// check for BLS12-377 G1 variables.
template<typename wppT>
class bls12_377_G1_membership_check_gadget : public gadget<libff::Fr<wppT>>
{
public:
    using nppT = other_curve<wppT>;
    using G1_mul_by_cofactor_gadget =
        G1_mul_by_const_scalar_gadget<wppT, libff::G1<nppT>::h_limbs>;

    // Point P to check
    G1_variable<wppT> _P;
    // P' s.t. [h]P' = P
    G1_variable<wppT> _P_primed;
    // Check that P' \in E(Fq)
    G1_checker_gadget<wppT> _P_primed_checker;
    // [h]P' = P condition
    G1_mul_by_cofactor_gadget _P_primed_mul_cofactor;

    bls12_377_G1_membership_check_gadget(
        protoboard<libff::Fr<wppT>> &pb,
        const G1_variable<wppT> &P,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Untwist-Frobenius-Twist operation on BLS12-377 G2 elements. (Note that
/// evaluate should be called on the result, or its components, before using it
/// in witness generation).
template<typename wppT>
G2_variable<wppT> bls12_377_g2_untwist_frobenius_twist(
    protoboard<libff::Fr<wppT>> &pb,
    const G2_variable<wppT> &g2,
    size_t exp,
    const std::string &annotation_prefix);

/// Curve equation (via generic G2_checker_gadget) and subgroup membership
/// check for BLS12-377 G2 variables.
template<typename wppT>
class bls12_377_G2_membership_check_gadget : public gadget<libff::Fr<wppT>>
{
public:
    // Follows libff implementation of bls12_377_G2::is_in_safe_subgroup().
    // See: libff/algebra/curves/bls12_377/bls12_377_g2.cpp.

    // Check that[h1.r] P == 0, where
    //   [h1.r]P is P + [t](\psi(P) - P) - \psi^2(P)
    // (See bls12_377.sage).
    // Note that in this case we check that:
    //   P + [t](\psi(P) - P) = \psi^2(P)
    // since G2_variable cannot represent 0 (in G2).

    // Check P is well-formed
    G2_checker_gadget<wppT> _P_checker;
    // \psi(P) - P
    G2_add_gadget<wppT> _psi_P_minus_P;
    // [t](\psi(P) - P)
    G2_mul_by_const_scalar_gadget<wppT, libff::bls12_377_r_limbs>
        _t_times_psi_P_minus_P;
    // P + [t](\psi(P) - P)
    G2_add_gadget<wppT> _P_plus_t_times_psi_P_minus_P;
    // P + [t](\psi(P) - P) = \psi^2(P)
    G2_equality_gadget<wppT> _h1_r_P_equals_zero;

    bls12_377_G2_membership_check_gadget(
        protoboard<libff::Fr<wppT>> &pb,
        G2_variable<wppT> &g2,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

} // namespace libsnark

#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_membership_check_gadgets.tcc"

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MEMBERSHIP_CHECK_GADGETS_HPP_
