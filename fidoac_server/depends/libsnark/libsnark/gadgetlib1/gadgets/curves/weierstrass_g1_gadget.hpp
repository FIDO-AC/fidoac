/** @file
 *****************************************************************************

 Declaration of interfaces for G1 gadgets.

 The gadgets verify curve arithmetic in G1 = E(F) where E/F: y^2 = x^3 + A * X +
 B is an elliptic curve over F in short Weierstrass form.

 *****************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef WEIERSTRASS_G1_GADGET_HPP_
#define WEIERSTRASS_G1_GADGET_HPP_

#include "libsnark/gadgetlib1/gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/curves/scalar_multiplication.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/pairing_params.hpp"

#include <libff/algebra/curves/public_params.hpp>

namespace libsnark
{

/// Gadget that represents a G1 variable.
template<typename ppT> class G1_variable : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;

    pb_linear_combination<FieldT> X;
    pb_linear_combination<FieldT> Y;

    pb_linear_combination_array<FieldT> all_vars;

    G1_variable(protoboard<FieldT> &pb, const std::string &annotation_prefix);
    G1_variable(
        protoboard<FieldT> &pb,
        const libff::G1<other_curve<ppT>> &P,
        const std::string &annotation_prefix);

    // NOTE: pb and annotation_prefix are redundant here, but required because
    // of the inheritance from gadget (and in order to construct a new
    // pb_linear_combination in operator-()).
    G1_variable(
        protoboard<FieldT> &pb,
        const pb_linear_combination<FieldT> &X,
        const pb_linear_combination<FieldT> &Y,
        const std::string &annotation_prefix);

    G1_variable operator-() const;

    void generate_r1cs_witness(const libff::G1<other_curve<ppT>> &elt);

    libff::G1<other_curve<ppT>> get_element() const;

    // (See a comment in r1cs_ppzksnark_verifier_gadget.hpp about why
    // we mark this function noinline.) TODO: remove later
    static size_t __attribute__((noinline)) size_in_bits();
    static size_t num_variables();
};

/// Depending on the value of a selector variable (which must be 0 or
/// 1), choose between two G1_variable objects (zero_case and
/// one_case),
template<typename ppT>
class G1_variable_selector_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using Field = libff::Fr<ppT>;

    const pb_linear_combination<Field> selector;
    const G1_variable<ppT> zero_case;
    const G1_variable<ppT> one_case;
    G1_variable<ppT> result;

    G1_variable_selector_gadget(
        protoboard<Field> &pb,
        const pb_linear_combination<Field> &selector,
        const G1_variable<ppT> &zero_case,
        const G1_variable<ppT> &one_case,
        const G1_variable<ppT> &result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/**
 * Gadget that creates constraints for the validity of a G1 variable.
 */
template<typename ppT> class G1_checker_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;

    G1_variable<ppT> P;
    pb_variable<FieldT> P_X_squared;
    pb_variable<FieldT> P_Y_squared;

    G1_checker_gadget(
        protoboard<FieldT> &pb,
        const G1_variable<ppT> &P,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/**
 * Gadget that creates constraints for G1 addition.
 */
template<typename ppT> class G1_add_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;

    pb_variable<FieldT> lambda;
    pb_variable<FieldT> inv;

    G1_variable<ppT> A;
    G1_variable<ppT> B;
    G1_variable<ppT> result;

    G1_add_gadget(
        protoboard<FieldT> &pb,
        const G1_variable<ppT> &A,
        const G1_variable<ppT> &B,
        const G1_variable<ppT> &result,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/**
 * Gadget that creates constraints for G1 doubling.
 */
template<typename ppT> class G1_dbl_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;

    pb_variable<FieldT> Xsquared;
    pb_variable<FieldT> lambda;

    G1_variable<ppT> A;
    G1_variable<ppT> result;

    G1_dbl_gadget(
        protoboard<FieldT> &pb,
        const G1_variable<ppT> &A,
        const G1_variable<ppT> &result,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/**
 * Gadget that creates constraints for G1 multi-scalar multiplication.
 */
template<typename ppT>
class G1_multiscalar_mul_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;

    std::vector<G1_variable<ppT>> computed_results;
    std::vector<G1_variable<ppT>> chosen_results;
    std::vector<G1_add_gadget<ppT>> adders;
    std::vector<G1_dbl_gadget<ppT>> doublers;

    G1_variable<ppT> base;
    pb_variable_array<FieldT> scalars;
    std::vector<G1_variable<ppT>> points;
    std::vector<G1_variable<ppT>> points_and_powers;
    G1_variable<ppT> result;

    const size_t elt_size;
    const size_t num_points;
    const size_t scalar_size;

    G1_multiscalar_mul_gadget(
        protoboard<FieldT> &pb,
        const G1_variable<ppT> &base,
        const pb_variable_array<FieldT> &scalars,
        const size_t elt_size,
        const std::vector<G1_variable<ppT>> &points,
        const G1_variable<ppT> &result,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Multiplication by constant scalar (leverages
/// point_mul_by_const_scalar_gadget - scalar_multiplication.hpp).
template<typename wppT, mp_size_t scalarLimbs>
using G1_mul_by_const_scalar_gadget = point_mul_by_const_scalar_gadget<
    libff::G1<other_curve<wppT>>,
    G1_variable<wppT>,
    G1_add_gadget<wppT>,
    G1_dbl_gadget<wppT>,
    libff::bigint<scalarLimbs>>;

template<typename wppT>
using G1_variable_or_identity =
    variable_or_identity<wppT, libff::G1<other_curve<wppT>>, G1_variable<wppT>>;

template<typename wppT>
using G1_variable_or_identity_selector_gadget = variable_or_identity_selector<
    wppT,
    libff::G1<other_curve<wppT>>,
    G1_variable<wppT>,
    G1_variable_selector_gadget<wppT>>;

template<typename wppT>
using G1_variable_and_variable_or_identity_selector_gadget =
    variable_and_variable_or_identity_selector<
        wppT,
        libff::G1<other_curve<wppT>>,
        G1_variable<wppT>,
        G1_variable_selector_gadget<wppT>>;

template<typename wppT>
using G1_add_variable_or_identity_gadget = add_variable_or_identity<
    wppT,
    libff::G1<other_curve<wppT>>,
    G1_variable<wppT>,
    G1_variable_selector_gadget<wppT>,
    G1_add_gadget<wppT>>;

template<typename wppT>
using G1_add_variable_and_variable_or_identity_gadget =
    add_variable_and_variable_or_identity<
        wppT,
        libff::G1<other_curve<wppT>>,
        G1_variable<wppT>,
        G1_variable_selector_gadget<wppT>,
        G1_add_gadget<wppT>>;

template<typename wppT>
using G1_dbl_variable_or_identity_gadget = dbl_variable_or_identity<
    wppT,
    libff::G1<other_curve<wppT>>,
    G1_variable<wppT>,
    G1_dbl_gadget<wppT>>;

template<typename wppT>
using G1_mul_by_scalar_gadget = point_mul_by_scalar_gadget<
    wppT,
    libff::G1<other_curve<wppT>>,
    G1_variable<wppT>,
    G1_variable_selector_gadget<wppT>,
    G1_add_gadget<wppT>,
    G1_dbl_gadget<wppT>>;

} // namespace libsnark

#include <libsnark/gadgetlib1/gadgets/curves/weierstrass_g1_gadget.tcc>

#endif // WEIERSTRASS_G1_GADGET_TCC_
