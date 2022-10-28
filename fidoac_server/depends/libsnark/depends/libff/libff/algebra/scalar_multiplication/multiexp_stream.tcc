/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef MULTIEXP_STREAM_TCC_
#define MULTIEXP_STREAM_TCC_

#include "libff/algebra/scalar_multiplication/multiexp.hpp"

namespace libff
{

template<form_t Form, compression_t Comp, typename GroupT>
void elements_from_stream_producer(
    std::istream &in_s,
    concurrent_fifo_spsc<GroupT> &fifo,
    const size_t num_entries)
{
    // Read and enqueue all entries from the stream
    for (size_t element_idx = 0; element_idx < num_entries; ++element_idx) {
        GroupT *dest = fifo.enqueue_begin_wait();
        group_read<encoding_binary, Form, Comp>(*dest, in_s);
        fifo.enqueue_end();
    }
}

/// Read blocks of elements and enqueue them as buffers into a
/// concurrent_buffer_fifo_spsc.
template<form_t Form, compression_t Comp, typename GroupT>
void element_buffers_from_stream_producer(
    std::istream &in_s,
    concurrent_buffer_fifo_spsc<GroupT> &fifo,
    const size_t num_buffers,
    const size_t num_elements_per_buffer)
{
    // Read and enqueue each buffer
    for (size_t element_idx = 0; element_idx < num_buffers; ++element_idx) {
        GroupT *dest = fifo.enqueue_begin_wait();
        GroupT *const dest_end = dest + num_elements_per_buffer;
        for (; dest < dest_end; ++dest) {
            group_read<encoding_binary, Form, Comp>(*dest, in_s);
        }
        fifo.enqueue_end();
    }
}

template<typename GroupT, typename FieldT>
GroupT multi_exp_base_elements_from_fifo_all_rounds(
    concurrent_fifo_spsc<GroupT> &fifo,
    const std::vector<FieldT> &exponents,
    const size_t c)
{
    const size_t num_entries = exponents.size();
    // Allow sufficient rounds for num_bits + 2, to accomodate overflow +
    // negative final digit.
    const size_t num_digits = (FieldT::num_bits + 2 + c - 1) / c;
    const size_t num_buckets = 1 << (c - 1);

    // Allocate state for all rounds.
    std::vector<std::vector<GroupT>> round_buckets(num_digits);
    std::vector<std::vector<bool>> round_bucket_hit(num_digits);
    for (std::vector<GroupT> &buckets : round_buckets) {
        buckets.resize(num_buckets);
    }
    for (std::vector<bool> &bucket_hit : round_bucket_hit) {
        bucket_hit.resize(num_buckets);
    }
    std::vector<ssize_t> digits(num_digits);

    // Process each element
    for (size_t el_idx = 0; el_idx < num_entries; ++el_idx) {
        // Decompose the scalar, and wait for an element from the fifo
        field_get_signed_digits(digits, exponents[el_idx], c, num_digits);
        const GroupT &group_element = *(fifo.dequeue_begin_wait());

        // Process all digits
        for (size_t digit_idx = 0; digit_idx < num_digits; ++digit_idx) {
            std::vector<GroupT> &buckets = round_buckets[digit_idx];
            std::vector<bool> &bucket_hit = round_bucket_hit[digit_idx];
            const ssize_t digit = digits[digit_idx];

            internal::multi_exp_add_element_to_bucket_with_signed_digit<
                GroupT,
                multi_exp_base_form_special>(
                buckets, bucket_hit, group_element, digit);
        }

        fifo.dequeue_end();
    }

    // For each digit, sum the buckets and accumulate the total
    GroupT result = internal::
        multiexp_accumulate_buckets<GroupT, multi_exp_base_form_normal>(
            round_buckets[num_digits - 1],
            round_bucket_hit[num_digits - 1],
            num_buckets);
    round_buckets[num_digits - 1].clear();
    round_bucket_hit[num_digits - 1].clear();

    for (size_t digit_idx = num_digits - 2; digit_idx < num_digits;
         --digit_idx) {
        for (size_t i = 0; i < c; ++i) {
            result = result.dbl();
        }

        const GroupT digit_sum = internal::
            multiexp_accumulate_buckets<GroupT, multi_exp_base_form_normal>(
                round_buckets[digit_idx],
                round_bucket_hit[digit_idx],
                num_buckets);
        round_buckets[digit_idx].clear();
        round_bucket_hit[digit_idx].clear();

        result = result + digit_sum;
    }

    return result;
}

template<typename GroupT, typename FieldT>
GroupT multi_exp_precompute_from_fifo(
    concurrent_buffer_fifo_spsc<GroupT> &fifo,
    const std::vector<FieldT> &exponents,
    const size_t c,
    const size_t num_digits)
{
    const size_t num_entries = exponents.size();
    const size_t num_buckets = 1 << (c - 1);

    // Allocate state (single collection of buckets).
    std::vector<GroupT> buckets(num_buckets);
    std::vector<bool> bucket_hit(num_buckets, false);
    std::vector<ssize_t> digits(num_digits);

    // Process each element
    for (size_t el_idx = 0; el_idx < num_entries; ++el_idx) {
        // Decompose the scalar into the signed digits
        field_get_signed_digits(digits, exponents[el_idx], c, num_digits);

        // Wait for the precomputed data
        const GroupT *const precomputed = fifo.dequeue_begin_wait();

        // Process all digits, using the preceomputed data
        for (size_t digit_idx = 0; digit_idx < num_digits; ++digit_idx) {
            internal::multi_exp_add_element_to_bucket_with_signed_digit<
                GroupT,
                multi_exp_base_form_special>(
                buckets, bucket_hit, precomputed[digit_idx], digits[digit_idx]);
        }

        fifo.dequeue_end();
    }

    // Sum the buckets
    return internal::
        multiexp_accumulate_buckets<GroupT, multi_exp_base_form_normal>(
            buckets, bucket_hit, num_buckets);
}

template<form_t Form, compression_t Comp, typename GroupT, typename FieldT>
GroupT multi_exp_stream(
    std::istream &base_elements_in, const std::vector<FieldT> &exponents)
{
    static const size_t FIFO_SIZE = 1024;
    const size_t num_entries = exponents.size();
    const size_t c = bdlo12_signed_optimal_c(num_entries);
    assert(c > 0);

    // Fifo for streaming
    concurrent_fifo_spsc<GroupT> fifo(FIFO_SIZE);

    // Launch the reading thread
    std::thread producer([&base_elements_in, &fifo, num_entries]() {
        elements_from_stream_producer<Form, Comp, GroupT>(
            base_elements_in, fifo, num_entries);
    });

    // Consume all elements from the fifo.
    const GroupT result =
        multi_exp_base_elements_from_fifo_all_rounds<GroupT, FieldT>(
            fifo, exponents, c);

    // Wait for reading thread
    producer.join();

    return result;
}

template<form_t Form, compression_t Comp, typename GroupT, typename FieldT>
GroupT multi_exp_stream_with_precompute(
    std::istream &base_elements_in,
    const std::vector<FieldT> &exponents,
    const size_t c)
{
    // Each entry is a buffer. We may want to tweak this (possibly depending on
    // exponents.size() and precompute_c), but for now we expect 8 to be a
    // reasonable number to avoid too much blocking.
    static const size_t FIFO_SIZE = 8;
    const size_t num_entries = exponents.size();
    const size_t num_digits = (FieldT::num_bits + c - 1) / c;

    // Construct the fifo and begin reading buffers. The fifo has size
    // FIFO_SIZE where each entry is all the precomputed values for a single
    // base element (i.e. the multiple for each digit).
    concurrent_buffer_fifo_spsc<GroupT> fifo(FIFO_SIZE, num_digits);

    std::thread producer([&base_elements_in, &fifo, num_entries, num_digits]() {
        element_buffers_from_stream_producer<Form, Comp, GroupT>(
            base_elements_in, fifo, num_entries, num_digits);
    });

    // Consume all elements from the fifo to compute the final result.
    const GroupT result = multi_exp_precompute_from_fifo<GroupT, FieldT>(
        fifo, exponents, c, num_digits);

    producer.join();

    return result;
}

} // namespace libff

#endif // MULTIEXP_STREAM_TCC_
