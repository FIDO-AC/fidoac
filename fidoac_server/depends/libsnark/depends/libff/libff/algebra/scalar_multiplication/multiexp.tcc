/** @file
 *****************************************************************************

 Implementation of interfaces for multi-exponentiation routines.

 See multiexp.hpp .

 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef MULTIEXP_TCC_
#define MULTIEXP_TCC_

#include <algorithm>
#include <cassert>
#include <libff/algebra/curves/curve_serialization.hpp>
#include <libff/algebra/fields/bigint.hpp>
#include <libff/algebra/fields/fp_aux.tcc>
#include <libff/algebra/scalar_multiplication/multiexp.hpp>
#include <libff/algebra/scalar_multiplication/wnaf.hpp>
#include <libff/common/concurrent_fifo.hpp>
#include <libff/common/profiling.hpp>
#include <libff/common/utils.hpp>
#include <type_traits>

namespace libff
{

namespace internal
{

inline size_t pippenger_optimal_c(const size_t num_elements)
{
    // empirically, this seems to be a decent estimate of the optimal value of c
    const size_t log2_num_elements = log2(num_elements);
    return log2_num_elements - (log2_num_elements / 3 - 2);
}

/// Add/subtract base_element to/from the correct bucket, based on a signed
/// digit, using and updating the bucket_hit flags. Supports regular / mixed
/// addition, based on base element form.
template<typename GroupT, multi_exp_base_form BaseForm>
void multi_exp_add_element_to_bucket_with_signed_digit(
    std::vector<GroupT> &buckets,
    std::vector<bool> &bucket_hit,
    const GroupT &base_element,
    ssize_t digit)
{
    if (digit < 0) {
        const size_t bucket_idx = (-digit) - 1;
        assert(bucket_idx < buckets.size());
        if (bucket_hit[bucket_idx]) {
            if (BaseForm == multi_exp_base_form_special) {
                buckets[bucket_idx] =
                    buckets[bucket_idx].mixed_add(-base_element);
            } else {
                buckets[bucket_idx] = buckets[bucket_idx].add(-base_element);
            }
        } else {
            buckets[bucket_idx] = -base_element;
            bucket_hit[bucket_idx] = true;
        }
    } else if (digit > 0) {
        const size_t bucket_idx = digit - 1;
        assert(bucket_idx < buckets.size());
        if (bucket_hit[bucket_idx]) {
            if (BaseForm == multi_exp_base_form_special) {
                buckets[bucket_idx] =
                    buckets[bucket_idx].mixed_add(base_element);
            } else {
                buckets[bucket_idx] = buckets[bucket_idx].add(base_element);
            }
        } else {
            buckets[bucket_idx] = base_element;
            bucket_hit[bucket_idx] = true;
        }
    }
}

/// Compute:
///   \sum_{i=1}^{num_buckets} [i] B_i
/// where
///   B_i is the i-th bucket value (stored in buckets[i - 1])
///
/// Caller must ensure that at least one bucket is not empty (i.e.
/// bucket_hit[i] == true for at least one i).
template<typename GroupT, multi_exp_base_form Form>
GroupT multiexp_accumulate_buckets(
    std::vector<GroupT> &buckets,
    std::vector<bool> &bucket_hit,
    const size_t num_buckets)
{
    // Find first set bucket and initialize the accumulator. For each remaining
    // buckets (set or unset), add the bucket value to the accumulator (if
    // set), and add the accumulator to the sum. In this way, the i-th bucket
    // will be added to sum exactly i times.

    size_t i = num_buckets - 1;
    while (!bucket_hit[i]) {
        --i;

        // Ensure that at least one bucket is initialized.
        assert(i < num_buckets);
    }

    GroupT sum;
    GroupT accumulator;
    sum = accumulator = buckets[i];
    while (i > 0) {
        --i;
        if (bucket_hit[i]) {
            if (Form == multi_exp_base_form_special) {
                accumulator = accumulator.mixed_add(buckets[i]);
            } else {
                accumulator = accumulator + buckets[i];
            }
        }
        sum = sum + accumulator;
    }

    return sum;
}

template<mp_size_t n> class ordered_exponent
{
    // to use std::push_heap and friends later
public:
    size_t idx;
    bigint<n> r;

    ordered_exponent(const size_t idx, const bigint<n> &r) : idx(idx), r(r){};

    bool operator<(const ordered_exponent<n> &other) const
    {
#if defined(__x86_64__) && defined(USE_ASM)
        if (n == 3) {
            long res;
            __asm__(                                   // Preserve alignment
                "// check for overflow           \n\t" //
                "mov $0, %[res]                  \n\t" //
                ADD_CMP(16)                            //
                ADD_CMP(8)                             //
                ADD_CMP(0)                             //
                "jmp done%=                      \n\t" //
                "subtract%=:                     \n\t" //
                "mov $1, %[res]                  \n\t" //
                "done%=:                         \n\t" //
                : [res] "=&r"(res)
                : [A] "r"(other.r.data), [mod] "r"(this->r.data)
                : "cc", "%rax");
            return res;
        } else if (n == 4) {
            long res;
            __asm__(                                   // Preserve alignment
                "// check for overflow           \n\t" //
                "mov $0, %[res]                  \n\t" //
                ADD_CMP(24)                            //
                ADD_CMP(16)                            //
                ADD_CMP(8)                             //
                ADD_CMP(0)                             //
                "jmp done%=                      \n\t" //
                "subtract%=:                     \n\t" //
                "mov $1, %[res]                  \n\t" //
                "done%=:                         \n\t" //
                : [res] "=&r"(res)
                : [A] "r"(other.r.data), [mod] "r"(this->r.data)
                : "cc", "%rax");
            return res;
        } else if (n == 5) {
            long res;
            __asm__(                                   // Preserve alignment
                "// check for overflow           \n\t" //
                "mov $0, %[res]                  \n\t" //
                ADD_CMP(32)                            //
                ADD_CMP(24)                            //
                ADD_CMP(16)                            //
                ADD_CMP(8)                             //
                ADD_CMP(0)                             //
                "jmp done%=                      \n\t" //
                "subtract%=:                     \n\t" //
                "mov $1, %[res]                  \n\t" //
                "done%=:                         \n\t" //
                : [res] "=&r"(res)
                : [A] "r"(other.r.data), [mod] "r"(this->r.data)
                : "cc", "%rax");
            return res;
        } else
#endif
        {
            return (mpn_cmp(this->r.data, other.r.data, n) < 0);
        }
    }
};

// Class holding the specialized multi exp implementations. Must implement a
// public static method of the form:
//   static GroupT multi_exp_inner(
//       typename std::vector<GroupT>::const_iterator bases,
//       typename std::vector<GroupT>::const_iterator bases_end,
//       typename std::vector<FieldT>::const_iterator exponents,
//       typename std::vector<FieldT>::const_iterator exponents_end);
template<
    typename GroupT,
    typename FieldT,
    multi_exp_method Method,
    multi_exp_base_form BaseForm>
class multi_exp_implementation
{
};

/// Naive multi_exp_implementation
template<typename GroupT, typename FieldT, multi_exp_base_form BaseForm>
class multi_exp_implementation<GroupT, FieldT, multi_exp_method_naive, BaseForm>
{
public:
    static GroupT multi_exp_inner(
        typename std::vector<GroupT>::const_iterator vec_start,
        typename std::vector<GroupT>::const_iterator vec_end,
        typename std::vector<FieldT>::const_iterator scalar_start,
        typename std::vector<FieldT>::const_iterator scalar_end)
    {
        GroupT result(GroupT::zero());

        typename std::vector<GroupT>::const_iterator vec_it;
        typename std::vector<FieldT>::const_iterator scalar_it;

        for (vec_it = vec_start, scalar_it = scalar_start; vec_it != vec_end;
             ++vec_it, ++scalar_it) {
            bigint<FieldT::num_limbs> scalar_bigint = scalar_it->as_bigint();
            result =
                result + opt_window_wnaf_exp(
                             *vec_it, scalar_bigint, scalar_bigint.num_bits());
        }
        assert(scalar_it == scalar_end);
        UNUSED(scalar_end);

        return result;
    }
};

// Naive plain multi_exp_implementation
template<typename GroupT, typename FieldT, multi_exp_base_form BaseForm>
class multi_exp_implementation<
    GroupT,
    FieldT,
    multi_exp_method_naive_plain,
    BaseForm>
{
public:
    static GroupT multi_exp_inner(
        typename std::vector<GroupT>::const_iterator vec_start,
        typename std::vector<GroupT>::const_iterator vec_end,
        typename std::vector<FieldT>::const_iterator scalar_start,
        typename std::vector<FieldT>::const_iterator scalar_end)
    {
        GroupT result(GroupT::zero());

        typename std::vector<GroupT>::const_iterator vec_it;
        typename std::vector<FieldT>::const_iterator scalar_it;

        for (vec_it = vec_start, scalar_it = scalar_start; vec_it != vec_end;
             ++vec_it, ++scalar_it) {
            result = result + (*scalar_it) * (*vec_it);
        }
        assert(scalar_it == scalar_end);
        UNUSED(scalar_end);

        return result;
    }
};

// multi_exp_implementation for BDLO12
template<typename GroupT, typename FieldT, multi_exp_base_form BaseForm>
class multi_exp_implementation<
    GroupT,
    FieldT,
    multi_exp_method_BDLO12,
    BaseForm>
{
public:
    static GroupT multi_exp_inner(
        typename std::vector<GroupT>::const_iterator bases,
        typename std::vector<GroupT>::const_iterator bases_end,
        typename std::vector<FieldT>::const_iterator exponents,
        typename std::vector<FieldT>::const_iterator exponents_end)
    {
        UNUSED(exponents_end);
        const size_t length = bases_end - bases;
        const size_t c = internal::pippenger_optimal_c(length);

        const mp_size_t exp_num_limbs =
            std::remove_reference<decltype(*exponents)>::type::num_limbs;
        std::vector<bigint<exp_num_limbs>> bi_exponents(length);
        size_t num_bits = 0;

        for (size_t i = 0; i < length; i++) {
            bi_exponents[i] = exponents[i].as_bigint();
            num_bits = std::max(num_bits, bi_exponents[i].num_bits());
        }

        const size_t num_groups = (num_bits + c - 1) / c;

        GroupT result;
        bool result_nonzero = false;

        for (size_t k = num_groups - 1; k <= num_groups; k--) {
            if (result_nonzero) {
                for (size_t i = 0; i < c; i++) {
                    result = result.dbl();
                }
            }

            std::vector<GroupT> buckets(1 << c);
            std::vector<bool> bucket_nonzero(1 << c);

            for (size_t i = 0; i < length; i++) {
                // id = k-th "digit" of bi_exponents[i], radix 2^c
                //    = (bi_exponents[i] >> (c*k)) & (2^c - 1)
                size_t id = 0;
                for (size_t j = 0; j < c; j++) {
                    if (bi_exponents[i].test_bit(k * c + j)) {
                        id |= 1 << j;
                    }
                }

                // Skip 0 digits.
                if (id == 0) {
                    continue;
                }

                // Add (or write) the group element into the appropriate bucket.
                if (bucket_nonzero[id]) {
                    if (BaseForm == multi_exp_base_form_special) {
                        buckets[id] = buckets[id].mixed_add(bases[i]);
                    } else {
                        buckets[id] = buckets[id] + bases[i];
                    }
                } else {
                    buckets[id] = bases[i];
                    bucket_nonzero[id] = true;
                }
            }

#ifdef USE_MIXED_ADDITION
            batch_to_special(buckets);
#endif

            GroupT running_sum;
            bool running_sum_nonzero = false;

            for (size_t i = (1u << c) - 1; i > 0; i--) {
                if (bucket_nonzero[i]) {
                    if (running_sum_nonzero) {
#ifdef USE_MIXED_ADDITION
                        running_sum = running_sum.mixed_add(buckets[i]);
#else
                        running_sum = running_sum + buckets[i];
#endif
                    } else {
                        running_sum = buckets[i];
                        running_sum_nonzero = true;
                    }
                }

                if (running_sum_nonzero) {
                    if (result_nonzero) {
                        result = result + running_sum;
                    } else {
                        result = running_sum;
                        result_nonzero = true;
                    }
                }
            }
        }

        return result;
    }
};

template<typename GroupT, typename FieldT, multi_exp_base_form BaseForm>
class multi_exp_implementation<
    GroupT,
    FieldT,
    multi_exp_method_bos_coster,
    BaseForm>
{
public:
    static GroupT multi_exp_inner(
        typename std::vector<GroupT>::const_iterator vec_start,
        typename std::vector<GroupT>::const_iterator vec_end,
        typename std::vector<FieldT>::const_iterator scalar_start,
        typename std::vector<FieldT>::const_iterator scalar_end)
    {
        const mp_size_t n =
            std::remove_reference<decltype(*scalar_start)>::type::num_limbs;

        if (vec_start == vec_end) {
            return GroupT::zero();
        }

        if (vec_start + 1 == vec_end) {
            return (*scalar_start) * (*vec_start);
        }

        std::vector<ordered_exponent<n>> opt_q;
        const size_t vec_len = scalar_end - scalar_start;
        const size_t odd_vec_len = (vec_len % 2 == 1 ? vec_len : vec_len + 1);
        opt_q.reserve(odd_vec_len);
        std::vector<GroupT> g;
        g.reserve(odd_vec_len);

        typename std::vector<GroupT>::const_iterator vec_it;
        typename std::vector<FieldT>::const_iterator scalar_it;
        size_t i;
        for (i = 0, vec_it = vec_start, scalar_it = scalar_start;
             vec_it != vec_end;
             ++vec_it, ++scalar_it, ++i) {
            g.emplace_back(*vec_it);

            opt_q.emplace_back(ordered_exponent<n>(i, scalar_it->as_bigint()));
        }
        std::make_heap(opt_q.begin(), opt_q.end());
        assert(scalar_it == scalar_end);

        if (vec_len != odd_vec_len) {
            g.emplace_back(GroupT::zero());
            opt_q.emplace_back(
                ordered_exponent<n>(odd_vec_len - 1, bigint<n>(0ul)));
        }
        assert(g.size() % 2 == 1);
        assert(opt_q.size() == g.size());

        GroupT opt_result = GroupT::zero();

        while (true) {
            ordered_exponent<n> &a = opt_q[0];
            ordered_exponent<n> &b =
                (opt_q[1] < opt_q[2] ? opt_q[2] : opt_q[1]);

            const size_t abits = a.r.num_bits();

            if (b.r.is_zero()) {
                // opt_result = opt_result + (a.r * g[a.idx]);
                opt_result =
                    opt_result + opt_window_wnaf_exp(g[a.idx], a.r, abits);
                break;
            }

            const size_t bbits = b.r.num_bits();
            const size_t limit = (abits - bbits >= 20 ? 20 : abits - bbits);

            if (bbits < 1ul << limit) {
                /*
                  In this case, exponentiating to the power of a is cheaper than
                  subtracting b from a multiple times, so let's do it directly
                */
                // opt_result = opt_result + (a.r * g[a.idx]);
                opt_result =
                    opt_result + opt_window_wnaf_exp(g[a.idx], a.r, abits);
