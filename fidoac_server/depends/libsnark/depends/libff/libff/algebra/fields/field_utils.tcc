/** @file
 *****************************************************************************
 Implementation of misc. math and serialization utility functions
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef FIELD_UTILS_TCC_
#define FIELD_UTILS_TCC_

#include <complex>
#include <libff/algebra/fields/fp.hpp>
#include <libff/common/double.hpp>
#include <libff/common/utils.hpp>
#include <stdexcept>

namespace libff
{

namespace internal
{

template<typename FieldT> class component_0_getter
{
public:
    static const typename FieldT::my_Fp &get_component_0(const FieldT &field_el)
    {
        using NextSubfield =
            typename std::decay<decltype(field_el.coeffs[0])>::type;
        return component_0_getter<NextSubfield>::get_component_0(
            field_el.coeffs[0]);
    }
};

template<mp_size_t n, const bigint<n> &modulus>
class component_0_getter<Fp_model<n, modulus>>
{
public:
    static const Fp_model<n, modulus> &get_component_0(
        const Fp_model<n, modulus> &field_el)
    {
        return field_el;
    }
};

} // namespace internal

template<mp_size_t n>
size_t field_get_digit(
    const bigint<n> &v, const size_t digit_size, const size_t digit_idx)
{
    constexpr size_t limb_size_bits = 8 * sizeof(v.data[0]);
    const size_t start_bit = digit_size * digit_idx;
    const size_t end_bit = start_bit + digit_size;
    const size_t low_limb = start_bit / limb_size_bits;
    const size_t high_limb = end_bit / limb_size_bits;

    // use_high_limb = (high_limb < n) && (high_limb != low_limb)
    //               = (1 - (n - 1 - high_limb) >> limb_size_bits) &&
    //                   (high_limb - low_limb)
    const size_t use_high_limb =
        (1 - ((n - 1 - high_limb) >> (limb_size_bits - 1))) &
        (high_limb - low_limb);
    assert(use_high_limb == ((high_limb < n) && (high_limb != low_limb)));
    const size_t low_limb_shift = start_bit - low_limb * limb_size_bits;

    // if use_high_limb == 0, then high_limb_shift = c, which causes any high
    // limb data to be shifted outside of the mask.
    const size_t high_limb_bits =
        use_high_limb * (end_bit - high_limb * limb_size_bits);
    const size_t high_limb_shift = digit_size - high_limb_bits;

    const size_t mask = (1ull << digit_size) - 1;

    return mask & ((v.data[use_high_limb * high_limb] << high_limb_shift) |
                   (v.data[low_limb] >> low_limb_shift));
}

template<mp_size_t n>
ssize_t field_get_signed_digit(
    const bigint<n> &v, const size_t digit_size, const size_t digit_index)
{
    //  digit            x x x x
    //  carry_mask       1 0 0 0
    //  overflow_mask  1 0 0 0 0
    const size_t carry_mask = 1ull << (digit_size - 1);
    const ssize_t overflow_mask = 1ll << digit_size;
    size_t carry = 0;
    ssize_t overflow = 0;
    size_t digit;
    size_t i = 0;

    // For each digit:
    //   if overflow, then digit = 0, carry = 1
    //   if carry, digit = digit - overflow_mask,
    //     (because overflow_mask is also the value added when carrying into
    //      in the next digit)
    //   else digit = digit

    do {
        // This is done at the start of the loop, rather than the end, since
        // the individual values (before this OR) are required to compute the
        // final value when this loop terminates.
        carry = overflow | carry;

        digit = field_get_digit(v, digit_size, i) + carry;
        overflow = (digit & overflow_mask) >> digit_size; // 1/0
        carry = (digit & carry_mask) >> (digit_size - 1); // 1/0
        ++i;
    } while (i <= digit_index);

    return (1 - overflow) * (digit - (carry * overflow_mask));
}

template<typename FieldT>
void field_get_signed_digits(
    std::vector<ssize_t> &digits,
    const FieldT &v,
    const size_t digit_size,
    const size_t num_digits)
{
    assert(digits.size() >= num_digits);

    // Code matches fixed_wnaf_digit(), but stores each digit.

    const size_t carry_mask = 1ull << (digit_size - 1);
    const ssize_t overflow_mask = 1ll << digit_size;
    const auto v_bi = v.as_bigint();

    size_t carry = 0;
    ssize_t overflow = 0;

    for (size_t digit_idx = 0; digit_idx < num_digits; ++digit_idx) {
        //   if overflow, then digit = 0, carry = 1
        //   if carry, digit = digit - overflow_mask,
        //     (because overflow_mask is also the value added when carrying into
        //      in the next digit)
        //   else digit = digit

        carry = overflow | carry;
        const ssize_t digit =
            field_get_digit(v_bi, digit_size, digit_idx) + carry;
        overflow = (digit & overflow_mask) >> digit_size; // 1/0
        carry = (digit & carry_mask) >> (digit_size - 1); // 1/0

        digits[digit_idx] = (1 - overflow) * (digit - (carry * overflow_mask));
    }
}

template<typename FieldT> FieldT coset_shift()
{
    return FieldT::multiplicative_generator.squared();
}

template<typename FieldT>
typename std::enable_if<std::is_same<FieldT, Double>::value, bool>::type
has_root_of_unity(const size_t /*n*/)
{
    return true;
}

