/** @file
 *****************************************************************************

 Declaration of interfaces for multi-exponentiation routines.

 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef MULTIEXP_HPP_
#define MULTIEXP_HPP_

#include <cstddef>
#include <vector>

namespace libff
{

enum multi_exp_method {
    /// Naive multi-exponentiation individually multiplies each base by the
    /// corresponding scalar and adds up the results. multi_exp_method_naive
    /// uses opt_window_wnaf_exp for exponentiation.
    multi_exp_method_naive,
    /// As multi_exp_method_naive, but uses operator * rather than
    /// opt_window_wnaf_exp.
    multi_exp_method_naive_plain,
    /// A variant of the Bos-Coster algorithm [1], with implementation
    /// suggestions from [2].
    ///   [1] = Bos and Coster, "Addition chain heuristics", CRYPTO '89
    ///   [2] = Bernstein, Duif, Lange, Schwabe, and Yang, "High-speed
    ///         high-security signatures", CHES '11
    multi_exp_method_bos_coster,
    /// A special case of Pippenger's algorithm from Page 15 of Bernstein,
    /// Doumen, Lange, Oosterwijk, "Faster batch forgery identification",
    /// INDOCRYPT 2012 (https://eprint.iacr.org/2012/549.pdf)
    /// Requires that T implements .dbl()
    multi_exp_method_BDLO12,
    /// Similar to multi_exp_method_BDLO12, but using signed digits.
    multi_exp_method_BDLO12_signed,
};

/// Form of base elements passed to multi_exp routines.
enum multi_exp_base_form {
    /// Incoming base elements are not in special form.
    multi_exp_base_form_normal,
    /// Incoming base elements are in special form (so that implementations can
    /// use mixed_add, where appropriate.
    multi_exp_base_form_special,
};

/// Optimal value of digit size (commonly refered to as c here) for
/// BDLO12-style algorithms which use signed digits. In general, this is used
/// internally by the routines here, but is useful externally in some cases
/// (for example where precomputing must be performed).
static inline size_t bdlo12_signed_optimal_c(size_t num_entries);

/// Computes the sum:
///   \sum_i scalar_start[i] * vec_start[i]
/// using the selected method. Input is split into the given number of chunks,
/// and processed in parallel.
template<
    typename T,
    typename FieldT,
    multi_exp_method Method,
    multi_exp_base_form BaseForm = multi_exp_base_form_normal>
T multi_exp(
    typename std::vector<T>::const_iterator vec_start,
    typename std::vector<T>::const_iterator vec_end,
    typename std::vector<FieldT>::const_iterator scalar_start,
    typename std::vector<FieldT>::const_iterator scalar_end,
    const size_t chunks);

/// A variant of multi_exp which includes special pre-processing step to skip
/// zeros, and directly sum base elements with factor 1. Remaining values are
/// processed as usual via multi_exp.
template<
    typename T,
    typename FieldT,
    multi_exp_method Method,
    multi_exp_base_form BaseForm = multi_exp_base_form_normal>
T multi_exp_filter_one_zero(
    typename std::vector<T>::const_iterator vec_start,
    typename std::vector<T>::const_iterator vec_end,
    typename std::vector<FieldT>::const_iterator scalar_start,
    typename std::vector<FieldT>::const_iterator scalar_end,
    const size_t chunks);

/// A convenience function for calculating a pure inner product, where the more
/// complicated methods are not required.
template<typename T>
T inner_product(
    typename std::vector<T>::const_iterator a_start,
    typename std::vector<T>::const_iterator a_end,
    typename std::vector<T>::const_iterator b_start,
    typename std::vector<T>::const_iterator b_end);

/// A window table stores window sizes for different instance sizes for
/// fixed-base multi-scalar multiplications.
template<typename T> using window_table = std::vector<std::vector<T>>;

/// Compute window size for the given number of scalars.
template<typename T> size_t get_exp_window_size(const size_t num_scalars);

/// Compute table of window sizes.
template<typename T>
window_table<T> get_window_table(
    const size_t scalar_size, const size_t window, const T &g);

template<typename T, typename FieldT>
T windowed_exp(
    const size_t scalar_size,
    const size_t window,
    const window_table<T> &powers_of_g,
    const FieldT &pow);

template<typename T, typename FieldT>
std::vector<T> batch_exp(
    const size_t scalar_size,
    const size_t window,
    const window_table<T> &table,
    const std::vector<FieldT> &v);

template<typename T, typename FieldT>
std::vector<T> batch_exp(
    const size_t scalar_size,
    const size_t window,
    const window_table<T> &table,
    const std::vector<FieldT> &v,
    size_t num_entries);

template<typename T, typename FieldT>
std::vector<T> batch_exp_with_coeff(
    const size_t scalar_size,
    const size_t window,
    const window_table<T> &table,
    const FieldT &coeff,
    const std::vector<FieldT> &v);

template<typename T> void batch_to_special(std::vector<T> &vec);

} // namespace libff

#include <libff/algebra/scalar_multiplication/multiexp.tcc>

#endif // MULTIEXP_HPP_