#ifdef DEBUG
                printf(
                    "Skipping the following pair (%zu bit number vs %zu "
                    "bit):\n",
                    abits,
                    bbits);
                a.r.print();
                b.r.print();
#endif
                a.r.clear();
            } else {
                // x A + y B => (x-y) A + y (B+A)
                mpn_sub_n(a.r.data, a.r.data, b.r.data, n);
                g[b.idx] = g[b.idx] + g[a.idx];
            }

            // regardless of whether a was cleared or subtracted from we push it
            // down, then take back up

            /* heapify A down */
            size_t a_pos = 0;
            while (2 * a_pos + 2 < odd_vec_len) {
                // this is a max-heap so to maintain a heap property we swap
                // with the largest of the two
                if (opt_q[2 * a_pos + 1] < opt_q[2 * a_pos + 2]) {
                    std::swap(opt_q[a_pos], opt_q[2 * a_pos + 2]);
                    a_pos = 2 * a_pos + 2;
                } else {
                    std::swap(opt_q[a_pos], opt_q[2 * a_pos + 1]);
                    a_pos = 2 * a_pos + 1;
                }
            }

            /* now heapify A up appropriate amount of times */
            while (a_pos > 0 && opt_q[(a_pos - 1) / 2] < opt_q[a_pos]) {
                std::swap(opt_q[a_pos], opt_q[(a_pos - 1) / 2]);
                a_pos = (a_pos - 1) / 2;
            }
        }

        return opt_result;
    }
};

