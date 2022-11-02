/** @file
 *****************************************************************************

 Implementation of interfaces for G1 gadgets.

 See weierstrass_g1_gadgets.hpp .

 *****************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef WEIERSTRASS_G1_GADGET_TCC_
#define WEIERSTRASS_G1_GADGET_TCC_

namespace libsnark
{

template<typename ppT>
G1_variable<ppT>::G1_variable(
    protoboard<FieldT> &pb, const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
{
    pb_variable<FieldT> X_var, Y_var;

    X_var.allocate(pb, FMT(annotation_prefix, " X"));
    Y_var.allocate(pb, FMT(annotation_prefix, " Y"));

    X = pb_linear_combination<FieldT>(X_var);
    Y = pb_linear_combination<FieldT>(Y_var);

    all_vars.emplace_back(X);
    all_vars.emplace_back(Y);
}

template<typename ppT>
G1_variable<ppT>::G1_variable(
    protoboard<FieldT> &pb,
    const libff::G1<other_curve<ppT>> &P,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
{
    libff::G1<other_curve<ppT>> Pcopy = P;
    Pcopy.to_affine_coordinates();

    X.assign(pb, Pcopy.X);
    Y.assign(pb, Pcopy.Y);
    X.evaluate(pb);
    Y.evaluate(pb);
    all_vars.emplace_back(X);
    all_vars.emplace_back(Y);
}

template<typename ppT>
G1_variable<ppT>::G1_variable(
    protoboard<FieldT> &pb,
    const pb_linear_combination<FieldT> &X,
    const pb_linear_combination<FieldT> &Y,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix), X(X), Y(Y)
{
    all_vars.emplace_back(X);
    all_vars.emplace_back(Y);
}

template<typename ppT> G1_variable<ppT> G1_variable<ppT>::operator-() const
{
    pb_linear_combination<FieldT> minus_Y;
    minus_Y.assign(this->pb, -Y);
    return G1_variable<ppT>(
        this->pb, X, minus_Y, FMT(this->annotation_prefix, " negative"));
}

template<typename ppT>
void G1_variable<ppT>::generate_r1cs_witness(
    const libff::G1<other_curve<ppT>> &el)
{
    libff::G1<other_curve<ppT>> el_normalized = el;
    el_normalized.to_affine_coordinates();

    this->pb.lc_val(X) = el_normalized.X;
    this->pb.lc_val(Y) = el_normalized.Y;
}

template<typename ppT>
libff::G1<other_curve<ppT>> G1_variable<ppT>::get_element() const
{
    using nppT = other_curve<ppT>;
    return libff::G1<nppT>(
        this->pb.lc_val(this->X),
        this->pb.lc_val(this->Y),
        libff::Fq<nppT>::one());
}

template<typename ppT> size_t G1_variable<ppT>::size_in_bits()
{
    return 2 * FieldT::size_in_bits();
}

template<typename ppT> size_t G1_variable<ppT>::num_variables() { return 2; }

template<typename ppT>
G1_variable_selector_gadget<ppT>::G1_variable_selector_gadget(
    protoboard<Field> &pb,
    const pb_linear_combination<Field> &selector,
    const G1_variable<ppT> &zero_case,
    const G1_variable<ppT> &one_case,
    const G1_variable<ppT> &result,
    const std::string &annotation_prefix)
    : gadget<Field>(pb, annotation_prefix)
    , selector(selector)
    , zero_case(zero_case)
    , one_case(one_case)
    , result(result)
{
}

template<typename ppT>
void G1_variable_selector_gadget<ppT>::generate_r1cs_constraints()
{
    // For selector \in {0, 1}, and zero_case and one_case \in G_1, we
    // require result \in G_1 to satisfy:
    //
    //      result = one_case if selector = 1, and
    //      result = zero_case if selector = 0.
    // or:
    //      result.X = selector * one_case.X + (1 - selector) * zero_case.X
    //      result.Y = selector * one_case.X + (1 - selector) * zero_case.Y
    //
    // This can be re-arranged in terms of a single constraint as
    // follows:
    //
    //      result.X = selector * one_case.X + (1 - selector) * zero_case.X
    //               = selector * (one_case.X - zero_case.X) + zero_case.X
    //  <=> result.X - zero_case.X = selector * (one_case.X - zero_case.X)
    //
    // (and similarly for result.Y)

    // selector * (one_case.X - zero_case.X) = result.X - zero_case.X
    this->pb.add_r1cs_constraint(
        r1cs_constraint<Field>(
            selector, one_case.X - zero_case.X, result.X - zero_case.X),
        FMT(this->annotation_prefix, " select X"));
    // selector * (one_case.Y - zero_case.Y) = result.Y - zero_case.Y
    this->pb.add_r1cs_constraint(
        r1cs_constraint<Field>(
            selector, one_case.Y - zero_case.Y, result.Y - zero_case.Y),
        FMT(this->annotation_prefix, " select Y"));
}

template<typename ppT>
void G1_variable_selector_gadget<ppT>::generate_r1cs_witness()
{
    protoboard<Field> &pb = this->pb;
    selector.evaluate(pb);

    zero_case.X.evaluate(pb);
    zero_case.Y.evaluate(pb);
    one_case.X.evaluate(pb);
    one_case.Y.evaluate(pb);

    if (pb.lc_val(selector) == Field::one()) {
        result.generate_r1cs_witness(one_case.get_element());
    } else {
        result.generate_r1cs_witness(zero_case.get_element());
    }
}

template<typename ppT>
G1_checker_gadget<ppT>::G1_checker_gadget(
    protoboard<FieldT> &pb,
    const G1_variable<ppT> &P,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix), P(P)
{
    P_X_squared.allocate(pb, FMT(annotation_prefix, " P_X_squared"));
    P_Y_squared.allocate(pb, FMT(annotation_prefix, " P_Y_squared"));
}

template<typename ppT> void G1_checker_gadget<ppT>::generate_r1cs_constraints()
{
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>({P.X}, {P.X}, {P_X_squared}),
        FMT(this->annotation_prefix, " P_X_squared"));
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>({P.Y}, {P.Y}, {P_Y_squared}),
        FMT(this->annotation_prefix, " P_Y_squared"));
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(
            {P.X},
            {P_X_squared, ONE * libff::G1<other_curve<ppT>>::coeff_a},
            {P_Y_squared, ONE * (-libff::G1<other_curve<ppT>>::coeff_b)}),
        FMT(this->annotation_prefix, " curve_equation"));
}

template<typename ppT> void G1_checker_gadget<ppT>::generate_r1cs_witness()
{
    this->pb.val(P_X_squared) = this->pb.lc_val(P.X).squared();
    this->pb.val(P_Y_squared) = this->pb.lc_val(P.Y).squared();
}

template<typename ppT>
G1_add_gadget<ppT>::G1_add_gadget(
    protoboard<FieldT> &pb,
    const G1_variable<ppT> &A,
    const G1_variable<ppT> &B,
    const G1_variable<ppT> &result,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix), A(A), B(B), result(result)
{
    /*
      lambda = (B.y - A.y)/(B.x - A.x)
      C.x = lambda^2 - A.x - B.x
      C.y = lambda(A.x - C.x) - A.y

      Special cases:

      doubling: if B.y = A.y and B.x = A.x then lambda is unbound and
      C = (lambda^2, lambda^3)

      addition of negative point: if B.y = -A.y and B.x = A.x then no
      lambda can satisfy the first equation unless B.y - A.y = 0. But
      then this reduces to doubling.

      So we need to check that A.x - B.x != 0, which can be done by
      enforcing I * (B.x - A.x) = 1
    */
    lambda.allocate(pb, FMT(annotation_prefix, " lambda"));
    inv.allocate(pb, FMT(annotation_prefix, " inv"));
}