template<typename FieldT>
typename std::enable_if<!std::is_same<FieldT, Double>::value, bool>::type
has_root_of_unity(const size_t n)
{
    const size_t logn = libff::log2(n);

    if (n != (1u << logn)) {
        return false;
    }

    if (logn > FieldT::s) {
        return false;
    }

    return true;
}

template<typename FieldT>
typename std::enable_if<std::is_same<FieldT, Double>::value, FieldT>::type
get_root_of_unity(const size_t n)
{
    const double PI = 3.141592653589793238460264338328L;

    return FieldT(cos(2 * PI / n), sin(2 * PI / n));
}

template<typename FieldT>
typename std::enable_if<!std::is_same<FieldT, Double>::value, FieldT>::type
get_root_of_unity(const size_t n)
{
    if (!has_root_of_unity<FieldT>(n)) {
        throw std::invalid_argument("libff::get_root_of_unity: expected n == "
                                    "(1u << logn) && logn <= FieldT::s");
    }

    const size_t logn = log2(n);
    FieldT omega = FieldT::root_of_unity;
    for (size_t i = FieldT::s; i > logn; --i) {
        omega *= omega;
    }

    return omega;
}

template<typename FieldT>
std::vector<FieldT> pack_int_vector_into_field_element_vector(
    const std::vector<size_t> &v, const size_t w)
{
    const size_t chunk_bits = FieldT::capacity();
    const size_t repacked_size = div_ceil(v.size() * w, chunk_bits);
    std::vector<FieldT> result(repacked_size);

    for (size_t i = 0; i < repacked_size; ++i) {
        bigint<FieldT::num_limbs> b;
        for (size_t j = 0; j < chunk_bits; ++j) {
            const size_t word_index = (i * chunk_bits + j) / w;
            const size_t pos_in_word = (i * chunk_bits + j) % w;
            const size_t word_or_0 =
                (word_index < v.size() ? v[word_index] : 0);
            const size_t bit = (word_or_0 >> pos_in_word) & 1;

            b.data[j / GMP_NUMB_BITS] |= bit << (j % GMP_NUMB_BITS);
        }
        result[i] = FieldT(b);
    }

    return result;
}

