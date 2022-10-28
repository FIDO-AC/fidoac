/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MILLER_LOOP_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MILLER_LOOP_HPP_

#include "libsnark/gadgetlib1/gadgets/curves/weierstrass_g1_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/curves/weierstrass_g2_gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp12_2over3over2_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp2_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_precomputation.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bw6_761_pairing_params.hpp"

#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>

namespace libsnark
{

/// Given some current f in Fqk, the pairing parameter P in G1, and the
/// precomputed coefficients for the function of some line function ell(),
/// compute:
///   f * ell(P)
/// Note that this gadget allocates the variable to hold the resulting value of
/// f.
template<typename ppT>
class bls12_377_ate_compute_f_ell_P : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using FqkT = libff::Fqk<other_curve<ppT>>;

    Fqe_mul_by_lc_gadget<ppT> _compute_ell_vv_times_Px;
    Fqe_mul_by_lc_gadget<ppT> _compute_ell_vw_times_Py;
    Fp12_2over3over2_mul_by_024_gadget<FqkT> _compute_f_mul_ell_P;

    bls12_377_ate_compute_f_ell_P(
        protoboard<FieldT> &pb,
        const pb_linear_combination<FieldT> &Px,
        const pb_linear_combination<FieldT> &Py,
        const bls12_377_ate_ell_coeffs<ppT> &ell_coeffs,
        const Fp12_2over3over2_variable<FqkT> &f,
        const Fp12_2over3over2_variable<FqkT> &f_out,
        const std::string &annotation_prefix);

    const Fp12_2over3over2_variable<FqkT> &result() const;
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

template<typename ppT>
class bls12_377_miller_loop_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using FqeT = libff::Fqe<other_curve<ppT>>;
    using FqkT = libff::Fqk<other_curve<ppT>>;
    using Fq6T = typename FqkT::my_Fp6;

    Fp12_2over3over2_variable<FqkT> _f0;

    // Squaring of f
    std::vector<std::shared_ptr<Fp12_2over3over2_square_gadget<FqkT>>>
        _f_squared;

    // f * ell(P) (for both double and add steps)
    std::vector<std::shared_ptr<bls12_377_ate_compute_f_ell_P<ppT>>> _f_ell_P;

    bls12_377_miller_loop_gadget(
        protoboard<FieldT> &pb,
        const bls12_377_G1_precomputation<ppT> &prec_P,
        const bls12_377_G2_precomputation<ppT> &prec_Q,
        const Fqk_variable<ppT> &result,
        const std::string &annotation_prefix);

    const Fp12_2over3over2_variable<FqkT> &result() const;
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

template<typename ppT>
class bls12_377_e_over_e_miller_loop_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using FqkT = libff::Fqk<other_curve<ppT>>;

    Fp12_2over3over2_variable<FqkT> _f0;
    pb_linear_combination<FieldT> _minus_P2_Y;

    // Squaring of f
    std::vector<std::shared_ptr<Fp12_2over3over2_square_gadget<FqkT>>>
        _f_squared;

    // f * ell(P) (for both double and add steps)
    std::vector<std::shared_ptr<bls12_377_ate_compute_f_ell_P<ppT>>> _f_ell_P;

    bls12_377_e_over_e_miller_loop_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const bls12_377_G1_precomputation<ppT> &P1_prec,
        const bls12_377_G2_precomputation<ppT> &Q1_prec,
        const bls12_377_G1_precomputation<ppT> &P2_prec,
        const bls12_377_G2_precomputation<ppT> &Q2_prec,
        const Fp12_2over3over2_variable<FqkT> &result,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

template<typename ppT>
class bls12_377_e_times_e_times_e_over_e_miller_loop_gadget
    : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;
    using FqkT = libff::Fqk<other_curve<ppT>>;

    Fp12_2over3over2_variable<FqkT> _f0;
    pb_linear_combination<FieldT> _minus_P4_Y;

    // Squaring of f
    std::vector<std::shared_ptr<Fp12_2over3over2_square_gadget<FqkT>>>
        _f_squared;

    // f * ell(P) (for both double and add steps)
    std::vector<std::shared_ptr<bls12_377_ate_compute_f_ell_P<ppT>>> _f_ell_P;

    bls12_377_e_times_e_times_e_over_e_miller_loop_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const bls12_377_G1_precomputation<ppT> &P1_prec,
        const bls12_377_G2_precomputation<ppT> &Q1_prec,
        const bls12_377_G1_precomputation<ppT> &P2_prec,
        const bls12_377_G2_precomputation<ppT> &Q2_prec,
        const bls12_377_G1_precomputation<ppT> &P3_prec,
        const bls12_377_G2_precomputation<ppT> &Q3_prec,
        const bls12_377_G1_precomputation<ppT> &P4_prec,
        const bls12_377_G2_precomputation<ppT> &Q4_prec,
        const Fp12_2over3over2_variable<FqkT> &result,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

} // namespace libsnark

#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_miller_loop.tcc"

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MILLER_LOOP_HPP_