template<typename ppT> void G1_add_gadget<ppT>::generate_r1cs_constraints()
{
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>({lambda}, {B.X, A.X * (-1)}, {B.Y, A.Y * (-1)}),
        FMT(this->annotation_prefix, " calc_lambda"));

    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>({lambda}, {lambda}, {result.X, A.X, B.X}),
        FMT(this->annotation_prefix, " calc_X"));

    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(
            {lambda}, {A.X, result.X * (-1)}, {result.Y, A.Y}),
        FMT(this->annotation_prefix, " calc_Y"));

    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>({inv}, {B.X, A.X * (-1)}, {ONE}),
        FMT(this->annotation_prefix, " no_special_cases"));
}

template<typename ppT> void G1_add_gadget<ppT>::generate_r1cs_witness()
{
    const libff::Fr<ppT> Ax = this->pb.lc_val(A.X);
    const libff::Fr<ppT> Bx = this->pb.lc_val(B.X);
    const libff::Fr<ppT> Ay = this->pb.lc_val(A.Y);
    const libff::Fr<ppT> By = this->pb.lc_val(B.Y);

    // Guard against the inverse operation failing.
    if (Ax == Bx) {
        throw std::runtime_error(
            "A.X == B.X is not supported by G1_add_gadget");
    }

    this->pb.val(inv) = (Bx - Ax).inverse();
    this->pb.val(lambda) = (By - Ay) * this->pb.val(inv);
    this->pb.lc_val(result.X) = this->pb.val(lambda).squared() - Ax - Bx;
    this->pb.lc_val(result.Y) =
        this->pb.val(lambda) * (Ax - this->pb.lc_val(result.X)) - Ay;
}