template<typename GroupT, typename FieldT, multi_exp_base_form BaseForm>
class multi_exp_implementation<
    GroupT,
    FieldT,
    multi_exp_method_BDLO12_signed,
    BaseForm>
{
public:
    using BigInt =
        typename std::decay<decltype(((FieldT *)nullptr)->mont_repr)>::type;

    /// buckets and bucket_hit should have at least 2^{c-1} entries.
    static GroupT signed_digits_round(
        typename std::vector<GroupT>::const_iterator bases,
        typename std::vector<GroupT>::const_iterator bases_end,
        typename std::vector<BigInt>::const_iterator exponents,
        std::vector<GroupT> &buckets,
        std::vector<bool> &bucket_hit,
        const size_t num_entries,
        const size_t num_buckets,
        const size_t c,
        const size_t digit_idx)
    {
        UNUSED(bases_end);

        assert(buckets.size() >= num_buckets);
        assert(bucket_hit.size() >= num_buckets);

        // Zero bucket_hit array.
        bucket_hit.assign(num_buckets, false);

        // For each scalar, element pair ...
        size_t non_zero = 0;
        for (size_t i = 0; i < num_entries; ++i) {
            const ssize_t digit =
                field_get_signed_digit(exponents[i], c, digit_idx);
            if (digit == 0) {
                continue;
            }

            multi_exp_add_element_to_bucket_with_signed_digit<GroupT, BaseForm>(
                buckets, bucket_hit, bases[i], digit);
            ++non_zero;
        }

        // Check up-front for the edge-case where no buckets have been touched.
        if (non_zero == 0) {
            return GroupT::zero();
        }

        // TODO: consider converting buckets to special form

        return multiexp_accumulate_buckets<GroupT, multi_exp_base_form_normal>(
            buckets, bucket_hit, num_buckets);
    }

    static GroupT multi_exp_inner(
        typename std::vector<GroupT>::const_iterator bases,
        typename std::vector<GroupT>::const_iterator bases_end,
        typename std::vector<FieldT>::const_iterator exponents,
        typename std::vector<FieldT>::const_iterator exponents_end)
    {
        UNUSED(exponents_end);

        const size_t num_entries = bases_end - bases;
        assert(exponents_end - exponents == (ssize_t)num_entries);
        const size_t c = bdlo12_signed_optimal_c(num_entries);
        assert(c > 0);

        // Pre-compute the bigint values
        size_t num_bits = 0;
        std::vector<BigInt> bi_exponents(num_entries);
        for (size_t i = 0; i < num_entries; ++i) {
            bi_exponents[i] = exponents[i].as_bigint();
            num_bits = std::max(num_bits, bi_exponents[i].num_bits());
        }

        // Allow sufficient rounds for num_bits + 2, to accomodate overflow +
        // negative final digit.
        const size_t num_rounds = (num_bits + 2 + c - 1) / c;

        // Digits have values between -(2^{c-1}) and 2^{c-1} - 1. Negative
        // values are negated (to make them positive since we have cheap
        // inversion of base elements), and 0 values are ignored. Hence we
        // require up to 2^{c-1} buckets
        const size_t num_buckets = 1 << (c - 1);

        // Allocate the round state once, and reuse it.
        std::vector<GroupT> buckets(num_buckets);
        std::vector<bool> bucket_hit(num_buckets);
        assert(buckets.size() == num_buckets);
        assert(bucket_hit.size() == num_buckets);

        // Compute from highest-order to lowest-order digits, accumulating at
        // the same time.
        GroupT result = signed_digits_round(
            bases,
            bases_end,
            bi_exponents.begin(),
            buckets,
            bucket_hit,
            num_entries,
            num_buckets,
            c,
            num_rounds - 1);
        for (size_t round_idx = 1; round_idx < num_rounds; ++round_idx) {
            const size_t digit_idx = num_rounds - 1 - round_idx;
            for (size_t i = 0; i < c; ++i) {
                result = result.dbl();
            }

            const GroupT round_result = signed_digits_round(
                bases,
                bases_end,
                bi_exponents.begin(),
                buckets,
                bucket_hit,
                num_entries,
                num_buckets,
                c,
                digit_idx);
            result = result + round_result;
        }

        return result;
    }
};

} // namespace internal

