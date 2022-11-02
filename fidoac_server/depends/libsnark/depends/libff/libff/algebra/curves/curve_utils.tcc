/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef CURVE_UTILS_TCC_
#define CURVE_UTILS_TCC_

namespace libff
{

template<typename GroupT, mp_size_t m>
GroupT scalar_mul(const GroupT &base, const bigint<m> &scalar)
{
    GroupT result = GroupT::zero();

    bool found_one = false;
    for (long i = static_cast<long>(scalar.max_bits() - 1); i >= 0; --i) {
        if (found_one) {
            result = result.dbl();
        }

        if (scalar.test_bit(i)) {
            found_one = true;
            result = result + base;
        }
    }

    return result;
}

template<typename GroupT>
decltype(((GroupT *)nullptr)->X) curve_point_y_at_x(
    const decltype(((GroupT *)nullptr)->X) &x)
{
    using base_field = decltype(((GroupT *)nullptr)->X);
    const base_field x_squared = x * x;
    const base_field x_cubed = x_squared * x;
    const base_field y_squared =
        x_cubed + (GroupT::coeff_a * x) + GroupT::coeff_b;
    return y_squared.sqrt();
}

template<typename GroupT>
GroupT g1_curve_point_at_x(const typename GroupT::base_field &x)
{
    const typename GroupT::base_field x_squared = x * x;
    const typename GroupT::base_field x_cubed = x_squared * x;
    const typename GroupT::base_field y_squared =
        x_cubed + (GroupT::coeff_a * x) + GroupT::coeff_b;
    // Check that y_squared is a quadratic residue (ensuring that sqrt()
    // terminates).
    if ((y_squared ^ GroupT::base_field::euler) != GroupT::base_field::one()) {
        throw std::runtime_error("curve eqn has no solution at x");
    }

    const typename GroupT::base_field y = y_squared.sqrt();
    return GroupT(x, y, GroupT::base_field::one());
}

template<typename GroupT>
GroupT g2_curve_point_at_x(const typename GroupT::twist_field &x)
{
    // TODO: Generic check (over all fields) that y_squared.sqrt() terminates.
    return GroupT(x, curve_point_y_at_x<GroupT>(x), GroupT::twist_field::one());
}

} // namespace libff
#endif // CURVE_UTILS_TCC_
