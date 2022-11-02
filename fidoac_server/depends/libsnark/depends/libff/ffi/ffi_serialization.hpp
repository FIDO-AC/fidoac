#ifndef __LIBFF_FFI_FFI_SERIALIZATION_HPP__
#define __LIBFF_FFI_FFI_SERIALIZATION_HPP__

#include <libff/algebra/curves/public_params.hpp>

namespace libff
{

namespace ffi
{

// FFI buffers are expected to be big-endian formatted, and padded on the left
// to be the same size as the in-memory bigint representations. Coefficients of
// field elements, and affine coordinates of curve points are placed
// consecutively in memory (where coefficients appear highest-order first, and
// coordinates appear X followed by Y).

// TODO: Since use the in-memory object sizes (and enforce that the
// passed-in buffer sizes match exactly), the required sizes could
// potentially vary based on the platform (depending on the limb types
// and number).  This could be addressed by explicitly specifying a
// serialized size for each object.

// TODO: Make the group read operations check the validity of the
// curve point.

/// Check the buffer size and copy in reverse-byte order.  Returns
/// true on success.
template<typename T>
bool object_read_from_buffer(T &object, const void *buffer, size_t buffer_size);

/// Check the buffer size and write in reverse-byte order.  Returns
/// true on success.
template<typename T>
bool object_write_to_buffer(const T &object, void *buffer, size_t buffer_size);

/// Read a field element, checking that the buffer size is as
/// expected.  Returns true on success.
template<typename FieldT>
bool field_element_read(FieldT &f, const void *buffer, size_t buffer_size);

/// Write a field element to a buffer, checking that the size is as
/// expected.  Returns true on success.
template<typename FieldT>
bool field_element_write(const FieldT &f, void *buffer, size_t buffer_size);

/// Read a group element (in affine form), checking that the buffer size is as
/// expected. Returns true on success.
template<typename GroupT>
bool group_element_read(GroupT &g, const void *buffer, size_t buffer_size);

/// Write a group element to a buffer (in affine form), checking that the size
/// is as expected. Returns true on success.
template<typename GroupT>
bool group_element_write(
    const GroupT &g, const void *buffer, size_t buffer_size);

} // namespace ffi

} // namespace libff

#include "ffi_serialization.tcc"

#endif // __LIBFF_FFI_FFI_SERIALIZATION_HPP__