static inline size_t bdlo12_signed_optimal_c(size_t num_entries)
{
    // For now, this seems like a good estimate in most cases.
    return internal::pippenger_optimal_c(num_entries) + 1;
}

template<
    typename GroupT,
    typename FieldT,
    multi_exp_method Method,
    multi_exp_base_form BaseForm>
GroupT multi_exp(
    typename std::vector<GroupT>::const_iterator vec_start,
    typename std::vector<GroupT>::const_iterator vec_end,
    typename std::vector<FieldT>::const_iterator scalar_start,
    typename std::vector<FieldT>::const_iterator scalar_end,
    const size_t chunks)
{
    const size_t total = vec_end - vec_start;
    if ((total < chunks) || (chunks == 1)) {
        // no need to split into "chunks", can call implementation directly
        return internal::
            multi_exp_implementation<GroupT, FieldT, Method, BaseForm>::
                multi_exp_inner(vec_start, vec_end, scalar_start, scalar_end);
    }

    const size_t one = total / chunks;

    std::vector<GroupT> partial(chunks, GroupT::zero());

#ifdef MULTICORE
#pragma omp parallel for
#endif
    for (size_t i = 0; i < chunks; ++i) {
        partial[i] = internal::
            multi_exp_implementation<GroupT, FieldT, Method, BaseForm>::
                multi_exp_inner(
                    vec_start + i * one,
                    (i == chunks - 1) ? vec_end : (vec_start + (i + 1) * one),
                    scalar_start + i * one,
                    (i == chunks - 1) ? scalar_end
                                      : (scalar_start + (i + 1) * one));
    }

    GroupT final = GroupT::zero();

    for (size_t i = 0; i < chunks; ++i) {
        final = final + partial[i];
    }

    return final;
}

