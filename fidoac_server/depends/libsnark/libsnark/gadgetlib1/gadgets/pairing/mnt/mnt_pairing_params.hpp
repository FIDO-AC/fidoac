/** @file
 *****************************************************************************

 Declaration of specializations of pairing_selector<ppT> to
 - pairing_selector<libff::mnt4_pp>, and
 - pairing_selector<libff::mnt6_pp>.

 See pairing_params.hpp .

 *****************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_MNT_MNT_PAIRING_PARAMS_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_MNT_MNT_PAIRING_PARAMS_HPP_

#include "libsnark/gadgetlib1/gadgets/fields/fp2_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp3_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp4_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp6_2over3_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/mnt/mnt_final_exponentiation.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/mnt/mnt_miller_loop.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/mnt/mnt_precomputation.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/pairing_params.hpp"

#include <libff/algebra/curves/mnt/mnt4/mnt4_pp.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_pp.hpp>

namespace libsnark
{

/**
 * Specialization for MNT4.
 */
template<> class pairing_selector<libff::mnt4_pp>
{
public:
    typedef libff::Fr<libff::mnt4_pp> FieldT;
    typedef libff::Fqe<libff::mnt6_pp> FqeT;
    typedef libff::Fqk<libff::mnt6_pp> FqkT;

    typedef Fp3_variable<FqeT> Fqe_variable_type;
    typedef Fp3_mul_gadget<FqeT> Fqe_mul_gadget_type;
    typedef Fp3_mul_by_lc_gadget<FqeT> Fqe_mul_by_lc_gadget_type;
    typedef Fp3_sqr_gadget<FqeT> Fqe_sqr_gadget_type;

    typedef Fp6_2over3_variable<FqkT> Fqk_variable_type;
    typedef Fp6_2over3_mul_gadget<FqkT> Fqk_mul_gadget_type;
    typedef Fp6_2over3_sqr_gadget<FqkT> Fqk_sqr_gadget_type;

    typedef libff::mnt6_pp other_curve_type;

    typedef G1_checker_gadget<libff::mnt4_pp> G1_checker_type;
    typedef G2_checker_gadget<libff::mnt4_pp> G2_checker_type;

    typedef mnt_G1_precomputation<libff::mnt4_pp> G1_precomputation_type;
    typedef mnt_G2_precomputation<libff::mnt4_pp> G2_precomputation_type;
    typedef mnt_precompute_G1_gadget<libff::mnt4_pp> precompute_G1_gadget_type;
    typedef mnt_precompute_G2_gadget<libff::mnt4_pp> precompute_G2_gadget_type;

    typedef mnt_miller_loop_gadget<libff::mnt4_pp> miller_loop_gadget_type;
    typedef mnt_e_over_e_miller_loop_gadget<libff::mnt4_pp>
        e_over_e_miller_loop_gadget_type;
    typedef mnt_e_times_e_over_e_miller_loop_gadget<libff::mnt4_pp>
        e_times_e_over_e_miller_loop_gadget_type;
    typedef mnt_e_times_e_times_e_over_e_miller_loop_gadget<libff::mnt4_pp>
        e_times_e_times_e_over_e_miller_loop_gadget_type;
    typedef mnt4_final_exp_gadget<libff::mnt4_pp> final_exp_gadget_type;
};

/**
 * Specialization for MNT6.
 */
template<> class pairing_selector<libff::mnt6_pp>
{
public:
    typedef libff::Fr<libff::mnt6_pp> FieldT;

    typedef libff::Fqe<libff::mnt4_pp> FqeT;
    typedef libff::Fqk<libff::mnt4_pp> FqkT;

    typedef Fp2_variable<FqeT> Fqe_variable_type;
    typedef Fp2_mul_gadget<FqeT> Fqe_mul_gadget_type;
    typedef Fp2_mul_by_lc_gadget<FqeT> Fqe_mul_by_lc_gadget_type;
    typedef Fp2_sqr_gadget<FqeT> Fqe_sqr_gadget_type;

    typedef Fp4_variable<FqkT> Fqk_variable_type;
    typedef Fp4_mul_gadget<FqkT> Fqk_mul_gadget_type;
    typedef Fp4_sqr_gadget<FqkT> Fqk_sqr_gadget_type;

    typedef libff::mnt4_pp other_curve_type;

    typedef G1_checker_gadget<libff::mnt6_pp> G1_checker_type;
    typedef G2_checker_gadget<libff::mnt6_pp> G2_checker_type;

    typedef mnt_G1_precomputation<libff::mnt6_pp> G1_precomputation_type;
    typedef mnt_G2_precomputation<libff::mnt6_pp> G2_precomputation_type;
    typedef mnt_precompute_G1_gadget<libff::mnt6_pp> precompute_G1_gadget_type;
    typedef mnt_precompute_G2_gadget<libff::mnt6_pp> precompute_G2_gadget_type;

    typedef mnt_miller_loop_gadget<libff::mnt6_pp> miller_loop_gadget_type;
    typedef mnt_e_over_e_miller_loop_gadget<libff::mnt6_pp>
        e_over_e_miller_loop_gadget_type;
    typedef mnt_e_times_e_over_e_miller_loop_gadget<libff::mnt6_pp>
        e_times_e_over_e_miller_loop_gadget_type;
    typedef mnt_e_times_e_times_e_over_e_miller_loop_gadget<libff::mnt6_pp>
        e_times_e_times_e_over_e_miller_loop_gadget_type;
    typedef mnt6_final_exp_gadget<libff::mnt6_pp> final_exp_gadget_type;
};

// Parameters internal to the mnt code
template<typename ppT> class mnt_pairing_params;

template<> class mnt_pairing_params<libff::mnt4_pp>
{
public:
    typedef Fp6_2over3_mul_by_2345_gadget<libff::Fqk<libff::mnt6_pp>>
        Fqk_special_mul_gadget_type;

    static const constexpr libff::bigint<libff::mnt6_Fr::num_limbs>
        &pairing_loop_count = libff::mnt6_ate_loop_count;
};

template<> class mnt_pairing_params<libff::mnt6_pp>
{
public:
    typedef Fp4_mul_gadget<libff::Fqk<libff::mnt4_pp>>
        Fqk_special_mul_gadget_type;

    static const constexpr libff::bigint<libff::mnt4_Fr::num_limbs>
        &pairing_loop_count = libff::mnt4_ate_loop_count;
};

} // namespace libsnark

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_MNT_MNT_PAIRING_PARAMS_HPP_