template<typename ppT>
G1_dbl_gadget<ppT>::G1_dbl_gadget(
    protoboard<FieldT> &pb,
    const G1_variable<ppT> &A,
    const G1_variable<ppT> &result,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix), A(A), result(result)
{
    Xsquared.allocate(pb, FMT(annotation_prefix, " X_squared"));
    lambda.allocate(pb, FMT(annotation_prefix, " lambda"));
}

template<typename ppT> void G1_dbl_gadget<ppT>::generate_r1cs_constraints()
{
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>({A.X}, {A.X}, {Xsquared}),
        FMT(this->annotation_prefix, " calc_Xsquared"));

    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(
            {lambda * 2},
            {A.Y},
            {Xsquared * 3, ONE * libff::G1<other_curve<ppT>>::coeff_a}),
        FMT(this->annotation_prefix, " calc_lambda"));

    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>({lambda}, {lambda}, {result.X, A.X * 2}),
        FMT(this->annotation_prefix, " calc_X"));

    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(
            {lambda}, {A.X, result.X * (-1)}, {result.Y, A.Y}),
        FMT(this->annotation_prefix, " calc_Y"));
}

template<typename ppT> void G1_dbl_gadget<ppT>::generate_r1cs_witness()
{
    this->pb.val(Xsquared) = this->pb.lc_val(A.X).squared();
    this->pb.val(lambda) = (FieldT(3) * this->pb.val(Xsquared) +
                            libff::G1<other_curve<ppT>>::coeff_a) *
                           (FieldT(2) * this->pb.lc_val(A.Y)).inverse();
    this->pb.lc_val(result.X) =
        this->pb.val(lambda).squared() - FieldT(2) * this->pb.lc_val(A.X);
    this->pb.lc_val(result.Y) =
        this->pb.val(lambda) *
            (this->pb.lc_val(A.X) - this->pb.lc_val(result.X)) -
        this->pb.lc_val(A.Y);
}

