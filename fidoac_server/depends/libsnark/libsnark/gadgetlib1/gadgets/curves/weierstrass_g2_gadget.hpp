/** @file
 *****************************************************************************

 Declaration of interfaces for G2 gadgets.

 The gadgets verify curve arithmetic in G2 = E'(F) where E'/F^e: y^2 = x^3 + A'
 * X + B' is an elliptic curve over F^e in short Weierstrass form.

 *****************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef WEIERSTRASS_G2_GADGET_HPP_
#define WEIERSTRASS_G2_GADGET_HPP_

#include "libsnark/gadgetlib1/gadget.hpp"
#include "libsnark/gadgetlib1/gadgets/curves/scalar_multiplication.hpp"
#include "libsnark/gadgetlib1/gadgets/fields/fp2_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/pairing_params.hpp"

#include <libff/algebra/curves/public_params.hpp>
#include <libff/algebra/fields/fp2.hpp>
#include <memory>

namespace libsnark
{

/// Gadget that represents a G2 variable.
template<typename ppT> class G2_variable : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;
    typedef libff::Fqe<other_curve<ppT>> FqeT;
    typedef libff::Fqk<other_curve<ppT>> FqkT;

    std::shared_ptr<Fqe_variable<ppT>> X;
    std::shared_ptr<Fqe_variable<ppT>> Y;

    pb_linear_combination_array<FieldT> all_vars;

    G2_variable(protoboard<FieldT> &pb, const std::string &annotation_prefix);
    G2_variable(
        protoboard<FieldT> &pb,
        const libff::G2<other_curve<ppT>> &Q,
        const std::string &annotation_prefix);

    G2_variable(
        protoboard<FieldT> &pb,
        const Fqe_variable<ppT> &X,
        const Fqe_variable<ppT> &Y,
        const std::string &annotation_prefix);

    G2_variable operator-() const;

    void generate_r1cs_witness(const libff::G2<other_curve<ppT>> &Q);

    libff::G2<other_curve<ppT>> get_element() const;

    // (See a comment in r1cs_ppzksnark_verifier_gadget.hpp about why
    // we mark this function noinline.) TODO: remove later
    static size_t __attribute__((noinline)) size_in_bits();
    static size_t num_variables();
};

/// Depending on the value of a selector variable (which must be 0 or
/// 1), choose between two G2_variable objects (zero_case and
/// one_case),
template<typename ppT>
class G2_variable_selector_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using Field = libff::Fr<ppT>;

    const pb_linear_combination<Field> selector;
    const G2_variable<ppT> zero_case;
    const G2_variable<ppT> one_case;
    G2_variable<ppT> result;
    Fqe_mul_by_lc_gadget<ppT> mul_select_X;
    Fqe_mul_by_lc_gadget<ppT> mul_select_Y;

    G2_variable_selector_gadget(
        protoboard<Field> &pb,
        const pb_linear_combination<Field> &selector,
        const G2_variable<ppT> &zero_case,
        const G2_variable<ppT> &one_case,
        const G2_variable<ppT> &result,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Gadget that creates constraints for the validity of a G2 variable.
template<typename ppT> class G2_checker_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fr<ppT> FieldT;
    typedef libff::Fqe<other_curve<ppT>> FqeT;
    typedef libff::Fqk<other_curve<ppT>> FqkT;

    G2_variable<ppT> Q;

    std::shared_ptr<Fqe_variable<ppT>> Xsquared;
    std::shared_ptr<Fqe_variable<ppT>> Ysquared;
    std::shared_ptr<Fqe_variable<ppT>> Xsquared_plus_a;
    std::shared_ptr<Fqe_variable<ppT>> Ysquared_minus_b;

    std::shared_ptr<Fqe_sqr_gadget<ppT>> compute_Xsquared;
    std::shared_ptr<Fqe_sqr_gadget<ppT>> compute_Ysquared;
    std::shared_ptr<Fqe_mul_gadget<ppT>> curve_equation;

    G2_checker_gadget(
        protoboard<FieldT> &pb,
        const G2_variable<ppT> &Q,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Gadget to add 2 G2 points
template<typename wppT> class G2_add_gadget : public gadget<libff::Fr<wppT>>
{
public:
    G2_variable<wppT> A;
    G2_variable<wppT> B;
    G2_variable<wppT> result;

    Fqe_variable<wppT> lambda;

    // For curve points A = (Ax, Ay), B = (Bx, By), we have that
    // A + B = R = (Rx, Ry) is given by:
    //
    //   Rx = lambda^2 - Ax - Bx
    //   Ry = lambda*(Ax - Rx) - Ay
    //   where lambda = (By - Ay) / (Bx - Ax)

    // lambda = (By - Ay) / (Bx - Ax)
    // <=>  lambda * (Bx - Ax) = By - Ay
    Fqe_mul_gadget<wppT> lambda_constraint;

    // Rx = lambda^2 - Ax - Bx
    // <=> lambda^2 = Rx + Ax + Bx
    Fqe_mul_gadget<wppT> Rx_constraint;

    // Ry = lambda * (Ax - Rx) - Ay
    // <=> lambda * (Ax - Rx) = Ry + Ay
    Fqe_mul_gadget<wppT> Ry_constraint;

    G2_add_gadget(
        protoboard<libff::Fr<wppT>> &pb,
        const G2_variable<wppT> &A,
        const G2_variable<wppT> &B,
        const G2_variable<wppT> &result,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Double a G2 point.
template<typename wppT> class G2_dbl_gadget : public gadget<libff::Fr<wppT>>
{
public:
    using nppT = other_curve<wppT>;

    G2_variable<wppT> A;
    G2_variable<wppT> result;

    Fqe_variable<wppT> lambda;

    // For a curve point A = (Ax, Ay), we have that A + A = B = (Bx, By) is
    // given by:
    //
    //   Bx = lambda^2 - 2 * Ax
    //   By = lambda*(Ax - Bx) - Ay
    //   where lambda = (3 * Ax^2) / 2 * Ay

    // Ax_squared = Ax * Ax
    Fqe_mul_gadget<wppT> Ax_squared_constraint;

    // lambda = (3 * Ax^2 + a) / 2 * Ay
    // <=> lambda * (Ay + Ay) = 3 * Ax_squared + a
    Fqe_mul_gadget<wppT> lambda_constraint;

    // Bx = lambda^2 - 2 * Ax
    // <=> lambda * lambda = Bx + Ax + Ax
    Fqe_mul_gadget<wppT> Bx_constraint;

    // By = lambda * (Ax - Bx) - Ay
    // <=> lambda * (Ax - Bx) = By + Ay
    Fqe_mul_gadget<wppT> By_constraint;

    G2_dbl_gadget(
        protoboard<libff::Fr<wppT>> &pb,
        const G2_variable<wppT> &A,
        const G2_variable<wppT> &result,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Check equality of 2 G2 points.
template<typename wppT>
class G2_equality_gadget : public gadget<libff::Fr<wppT>>
{
public:
    using nppT = other_curve<wppT>;
    using FqeT = libff::Fqe<nppT>;

    G2_variable<wppT> _A;
    G2_variable<wppT> _B;

    G2_equality_gadget(
        protoboard<libff::Fr<wppT>> &pb,
        const G2_variable<wppT> &A,
        const G2_variable<wppT> &B,
        const std::string &annotation_prefix);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();

private:
    // There is no generic way to iterate over the components of Fp?_variable,
    // so this method must be specialized per field extension. However, the
    // version that expects 2 components may still compile on Fp3_variable,
    // say. Hence we specify Fp2_variable explicitly in the parameters to avoid
    // callers accidentally using this for other pairings and passing in
    // Fp?_variable.
    void generate_fpe_equality_constraints(
        const Fp2_variable<FqeT> &a, const Fp2_variable<FqeT> &b);
};

/// Multiplication by constant scalar (leverages
/// point_mul_by_const_scalar_gadget - scalar_multiplication.hpp).
template<typename wppT, mp_size_t scalarLimbs>
using G2_mul_by_const_scalar_gadget = point_mul_by_const_scalar_gadget<
    libff::G2<other_curve<wppT>>,
    G2_variable<wppT>,
    G2_add_gadget<wppT>,
    G2_dbl_gadget<wppT>,
    libff::bigint<scalarLimbs>>;

template<typename wppT>
using G2_variable_or_identity =
    variable_or_identity<wppT, libff::G2<other_curve<wppT>>, G2_variable<wppT>>;

template<typename wppT>
using G2_variable_or_identity_selector_gadget = variable_or_identity_selector<
    wppT,
    libff::G2<other_curve<wppT>>,
    G2_variable<wppT>,
    G2_variable_selector_gadget<wppT>>;

template<typename wppT>
using G2_variable_and_variable_or_identity_selector_gadget =
    variable_and_variable_or_identity_selector<
        wppT,
        libff::G2<other_curve<wppT>>,
        G2_variable<wppT>,
        G2_variable_selector_gadget<wppT>>;

template<typename wppT>
using G2_add_variable_or_identity_gadget = add_variable_or_identity<
    wppT,
    libff::G2<other_curve<wppT>>,
    G2_variable<wppT>,
    G2_variable_selector_gadget<wppT>,
    G2_add_gadget<wppT>>;

template<typename wppT>
using G2_add_variable_and_variable_or_identity_gadget =
    add_variable_and_variable_or_identity<
        wppT,
        libff::G2<other_curve<wppT>>,
        G2_variable<wppT>,
        G2_variable_selector_gadget<wppT>,
        G2_add_gadget<wppT>>;

template<typename wppT>
using G2_dbl_variable_or_identity_gadget = dbl_variable_or_identity<
    wppT,
    libff::G2<other_curve<wppT>>,
    G2_variable<wppT>,
    G2_dbl_gadget<wppT>>;

template<typename wppT>
using G2_mul_by_scalar_gadget = point_mul_by_scalar_gadget<
    wppT,
    libff::G2<other_curve<wppT>>,
    G2_variable<wppT>,
    G2_variable_selector_gadget<wppT>,
    G2_add_gadget<wppT>,
    G2_dbl_gadget<wppT>>;

} // namespace libsnark

#include <libsnark/gadgetlib1/gadgets/curves/weierstrass_g2_gadget.tcc>

#endif // WEIERSTRASS_G2_GADGET_HPP_
