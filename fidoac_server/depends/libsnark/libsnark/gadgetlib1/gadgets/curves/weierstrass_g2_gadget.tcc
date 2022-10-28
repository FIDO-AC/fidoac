/** @file
 *****************************************************************************

 Implementation of interfaces for G2 gadgets.

 See weierstrass_g2_gadgets.hpp .

 *****************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef WEIERSTRASS_G2_GADGET_TCC_
#define WEIERSTRASS_G2_GADGET_TCC_

#include <libff/algebra/scalar_multiplication/wnaf.hpp>

namespace libsnark
{

template<typename ppT>
G2_variable<ppT>::G2_variable(
    protoboard<FieldT> &pb, const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
{
    X.reset(new Fqe_variable<ppT>(pb, FMT(annotation_prefix, " X")));
    Y.reset(new Fqe_variable<ppT>(pb, FMT(annotation_prefix, " Y")));

    all_vars.insert(all_vars.end(), X->all_vars.begin(), X->all_vars.end());
    all_vars.insert(all_vars.end(), Y->all_vars.begin(), Y->all_vars.end());
}

template<typename ppT>
G2_variable<ppT>::G2_variable(
    protoboard<FieldT> &pb,
    const libff::G2<other_curve<ppT>> &Q,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
{
    libff::G2<other_curve<ppT>> Q_copy = Q;
    Q_copy.to_affine_coordinates();

    X.reset(new Fqe_variable<ppT>(pb, Q_copy.X, FMT(annotation_prefix, " X")));
    Y.reset(new Fqe_variable<ppT>(pb, Q_copy.Y, FMT(annotation_prefix, " Y")));

    all_vars.insert(all_vars.end(), X->all_vars.begin(), X->all_vars.end());
    all_vars.insert(all_vars.end(), Y->all_vars.begin(), Y->all_vars.end());
}

template<typename ppT>
G2_variable<ppT>::G2_variable(
    protoboard<FieldT> &pb,
    const Fqe_variable<ppT> &X_coord,
    const Fqe_variable<ppT> &Y_coord,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , X(new Fqe_variable<ppT>(X_coord))
    , Y(new Fqe_variable<ppT>(Y_coord))
{
    all_vars.insert(all_vars.end(), X->all_vars.begin(), X->all_vars.end());
    all_vars.insert(all_vars.end(), Y->all_vars.begin(), Y->all_vars.end());
}

template<typename ppT> G2_variable<ppT> G2_variable<ppT>::operator-() const
{
    return G2_variable<ppT>(
        this->pb, *X, -(*Y), FMT(this->annotation_prefix, " negative"));
}

template<typename ppT>
void G2_variable<ppT>::generate_r1cs_witness(
    const libff::G2<other_curve<ppT>> &Q)
{
    libff::G2<other_curve<ppT>> Qcopy = Q;
    Qcopy.to_affine_coordinates();

    X->generate_r1cs_witness(Qcopy.X);
    Y->generate_r1cs_witness(Qcopy.Y);
}

template<typename ppT>
libff::G2<other_curve<ppT>> G2_variable<ppT>::get_element() const
{
    using nppT = other_curve<ppT>;
    return libff::G2<nppT>(
        X->get_element(),
        Y->get_element(),
        libff::G2<nppT>::twist_field::one());
}

template<typename ppT> size_t G2_variable<ppT>::size_in_bits()
{
    return 2 * Fqe_variable<ppT>::size_in_bits();
}

template<typename ppT> size_t G2_variable<ppT>::num_variables()
{
    return 2 * Fqe_variable<ppT>::num_variables();
}

template<typename ppT>
G2_variable_selector_gadget<ppT>::G2_variable_selector_gadget(
    protoboard<Field> &pb,
    const pb_linear_combination<Field> &selector,
    const G2_variable<ppT> &zero_case,
    const G2_variable<ppT> &one_case,
    const G2_variable<ppT> &result,
    const std::string &annotation_prefix)
    : gadget<Field>(pb, annotation_prefix)
    , selector(selector)
    , zero_case(zero_case)
    , one_case(one_case)
    , result(result)
    , mul_select_X(
          pb,
          *one_case.X - *zero_case.X,
          selector,
          *result.X - *zero_case.X,
          FMT(annotation_prefix, " mul_select_X"))
    , mul_select_Y(
          pb,
          *one_case.Y - *zero_case.Y,
          selector,
          *result.Y - *zero_case.Y,
          FMT(annotation_prefix, " mul_select_Y"))
{
}

template<typename ppT>
void G2_variable_selector_gadget<ppT>::generate_r1cs_constraints()
{
    mul_select_X.generate_r1cs_constraints();
    mul_select_Y.generate_r1cs_constraints();
}

template<typename ppT>
void G2_variable_selector_gadget<ppT>::generate_r1cs_witness()
{
    protoboard<Field> &pb = this->pb;
    selector.evaluate(pb);

    zero_case.X->evaluate();
    zero_case.Y->evaluate();
    one_case.X->evaluate();
    one_case.Y->evaluate();
    mul_select_X.generate_r1cs_witness();
    mul_select_Y.generate_r1cs_witness();

    if (pb.lc_val(selector) == Field::one()) {
        result.generate_r1cs_witness(one_case.get_element());
    } else {
        result.generate_r1cs_witness(zero_case.get_element());
    }
}

template<typename ppT>
G2_checker_gadget<ppT>::G2_checker_gadget(
    protoboard<FieldT> &pb,
    const G2_variable<ppT> &Q,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix), Q(Q)
{
    Xsquared.reset(
        new Fqe_variable<ppT>(pb, FMT(annotation_prefix, " Xsquared")));
    Ysquared.reset(
        new Fqe_variable<ppT>(pb, FMT(annotation_prefix, " Ysquared")));

    compute_Xsquared.reset(new Fqe_sqr_gadget<ppT>(
        pb, *(Q.X), *Xsquared, FMT(annotation_prefix, " compute_Xsquared")));
    compute_Ysquared.reset(new Fqe_sqr_gadget<ppT>(
        pb, *(Q.Y), *Ysquared, FMT(annotation_prefix, " compute_Ysquared")));

    Xsquared_plus_a.reset(new Fqe_variable<ppT>(
        (*Xsquared) + libff::G2<other_curve<ppT>>::coeff_a));
    Ysquared_minus_b.reset(new Fqe_variable<ppT>(
        (*Ysquared) + (-libff::G2<other_curve<ppT>>::coeff_b)));

    curve_equation.reset(new Fqe_mul_gadget<ppT>(
        pb,
        *(Q.X),
        *Xsquared_plus_a,
        *Ysquared_minus_b,
        FMT(annotation_prefix, " curve_equation")));
}

template<typename ppT> void G2_checker_gadget<ppT>::generate_r1cs_constraints()
{
    compute_Xsquared->generate_r1cs_constraints();
    compute_Ysquared->generate_r1cs_constraints();
    curve_equation->generate_r1cs_constraints();
}

template<typename ppT> void G2_checker_gadget<ppT>::generate_r1cs_witness()
{
    compute_Xsquared->generate_r1cs_witness();
    compute_Ysquared->generate_r1cs_witness();
    Xsquared_plus_a->evaluate();
    curve_equation->generate_r1cs_witness();
}

template<typename wppT>
G2_add_gadget<wppT>::G2_add_gadget(
    protoboard<libff::Fr<wppT>> &pb,
    const G2_variable<wppT> &A,
    const G2_variable<wppT> &B,
    const G2_variable<wppT> &result,
    const std::string &annotation_prefix)
    : gadget<libff::Fr<wppT>>(pb, annotation_prefix)
    , A(A)
    , B(B)
    , result(result)
    , lambda(pb, FMT(annotation_prefix, " lambda"))
    // lambda = (By - Ay) / (Bx - Ax)
    // <=>  lambda * (Bx - Ax) = By - Ay
    , lambda_constraint(
          pb,
          lambda,
          *B.X - *A.X,
          *B.Y - *A.Y,
          FMT(annotation_prefix, " lambda_constraint"))
    // Rx = lambda^2 - Ax - Bx
    // <=> lambda^2 = Rx + Ax + Bx
    , Rx_constraint(
          pb,
          lambda,
          lambda,
          *result.X + *A.X + *B.X,
          FMT(annotation_prefix, " Rx_constraint"))
    // Ry = lambda * (Ax - Rx) - Ay
    // <=> lambda * (Ax - Rx) = Ry + Ay
    , Ry_constraint(
          pb,
          lambda,
          (*A.X - *result.X),
          *result.Y + *A.Y,
          FMT(annotation_prefix, " Ry_constraint"))
{
}

template<typename wppT> void G2_add_gadget<wppT>::generate_r1cs_constraints()
{
    lambda_constraint.generate_r1cs_constraints();
    Rx_constraint.generate_r1cs_constraints();
    Ry_constraint.generate_r1cs_constraints();
}

template<typename wppT> void G2_add_gadget<wppT>::generate_r1cs_witness()
{
    using nppT = other_curve<wppT>;
    const libff::Fqe<nppT> Ax = A.X->get_element();
    const libff::Fqe<nppT> Ay = A.Y->get_element();
    const libff::Fqe<nppT> Bx = B.X->get_element();
    const libff::Fqe<nppT> By = B.Y->get_element();

    // Guard against the inverse operation failing.
    if (Ax == Bx) {
        throw std::runtime_error(
            "A.X == B.X is not supported by G2_add_gadget");
    }

    // lambda = (By - Ay) / (Bx - Ax)
    const libff::Fqe<nppT> lambda_value = (By - Ay) * (Bx - Ax).inverse();
    lambda.generate_r1cs_witness(lambda_value);
    lambda_constraint.B.evaluate();
    lambda_constraint.result.evaluate();
    lambda_constraint.generate_r1cs_witness();

    // Rx = lambda^2 - Ax - Bx
    // Ry = lambda * (Ax - Rx) - Ay
    const libff::Fqe<nppT> Rx = lambda_value.squared() - Ax - Bx;
    const libff::Fqe<nppT> Ry = lambda_value * (Ax - Rx) - Ay;
    result.generate_r1cs_witness(
        libff::G2<nppT>(Rx, Ry, libff::Fqe<nppT>::one()));

    // lambda^2 = Rx + Ax + Bx
    Rx_constraint.result.evaluate();
    Rx_constraint.generate_r1cs_witness();

    // lambda * (Ax - Rx) = Ry + Ay
    Ry_constraint.B.evaluate();
    Ry_constraint.result.evaluate();
    Ry_constraint.generate_r1cs_witness();
}

template<typename wppT>
G2_dbl_gadget<wppT>::G2_dbl_gadget(
    protoboard<libff::Fr<wppT>> &pb,
    const G2_variable<wppT> &A,
    const G2_variable<wppT> &result,
    const std::string &annotation_prefix)
    : gadget<libff::Fr<wppT>>(pb, annotation_prefix)
    , A(A)
    , result(result)
    , lambda(pb, FMT(annotation_prefix, " lambda"))
    // Ax_squared = Ax * Ax
    , Ax_squared_constraint(
          pb,
          *A.X,
          *A.X,
          Fqe_variable<wppT>(pb, FMT(annotation_prefix, " Ax^2")),
          FMT(annotation_prefix, " _Ax_squared_constraint"))
    // lambda = (3 * Ax^2 + a) / 2 * Ay
    // <=> lambda * (Ay + Ay) = 3 * Ax_squared + a
    , lambda_constraint(
          pb,
          lambda,
          *A.Y + *A.Y,
          Ax_squared_constraint.result * libff::Fr<wppT>(3) +
              libff::G2<nppT>::coeff_a,
          FMT(annotation_prefix, " lambda_constraint"))
    // Bx = lambda^2 - 2 * Ax
    // <=> lambda * lambda = Bx + Ax + Ax
    , Bx_constraint(
          pb,
          lambda,
          lambda,
          *result.X + *A.X + *A.X,
          FMT(annotation_prefix, " Bx_constraint"))
    // By = lambda * (Ax - Bx) - Ay
    // <=> lambda * (Ax - Bx) = By + Ay
    , By_constraint(
          pb,
          lambda,
          (*A.X - *result.X),
          *result.Y + *A.Y,
          FMT(annotation_prefix, " By_constraint"))
{
}

template<typename wppT> void G2_dbl_gadget<wppT>::generate_r1cs_constraints()
{
    Ax_squared_constraint.generate_r1cs_constraints();
    lambda_constraint.generate_r1cs_constraints();
    Bx_constraint.generate_r1cs_constraints();
    By_constraint.generate_r1cs_constraints();
}

template<typename wppT> void G2_dbl_gadget<wppT>::generate_r1cs_witness()
{
    const libff::Fqe<nppT> Ax = A.X->get_element();
    const libff::Fqe<nppT> Ay = A.Y->get_element();

    // Ax_squared = Ax * Ax
    Ax_squared_constraint.generate_r1cs_witness();
    Ax_squared_constraint.result.evaluate();
    const libff::Fqe<nppT> Ax_squared =
        Ax_squared_constraint.result.get_element();

    // lambda = (3 * Ax^2 + a) / 2 * Ay
    // <=> lambda * (Ay + Ay) = 3 * Ax_squared + a
    const libff::Fqe<nppT> Ax_squared_plus_a =
        Ax_squared + Ax_squared + Ax_squared + libff::G2<nppT>::coeff_a;
    const libff::Fqe<nppT> lambda_value =
        Ax_squared_plus_a * (Ay + Ay).inverse();
    lambda.generate_r1cs_witness(lambda_value);
    lambda_constraint.B.evaluate();
    lambda_constraint.generate_r1cs_witness();

    // Bx = lambda^2 - 2 * Ax
    // By = lambda * (Ax - Bx) - Ay
    const libff::Fqe<nppT> Bx = lambda_value.squared() - Ax - Ax;
    const libff::Fqe<nppT> By = lambda_value * (Ax - Bx) - Ay;
    result.generate_r1cs_witness(
        libff::G2<nppT>(Bx, By, libff::Fqe<nppT>::one()));

    // lambda * lambda = Bx + Ax + Ax
    Bx_constraint.generate_r1cs_witness();

    // lambda * (Ax - Bx) = By + Ay
    By_constraint.B.evaluate();
    By_constraint.generate_r1cs_witness();
}

template<typename wppT>
G2_equality_gadget<wppT>::G2_equality_gadget(
    protoboard<libff::Fr<wppT>> &pb,
    const G2_variable<wppT> &A,
    const G2_variable<wppT> &B,
    const std::string &annotation_prefix)
    : gadget<libff::Fr<wppT>>(pb, annotation_prefix), _A(A), _B(B)
{
}

template<typename wppT>
void G2_equality_gadget<wppT>::generate_r1cs_constraints()
{
    // A.X == B.X
    generate_fpe_equality_constraints(*_A.X, *_B.X);
    // A.Y == B.Y
    generate_fpe_equality_constraints(*_A.X, *_B.X);
}

template<typename wppT> void G2_equality_gadget<wppT>::generate_r1cs_witness()
{
    // Nothing to do
}

template<typename wppT>
void G2_equality_gadget<wppT>::generate_fpe_equality_constraints(
    const Fp2_variable<libff::Fqe<other_curve<wppT>>> &a,
    const Fp2_variable<libff::Fqe<other_curve<wppT>>> &b)
{
    this->pb.add_r1cs_constraint(
        r1cs_constraint<libff::Fr<wppT>>(a.c0, 1, b.c0),
        FMT(this->annotation_prefix, " c0"));
    this->pb.add_r1cs_constraint(
        r1cs_constraint<libff::Fr<wppT>>(a.c1, 1, b.c1),
        FMT(this->annotation_prefix, " c1"));
}

} // namespace libsnark

#endif // WEIERSTRASS_G2_GADGET_TCC_
