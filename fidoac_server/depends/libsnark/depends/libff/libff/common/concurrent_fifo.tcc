/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef __LIBFF_COMMON_CONCURRENT_FIFO_TCC__
#define __LIBFF_COMMON_CONCURRENT_FIFO_TCC__

#include "libff/common/concurrent_fifo.hpp"

#include <assert.h>
#include <stdlib.h>
#include <thread>

namespace libff
{

// concurrent_fifo_spsc

template<typename T>
concurrent_fifo_spsc<T>::concurrent_fifo_spsc(size_t capacity)
    : _capacity(capacity)
    , _buffer((T *)malloc(capacity * sizeof(T)))
    , _producer_next_idx(0)
    , _producer_num_produced(0)
    , _consumer_next_idx(0)
    , _consumer_num_consumed(0)
{
}

template<typename T> concurrent_fifo_spsc<T>::~concurrent_fifo_spsc()
{
    free(_buffer);
}

template<typename T> size_t concurrent_fifo_spsc<T>::capacity() const
{
    return _capacity;
}

template<typename T> T *concurrent_fifo_spsc<T>::try_enqueue_begin()
{
    const size_t consumer_num_consumed =
        _consumer_num_consumed.load(std::memory_order_relaxed);
    const size_t producer_num_produced =
        _producer_num_produced.load(std::memory_order_relaxed);
    const size_t num_empty_slots =
        consumer_num_consumed + _capacity - producer_num_produced;
    // TODO: handle overflow
    assert(num_empty_slots <= _capacity);

    if (num_empty_slots > 0) {
        return &_buffer[_producer_next_idx];
    }

    return nullptr;
}

template<typename T> T *concurrent_fifo_spsc<T>::enqueue_begin_wait()
{
    T *v = try_enqueue_begin();
    while (!v) {
        std::this_thread::yield();
        v = try_enqueue_begin();
    }
    return v;
}

template<typename T> void concurrent_fifo_spsc<T>::enqueue_end()
{
    // Caller is expected to only call this if try_enqueue_begin succeeded. No
    // need to check consumer state.

    _producer_next_idx = (_producer_next_idx + 1) % _capacity;
    const size_t producer_num_produced =
        _producer_num_produced.load(std::memory_order_relaxed);
    _producer_num_produced.store(
        producer_num_produced + 1, std::memory_order_release);
}

template<typename T> const T *concurrent_fifo_spsc<T>::try_dequeue_begin()
{
    const size_t producer_num_produced =
        _producer_num_produced.load(std::memory_order_relaxed);
    const size_t consumer_num_consumed =
        _consumer_num_consumed.load(std::memory_order_relaxed);
    const size_t num_available = producer_num_produced - consumer_num_consumed;
    assert(num_available <= _capacity);
    // TODO: handle overflow

    if (num_available > 0) {
        return &_buffer[_consumer_next_idx];
    }

    return nullptr;
}

template<typename T> const T *concurrent_fifo_spsc<T>::dequeue_begin_wait()
{
    const T *v = try_dequeue_begin();
    while (!v) {
        std::this_thread::yield();
        v = try_dequeue_begin();
    }
    return v;
}

template<typename T> void concurrent_fifo_spsc<T>::dequeue_end()
{
    // Caller must only call if try_dequeue_begin succeeded. No need to check
    // producer state.

    _consumer_next_idx = (_consumer_next_idx + 1) % _capacity;
    const size_t consumer_num_consumed =
        _consumer_num_consumed.load(std::memory_order_relaxed);
    _consumer_num_consumed.store(
        consumer_num_consumed + 1, std::memory_order_release);
}

// concurrent_buffer_fifo_spsc

template<typename T>
concurrent_buffer_fifo_spsc<T>::concurrent_buffer_fifo_spsc(
    size_t num_buffers, size_t num_entries_per_buffer)
    : _queue(num_buffers)
    , _entries_per_buffer(num_entries_per_buffer)
    , _buffers((T **)malloc(num_buffers * sizeof(T *)))
    , _next_buffer_idx(0)
{
    for (size_t i = 0; i < num_buffers; ++i) {
        _buffers[i] = (T *)malloc(num_entries_per_buffer * sizeof(T));
    }
}

template<typename T>
concurrent_buffer_fifo_spsc<T>::~concurrent_buffer_fifo_spsc()
{
    const size_t num_buffers = _queue.capacity();
    // Free all buffers, then the list of pointers.
    for (size_t i = 0; i < num_buffers; ++i) {
        free(_buffers[i]);
    }
    free(_buffers);
}

template<typename T> T *concurrent_buffer_fifo_spsc<T>::try_enqueue_begin()
{
    T **const buffer_ptr = _queue.try_enqueue_begin();
    if (buffer_ptr) {
        return enqueue_next_buffer(buffer_ptr);
    }
    return nullptr;
}

template<typename T> T *concurrent_buffer_fifo_spsc<T>::enqueue_begin_wait()
{
    return enqueue_next_buffer(_queue.enqueue_begin_wait());
}

template<typename T> void concurrent_buffer_fifo_spsc<T>::enqueue_end()
{
    // The buffer location has already been written into the queue, ready for
    // dequeuing. Nothing to do except call the underlying queue.
    _queue.enqueue_end();
}

template<typename T>
const T *concurrent_buffer_fifo_spsc<T>::try_dequeue_begin()
{
    T *const *const buffer_ptr = _queue.try_dequeue_begin();
    if (buffer_ptr) {
        return *buffer_ptr;
    }
    return nullptr;
}

template<typename T>
const T *concurrent_buffer_fifo_spsc<T>::dequeue_begin_wait()
{
    T *const *const buffer_ptr = _queue.dequeue_begin_wait();
    return *buffer_ptr;
}

template<typename T> void concurrent_buffer_fifo_spsc<T>::dequeue_end()
{
    _queue.dequeue_end();
}

template<typename T>
T *concurrent_buffer_fifo_spsc<T>::enqueue_next_buffer(T **const buffer_ptr)
{
    T *const buffer = _buffers[_next_buffer_idx];
    _next_buffer_idx = (_next_buffer_idx + 1) % _queue.capacity();

    *buffer_ptr = buffer;
    return buffer;
}

} // namespace libff

#endif // __LIBFF_COMMON_CONCURRENT_FIFO_TCC__