template<
    typename GroupT,
    typename FieldT,
    multi_exp_method Method,
    multi_exp_base_form BaseForm>
GroupT multi_exp_filter_one_zero(
    typename std::vector<GroupT>::const_iterator vec_start,
    typename std::vector<GroupT>::const_iterator vec_end,
    typename std::vector<FieldT>::const_iterator scalar_start,
    typename std::vector<FieldT>::const_iterator scalar_end,
    const size_t chunks)
{
    assert(
        std::distance(vec_start, vec_end) ==
        std::distance(scalar_start, scalar_end));
    UNUSED(vec_end);
    enter_block("Process scalar vector");
    auto value_it = vec_start;
    auto scalar_it = scalar_start;

    std::vector<FieldT> p;
    std::vector<GroupT> g;

    GroupT acc = GroupT::zero();

    size_t num_skip = 0;
    size_t num_add = 0;
    size_t num_other = 0;

    for (; scalar_it != scalar_end; ++scalar_it, ++value_it) {
        if (scalar_it->is_zero()) {
            // do nothing
            ++num_skip;
        } else if (*scalar_it == FieldT::one()) {
            if (BaseForm == multi_exp_base_form_special) {
                acc = acc.mixed_add(*value_it);
            } else {
                acc = acc + (*value_it);
            }
            ++num_add;
        } else {
            p.emplace_back(*scalar_it);
            g.emplace_back(*value_it);
            ++num_other;
        }
    }

    print_indent();
    printf(
        "* Elements of w skipped: %zu (%0.2f%%)\n",
        num_skip,
        100. * num_skip / (num_skip + num_add + num_other));
    print_indent();
    printf(
        "* Elements of w processed with special addition: %zu (%0.2f%%)\n",
        num_add,
        100. * num_add / (num_skip + num_add + num_other));
    print_indent();
    printf(
        "* Elements of w remaining: %zu (%0.2f%%)\n",
        num_other,
        100. * num_other / (num_skip + num_add + num_other));

    leave_block("Process scalar vector");

    return acc + multi_exp<GroupT, FieldT, Method, BaseForm>(
                     g.begin(), g.end(), p.begin(), p.end(), chunks);
}

