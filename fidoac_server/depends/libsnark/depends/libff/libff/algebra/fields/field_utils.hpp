/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef FIELD_UTILS_HPP_
#define FIELD_UTILS_HPP_

#include "libff/algebra/fields/bigint.hpp"
#include "libff/algebra/fields/fp.hpp"
#include "libff/common/double.hpp"
#include "libff/common/utils.hpp"

#include <cstdint>

namespace libff
{

// returns true if get_root_of_unity will succeed.
template<typename FieldT>
typename std::enable_if<std::is_same<FieldT, Double>::value, bool>::type
has_root_of_unity(const size_t n);

template<typename FieldT>
typename std::enable_if<!std::is_same<FieldT, Double>::value, bool>::type
has_root_of_unity(const size_t n);

// returns root of unity of order n (for n a power of 2), if one exists
template<typename FieldT>
typename std::enable_if<std::is_same<FieldT, Double>::value, FieldT>::type
get_root_of_unity(const size_t n);

template<typename FieldT>
typename std::enable_if<!std::is_same<FieldT, Double>::value, FieldT>::type
get_root_of_unity(const size_t n);

/// Decompose v into fixed-size digits of digit_size bits each.
template<mp_size_t n>
size_t field_get_digit(
    const bigint<n> &v, const size_t digit_size, const size_t digit_index);

/// Decompose v into fixed-size signed digits of digit_size bits each.
template<mp_size_t n>
ssize_t field_get_signed_digit(
    const bigint<n> &v, const size_t digit_size, const size_t digit_index);

/// Decompose the input into a (pre-allocated) vector of fixed-size signed
/// digits, of size digit_size bits.
template<typename FieldT>
void field_get_signed_digits(
    std::vector<ssize_t> &digits,
    const FieldT &v,
    const size_t digit_size,
    const size_t num_digits);

template<typename FieldT>
std::vector<FieldT> pack_int_vector_into_field_element_vector(
    const std::vector<size_t> &v, const size_t w);

template<typename FieldT>
std::vector<FieldT> pack_bit_vector_into_field_element_vector(
    const bit_vector &v, const size_t chunk_bits);

template<typename FieldT>
std::vector<FieldT> pack_bit_vector_into_field_element_vector(
    const bit_vector &v);

template<typename FieldT>
std::vector<FieldT> convert_bit_vector_to_field_element_vector(
    const bit_vector &v);

template<typename FieldT>
bit_vector convert_field_element_vector_to_bit_vector(
    const std::vector<FieldT> &v);

template<typename FieldT>
bit_vector convert_field_element_to_bit_vector(const FieldT &el);

template<typename FieldT>
bit_vector convert_field_element_to_bit_vector(
    const FieldT &el, const size_t bitcount);

template<typename FieldT>
FieldT convert_bit_vector_to_field_element(const bit_vector &v);

template<typename FieldT> void batch_invert(std::vector<FieldT> &vec);

/// Rerturns a reference to the 0-th component of the element (or the element
/// itself if FieldT is not an extension field).
template<typename FieldT>
const typename FieldT::my_Fp &field_get_component_0(const FieldT &v);

/// Safe conversion from one finite field type to another. Internally asserts
/// that the transformation is injective (i.e. multiple input values cannot
/// result in the same output value).
template<
    mp_size_t wn,
    const bigint<wn> &wmodulus,
    mp_size_t nn,
    const bigint<nn> &nmodulus>
void fp_from_fp(Fp_model<wn, wmodulus> &wfp, const Fp_model<nn, nmodulus> &nfp);

} // namespace libff

#include <libff/algebra/fields/field_utils.tcc>

#endif // FIELD_UTILS_HPP_
