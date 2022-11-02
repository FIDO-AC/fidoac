/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef __LIBFF_ALGEBRA_FIELDS_FIELD_SERIALIZATION_HPP__
#define __LIBFF_ALGEBRA_FIELDS_FIELD_SERIALIZATION_HPP__

#include "libff/algebra/fields/bigint.hpp"
#include "libff/algebra/serialization.hpp"

namespace libff
{

template<typename BigIntT>
void bigint_from_hex(BigIntT &v, const std::string &hex);

template<typename BigIntT>
std::string bigint_to_hex(const BigIntT &v, bool prefix = false);

template<
    encoding_t Enc = encoding_binary,
    form_t Form = form_plain,
    typename FieldT>
void field_read(FieldT &v, std::istream &in_s);

template<
    encoding_t Enc = encoding_binary,
    form_t Form = form_plain,
    typename FieldT>
void field_write(const FieldT &v, std::ostream &out_s);

} // namespace libff

#include "libff/algebra/fields/field_serialization.tcc"

#endif // __LIBFF_ALGEBRA_FIELDS_FIELD_SERIALIZATION_HPP__