template<typename T>
T inner_product(
    typename std::vector<T>::const_iterator a_start,
    typename std::vector<T>::const_iterator a_end,
    typename std::vector<T>::const_iterator b_start,
    typename std::vector<T>::const_iterator b_end)
{
    return multi_exp<T, T, multi_exp_method_naive_plain>(
        a_start, a_end, b_start, b_end, 1);
}

template<typename T> size_t get_exp_window_size(const size_t num_scalars)
{
    if (T::fixed_base_exp_window_table.empty()) {
#ifdef LOWMEM
        return 14;
#else
        return 17;
#endif
    }
    size_t window = 1;
    for (long i = T::fixed_base_exp_window_table.size() - 1; i >= 0; --i) {
#ifdef DEBUG
        if (!inhibit_profiling_info) {
            printf(
                "%ld %zu %zu\n",
                i,
                num_scalars,
                T::fixed_base_exp_window_table[i]);
        }
#endif
        if (T::fixed_base_exp_window_table[i] != 0 &&
            num_scalars >= T::fixed_base_exp_window_table[i]) {
            window = i + 1;
            break;
        }
    }

    if (!inhibit_profiling_info) {
        print_indent();
        printf(
            "Choosing window size %zu for %zu elements\n", window, num_scalars);
    }

#ifdef LOWMEM
    window = std::min((size_t)14, window);
#endif
    return window;
}