template<typename FieldT>
std::vector<FieldT> pack_bit_vector_into_field_element_vector(
    const bit_vector &v, const size_t chunk_bits)
{
    assert(chunk_bits <= FieldT::capacity());

    const size_t repacked_size = div_ceil(v.size(), chunk_bits);
    std::vector<FieldT> result(repacked_size);

    for (size_t i = 0; i < repacked_size; ++i) {
        bigint<FieldT::num_limbs> b;
        for (size_t j = 0; j < chunk_bits; ++j) {
            b.data[j / GMP_NUMB_BITS] |=
                ((i * chunk_bits + j) < v.size() && v[i * chunk_bits + j] ? 1ll
                                                                          : 0ll)
                << (j % GMP_NUMB_BITS);
        }
        result[i] = FieldT(b);
    }

    return result;
}

template<typename FieldT>
std::vector<FieldT> pack_bit_vector_into_field_element_vector(
    const bit_vector &v)
{
    return pack_bit_vector_into_field_element_vector<FieldT>(
        v, FieldT::capacity());
}

template<typename FieldT>
std::vector<FieldT> convert_bit_vector_to_field_element_vector(
    const bit_vector &v)
{
    std::vector<FieldT> result;
    result.reserve(v.size());

    for (const bool b : v) {
        result.emplace_back(b ? FieldT::one() : FieldT::zero());
    }

    return result;
}

template<typename FieldT>
bit_vector convert_field_element_vector_to_bit_vector(
    const std::vector<FieldT> &v)
{
    bit_vector result;

    for (const FieldT &el : v) {
        const bit_vector el_bits =
            convert_field_element_to_bit_vector<FieldT>(el);
        result.insert(result.end(), el_bits.begin(), el_bits.end());
    }

    return result;
}

template<typename FieldT>
bit_vector convert_field_element_to_bit_vector(const FieldT &el)
{
    bit_vector result;

    bigint<FieldT::num_limbs> b = el.as_bigint();
    for (size_t i = 0; i < FieldT::size_in_bits(); ++i) {
        result.push_back(b.test_bit(i));
    }

    return result;
}

template<typename FieldT>
bit_vector convert_field_element_to_bit_vector(
    const FieldT &el, const size_t bitcount)
{
    bit_vector result = convert_field_element_to_bit_vector(el);
    result.resize(bitcount);

    return result;
}

template<typename FieldT>
FieldT convert_bit_vector_to_field_element(const bit_vector &v)
{
    assert(v.size() <= FieldT::size_in_bits());

    FieldT res = FieldT::zero();
    FieldT c = FieldT::one();
    for (bool b : v) {
        res += b ? c : FieldT::zero();
        c += c;
    }
    return res;
}

template<typename FieldT> void batch_invert(std::vector<FieldT> &vec)
{
    std::vector<FieldT> prod;
    prod.reserve(vec.size());

    FieldT acc = FieldT::one();

    for (auto el : vec) {
        assert(!el.is_zero());
        prod.emplace_back(acc);
        acc = acc * el;
    }

    FieldT acc_inverse = acc.inverse();

    for (long i = static_cast<long>(vec.size() - 1); i >= 0; --i) {
        const FieldT old_el = vec[i];
        vec[i] = acc_inverse * prod[i];
        acc_inverse = acc_inverse * old_el;
    }
}

template<typename FieldT>
const typename FieldT::my_Fp &field_get_component_0(const FieldT &v)
{
    return internal::component_0_getter<FieldT>::get_component_0(v);
}

template<
    mp_size_t wn,
    const bigint<wn> &wmodulus,
    mp_size_t nn,
    const bigint<nn> &nmodulus>
void fp_from_fp(Fp_model<wn, wmodulus> &wfp, const Fp_model<nn, nmodulus> &nfp)
{
    bigint<wn> wint;
    const bigint<nn> nint = nfp.as_bigint();
    assert(wint.max_bits() >= nint.max_bits());
    for (size_t limb_idx = 0; limb_idx < nn; ++limb_idx) {
        wint.data[limb_idx] = nint.data[limb_idx];
    }

    wfp = Fp_model<wn, wmodulus>(wint);
}

} // namespace libff

#endif // FIELD_UTILS_TCC_
