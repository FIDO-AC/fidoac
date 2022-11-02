/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef __LIBFF_COMMON_CONCURRENT_FIFO_HPP__
#define __LIBFF_COMMON_CONCURRENT_FIFO_HPP__

#include <atomic>
#include <stddef.h>

namespace libff
{

/// Simple lock-less single-producer, single-consumer fifo buffer. Exposes the
/// raw read/write location directly, in way that the producer can know if the
/// destination memory is available BEFORE he begins data production. In
/// particular, if T = void *, say, the producer can recycle the buffer pointed
/// to by the void * once try_enqueue_begin() succeeds.
template<typename T> class concurrent_fifo_spsc
{
public:
    concurrent_fifo_spsc() = delete;
    concurrent_fifo_spsc(const concurrent_fifo_spsc &) = delete;
    concurrent_fifo_spsc &operator=(const concurrent_fifo_spsc &) = delete;

    concurrent_fifo_spsc(size_t capacity);
    ~concurrent_fifo_spsc();

    size_t capacity() const;

    /// Producer must call this until it succeeds (returns a non-null pointer),
    /// and then write values at the returned address before calling
    /// enqueue_end().
    T *try_enqueue_begin();

    /// Call try_enqueue_begin() until it succeeds. The caller should be sure
    /// that a consumer is active, to avoid the fifo being permanently blocked.
    T *enqueue_begin_wait();

    void enqueue_end();

    /// Consumer must call this until it succeeds (returns a non-null pointer),
    /// and then read data from the returned address, before calling
    /// dequeue_end().
    const T *try_dequeue_begin();

    /// Call try_dequeue_begin() until it succeeds. The caller should be sure
    /// that a producer will produce an element.
    const T *dequeue_begin_wait();

    void dequeue_end();

protected:
    const size_t _capacity;

    T *const _buffer;

    size_t _producer_next_idx;
    std::atomic<size_t> _producer_num_produced;

    size_t _consumer_next_idx;
    std::atomic<size_t> _consumer_num_consumed;
};

/// Similar semantics as concurrent_fifo_spsc, but entries are pre-allocated
/// flat arrays of some runtime-determined size. Can be used to reduce
/// contention on the fifo (en/dequeue chunks) or to operate on logical units.
template<typename T> class concurrent_buffer_fifo_spsc
{
public:
    concurrent_buffer_fifo_spsc() = delete;
    concurrent_buffer_fifo_spsc(const concurrent_buffer_fifo_spsc &) = delete;
    concurrent_buffer_fifo_spsc &operator=(
        const concurrent_buffer_fifo_spsc &) = delete;

    concurrent_buffer_fifo_spsc(
        size_t num_buffers, size_t num_entries_per_buffer);
    ~concurrent_buffer_fifo_spsc();

    /// Similar to concurrent_fifo_spsc::try_enqueue_begin(), except that the
    /// returned value points to an array of num_entries_per_buffer.
    T *try_enqueue_begin();

    T *enqueue_begin_wait();

    void enqueue_end();

    /// Similar to concurrent_fifo_spsc::try_dequeue_begin(), except that the
    /// returned value points to an array of num_entries_per_buffer.
    const T *try_dequeue_begin();

    const T *dequeue_begin_wait();

    void dequeue_end();

protected:
    T *enqueue_next_buffer(T **const buffer_ptr);

    concurrent_fifo_spsc<T *> _queue;
    const size_t _entries_per_buffer;
    T **const _buffers;
    size_t _next_buffer_idx;
};

} // namespace libff

#include "libff/common/concurrent_fifo.tcc"

#endif // __LIBFF_COMMON_CONCURRENT_FIFO_HPP__