template<typename T>
window_table<T> get_window_table(
    const size_t scalar_size, const size_t window, const T &g)
{
    const size_t in_window = 1ul << window;
    const size_t outerc = (scalar_size + window - 1) / window;
    const size_t last_in_window = 1ul << (scalar_size - (outerc - 1) * window);
#ifdef DEBUG
    if (!inhibit_profiling_info) {
        print_indent();
        printf(
            "* scalar_size=%zu; window=%zu; in_window=%zu; outerc=%zu\n",
            scalar_size,
            window,
            in_window,
            outerc);
    }
#endif

    window_table<T> powers_of_g(outerc, std::vector<T>(in_window, T::zero()));

    T gouter = g;

    for (size_t outer = 0; outer < outerc; ++outer) {
        T ginner = T::zero();
        size_t cur_in_window = outer == outerc - 1 ? last_in_window : in_window;
        for (size_t inner = 0; inner < cur_in_window; ++inner) {
            powers_of_g[outer][inner] = ginner;
            ginner = ginner + gouter;
        }

        for (size_t i = 0; i < window; ++i) {
            gouter = gouter + gouter;
        }
    }

    return powers_of_g;
}

template<typename T, typename FieldT>
T windowed_exp(
    const size_t scalar_size,
    const size_t window,
    const window_table<T> &powers_of_g,
    const FieldT &pow)
{
    const size_t outerc = (scalar_size + window - 1) / window;
    const bigint<FieldT::num_limbs> pow_val = pow.as_bigint();

    /* exp */
    T res = powers_of_g[0][0];

    for (size_t outer = 0; outer < outerc; ++outer) {
        size_t inner = 0;
        for (size_t i = 0; i < window; ++i) {
            if (pow_val.test_bit(outer * window + i)) {
                inner |= 1u << i;
            }
        }

        res = res + powers_of_g[outer][inner];
    }

    return res;
}

