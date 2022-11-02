/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_PAIRING_PARAMS_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_PAIRING_PARAMS_HPP_

#include "libsnark/gadgetlib1/gadgets/fields/fp12_2over3over2_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp2_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_final_exponentiation.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_membership_check_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_miller_loop.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_precomputation.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/pairing_params.hpp"

#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_pp.hpp>

namespace libsnark
{

template<typename ppT> class bls12_377_G1_precomputation;
template<typename ppT> class bls12_377_G1_precompute_gadget;
template<typename ppT> class bls12_377_G2_precomputation;
template<typename ppT> class bls12_377_G2_precompute_gadget;
template<typename ppT> class bls12_377_miller_loop_gadget;
template<typename ppT>
class bls12_377_e_times_e_times_e_over_e_miller_loop_gadget;
template<typename ppT> class bls12_377_final_exp_gadget;

// Parameters for creating BW6-761 proofs that include statements about
// BLS12_377 pairings.
template<> class pairing_selector<libff::bw6_761_pp>
{
public:
    static_assert(
        std::is_same<
            libff::Fr<libff::bw6_761_pp>,
            libff::Fq<libff::bls12_377_pp>>::value,
        "Field types do not match");

    typedef libff::Fr<libff::bw6_761_pp> FieldT;
    typedef libff::Fqe<libff::bls12_377_pp> FqeT;
    typedef libff::Fqk<libff::bls12_377_pp> FqkT;

    typedef Fp2_variable<FqeT> Fqe_variable_type;
    typedef Fp2_mul_gadget<FqeT> Fqe_mul_gadget_type;
    typedef Fp2_mul_by_lc_gadget<FqeT> Fqe_mul_by_lc_gadget_type;
    typedef Fp2_sqr_gadget<FqeT> Fqe_sqr_gadget_type;

    typedef Fp12_2over3over2_variable<FqkT> Fqk_variable_type;
    typedef Fp12_2over3over2_mul_gadget<FqkT> Fqk_mul_gadget_type;
    typedef Fp12_2over3over2_square_gadget<FqkT> Fqk_sqr_gadget_type;

    typedef libff::bls12_377_pp other_curve_type;

    typedef bls12_377_G1_membership_check_gadget<libff::bw6_761_pp>
        G1_checker_type;
    typedef bls12_377_G2_membership_check_gadget<libff::bw6_761_pp>
        G2_checker_type;

    typedef bls12_377_G1_precomputation<libff::bw6_761_pp>
        G1_precomputation_type;
    typedef bls12_377_G1_precompute_gadget<libff::bw6_761_pp>
        precompute_G1_gadget_type;

    typedef bls12_377_G2_precomputation<libff::bw6_761_pp>
        G2_precomputation_type;
    typedef bls12_377_G2_precompute_gadget<libff::bw6_761_pp>
        precompute_G2_gadget_type;

    typedef bls12_377_miller_loop_gadget<libff::bw6_761_pp>
        miller_loop_gadget_type;

    typedef bls12_377_e_over_e_miller_loop_gadget<libff::bw6_761_pp>
        e_over_e_miller_loop_gadget_type;

    // Not implemented:
    // typedef bls12_377_e_times_e_over_e_miller_loop_gadget
    //     e_times_e_over_e_miller_loop_gadget_type;

    typedef bls12_377_e_times_e_times_e_over_e_miller_loop_gadget<
        libff::bw6_761_pp>
        e_times_e_times_e_over_e_miller_loop_gadget_type;

    typedef bls12_377_final_exp_gadget<libff::bw6_761_pp> final_exp_gadget_type;
};

} // namespace libsnark

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_PAIRING_PARAMS_HPP_