template<typename ppT>
G1_multiscalar_mul_gadget<ppT>::G1_multiscalar_mul_gadget(
    protoboard<FieldT> &pb,
    const G1_variable<ppT> &base,
    const pb_variable_array<FieldT> &scalars,
    const size_t elt_size,
    const std::vector<G1_variable<ppT>> &points,
    const G1_variable<ppT> &result,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , base(base)
    , scalars(scalars)
    , points(points)
    , result(result)
    , elt_size(elt_size)
    , num_points(points.size())
    , scalar_size(scalars.size())
{
    assert(num_points >= 1);
    assert(num_points * elt_size == scalar_size);

    for (size_t i = 0; i < num_points; ++i) {
        points_and_powers.emplace_back(points[i]);
        for (size_t j = 0; j < elt_size - 1; ++j) {
            points_and_powers.emplace_back(G1_variable<ppT>(
                pb,
                FMT(annotation_prefix,
                    " points_%zu_times_2_to_%zu",
                    i,
                    j + 1)));
            doublers.emplace_back(G1_dbl_gadget<ppT>(
                pb,
                points_and_powers[i * elt_size + j],
                points_and_powers[i * elt_size + j + 1],
                FMT(annotation_prefix, " double_%zu_to_2_to_%zu", i, j + 1)));
        }
    }

    chosen_results.emplace_back(base);
    for (size_t i = 0; i < scalar_size; ++i) {
        computed_results.emplace_back(G1_variable<ppT>(
            pb, FMT(annotation_prefix, " computed_results_%zu")));
        if (i < scalar_size - 1) {
            chosen_results.emplace_back(G1_variable<ppT>(
                pb, FMT(annotation_prefix, " chosen_results_%zu")));
        } else {
            chosen_results.emplace_back(result);
        }

        adders.emplace_back(G1_add_gadget<ppT>(
            pb,
            chosen_results[i],
            points_and_powers[i],
            computed_results[i],
            FMT(annotation_prefix, " adders_%zu")));
    }
}

template<typename ppT>
void G1_multiscalar_mul_gadget<ppT>::generate_r1cs_constraints()
{
    const size_t num_constraints_before = this->pb.num_constraints();

    for (size_t i = 0; i < scalar_size - num_points; ++i) {
        doublers[i].generate_r1cs_constraints();
    }

    for (size_t i = 0; i < scalar_size; ++i) {
        adders[i].generate_r1cs_constraints();

        /*
          chosen_results[i+1].X = scalars[i] * computed_results[i].X +
          (1-scalars[i]) *  chosen_results[i].X chosen_results[i+1].X -
          chosen_results[i].X = scalars[i] * (computed_results[i].X -
          chosen_results[i].X)
        */
        this->pb.add_r1cs_constraint(
            r1cs_constraint<FieldT>(
                scalars[i],
                computed_results[i].X - chosen_results[i].X,
                chosen_results[i + 1].X - chosen_results[i].X),
            FMT(this->annotation_prefix, " chosen_results_%zu_X", i + 1));
        this->pb.add_r1cs_constraint(
            r1cs_constraint<FieldT>(
                scalars[i],
                computed_results[i].Y - chosen_results[i].Y,
                chosen_results[i + 1].Y - chosen_results[i].Y),
            FMT(this->annotation_prefix, " chosen_results_%zu_Y", i + 1));
    }

    const size_t num_constraints_after = this->pb.num_constraints();
    assert(
        num_constraints_after - num_constraints_before ==
        4 * (scalar_size - num_points) + (4 + 2) * scalar_size);
    libff::UNUSED(num_constraints_before);
    libff::UNUSED(num_constraints_after);
}

template<typename ppT>
void G1_multiscalar_mul_gadget<ppT>::generate_r1cs_witness()
{
    for (size_t i = 0; i < scalar_size - num_points; ++i) {
        doublers[i].generate_r1cs_witness();
    }

    for (size_t i = 0; i < scalar_size; ++i) {
        adders[i].generate_r1cs_witness();
        this->pb.lc_val(chosen_results[i + 1].X) =
            (this->pb.val(scalars[i]) == libff::Fr<ppT>::zero()
                 ? this->pb.lc_val(chosen_results[i].X)
                 : this->pb.lc_val(computed_results[i].X));
        this->pb.lc_val(chosen_results[i + 1].Y) =
            (this->pb.val(scalars[i]) == libff::Fr<ppT>::zero()
                 ? this->pb.lc_val(chosen_results[i].Y)
                 : this->pb.lc_val(computed_results[i].Y));
    }
}

} // namespace libsnark

#endif // WEIERSTRASS_G1_GADGET_TCC_