template<typename T, typename FieldT>
std::vector<T> batch_exp(
    const size_t scalar_size,
    const size_t window,
    const window_table<T> &table,
    const std::vector<FieldT> &v)
{
    return batch_exp(scalar_size, window, table, v, v.size());
}

template<typename T, typename FieldT>
std::vector<T> batch_exp(
    const size_t scalar_size,
    const size_t window,
    const window_table<T> &table,
    const std::vector<FieldT> &v,
    size_t num_entries)
{
    if (!inhibit_profiling_info) {
        print_indent();
    }
    std::vector<T> res(num_entries, table[0][0]);

#ifdef MULTICORE
#pragma omp parallel for
#endif
    for (size_t i = 0; i < num_entries; ++i) {
        res[i] = windowed_exp(scalar_size, window, table, v[i]);

        if (!inhibit_profiling_info && (i % 10000 == 0)) {
            printf(".");
            fflush(stdout);
        }
    }

    if (!inhibit_profiling_info) {
        printf(" DONE!\n");
    }

    return res;
}

template<typename T, typename FieldT>
std::vector<T> batch_exp_with_coeff(
    const size_t scalar_size,
    const size_t window,
    const window_table<T> &table,
    const FieldT &coeff,
    const std::vector<FieldT> &v)
{
    if (!inhibit_profiling_info) {
        print_indent();
    }
    std::vector<T> res(v.size(), table[0][0]);

#ifdef MULTICORE
#pragma omp parallel for
#endif
    for (size_t i = 0; i < v.size(); ++i) {
        res[i] = windowed_exp(scalar_size, window, table, coeff * v[i]);

        if (!inhibit_profiling_info && (i % 10000 == 0)) {
            printf(".");
            fflush(stdout);
        }
    }

    if (!inhibit_profiling_info) {
        printf(" DONE!\n");
    }

    return res;
}

template<typename T> void batch_to_special(std::vector<T> &vec)
{
    enter_block("Batch-convert elements to special form");

    std::vector<T> non_zero_vec;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (!vec[i].is_zero()) {
            non_zero_vec.emplace_back(vec[i]);
        }
    }

    T::batch_to_special_all_non_zeros(non_zero_vec);
    auto it = non_zero_vec.begin();
    T zero_special = T::zero();
    zero_special.to_special();

    for (size_t i = 0; i < vec.size(); ++i) {
        if (!vec[i].is_zero()) {
            vec[i] = *it;
            ++it;
        } else {
            vec[i] = zero_special;
        }
    }
    leave_block("Batch-convert elements to special form");
}

} // namespace libff

#endif // MULTIEXP_TCC_
