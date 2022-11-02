/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef __LIBSNARK_COMMON_DATA_STRUCTURES_POLYNOMIAL_HPP__
#define __LIBSNARK_COMMON_DATA_STRUCTURES_POLYNOMIAL_HPP__

namespace libsnark
{

/// Polynomial type, as a list of coefficients (currently only represents
/// univariate polynomials). Value at index i is the coefficient of x^i.
template<typename FieldT> using polynomial = std::vector<FieldT>;

} // namespace libsnark

#endif // __LIBSNARK_COMMON_DATA_STRUCTURES_POLYNOMIAL_HPP__
