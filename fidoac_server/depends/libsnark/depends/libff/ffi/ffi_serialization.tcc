#ifndef __LIBFF_FFI_FFI_SERIALIZATION_TCC__
#define __LIBFF_FFI_FFI_SERIALIZATION_TCC__

#include "ffi_serialization.hpp"

#include <libff/algebra/fields/fp.hpp>

namespace libff
{

namespace ffi
{

namespace internal
{

// Generic big-endian field_serializer for multi-component field
// elements.
template<typename FieldT> class field_serializer
{
public:
    static bool read_from_buffer(
        FieldT &f, const void *buffer, size_t buffer_size)
    {
        constexpr size_t component_size = sizeof(f.coeffs[0]);
        size_t i = FieldT::extension_degree() - 1;
        do {
            if (buffer_size < component_size ||
                !field_element_read(f.coeffs[i], buffer, component_size)) {
                return false;
            }
            buffer = ((const char *)buffer) + component_size;
            buffer_size -= component_size;
        } while (i-- > 0);

        return true;
    }

    static bool write_to_buffer(
        const FieldT &f, void *buffer, size_t buffer_size)
    {
        constexpr size_t component_size = sizeof(f.coeffs[0]);
        size_t i = FieldT::extension_degree() - 1;
        do {
            if (buffer_size < component_size ||
                !field_element_write(f.coeffs[i], buffer, component_size)) {
                return false;
            }
            buffer = ((char *)buffer) + component_size;
            buffer_size -= component_size;
        } while (i-- > 0);

        return true;
    }
};

// Specialized version of field_serializer, for the base-case of base
// fields.
template<mp_size_t n, const libff::bigint<n> &modulus>
class field_serializer<libff::Fp_model<n, modulus>>
{
public:
    using FieldT = libff::Fp_model<n, modulus>;

    static bool read_from_buffer(
        FieldT &f, const void *buffer, size_t buffer_size)
    {
        libff::bigint<n> bigint;
        if (object_read_from_buffer(bigint, buffer, buffer_size) &&
            mpn_cmp(modulus.data, bigint.data, n) > 0) {
            f = libff::Fp_model<n, modulus>(bigint);
            return true;
        }

        return false;
    }

    static bool write_to_buffer(
        const FieldT &f, void *buffer, size_t buffer_size)
    {
        const libff::bigint<n> bigint = f.as_bigint();
        return object_write_to_buffer(bigint, buffer, buffer_size);
    }
};

template<typename FieldT> bool field_element_equals_zero(const FieldT &f)
{
    return f == FieldT::zero();
}

template<typename FieldT> bool field_element_equals_one(const FieldT &f)
{
    return f == FieldT::one();
}

} // namespace internal

template<typename T>
bool object_read_from_buffer(T &object, const void *buffer, size_t buffer_size)
{
    constexpr size_t object_size = sizeof(T);
    if (buffer_size != object_size) {
        return false;
    }

    char *dest = (char *)&object;
    const char *src = ((const char *)buffer) + buffer_size;
    const char *end = src - object_size;
    while (src > end) {
        *(dest++) = *(--src);
    }

    return true;
}

// Ensure there is enough data in the buffer, copy in reverse-byte
// order from the right of the buffer into an array of element_size.
template<typename T>
bool object_write_to_buffer(const T &object, void *buffer, size_t buffer_size)
{
    constexpr size_t object_size = sizeof(T);
    if (buffer_size != object_size) {
        return false;
    }

    char *dest = (char *)buffer;
    const char *const end = ((const char *)&object);
    const char *src = end + object_size;
    while (src > end) {
        *(dest++) = *(--src);
    }

    return true;
}

template<typename FieldT>
bool field_element_read(FieldT &f, const void *buffer, size_t buffer_size)
{
    return internal::field_serializer<FieldT>::read_from_buffer(
        f, buffer, buffer_size);
}

template<typename FieldT>
bool field_element_write(const FieldT &f, void *buffer, size_t buffer_size)
{
    return internal::field_serializer<FieldT>::write_to_buffer(
        f, buffer, buffer_size);
}

template<typename GroupT>
bool group_element_read(GroupT &g, const void *buffer, size_t buffer_size)
{
    constexpr size_t coordinate_size = sizeof(g.X);
    if (buffer_size == 2 * coordinate_size) {
        if (field_element_read(g.X, buffer, coordinate_size)) {
            buffer = ((const char *)buffer) + coordinate_size;
            if (field_element_read(g.Y, buffer, coordinate_size)) {
                if (internal::field_element_equals_zero(g.X) &&
                    internal::field_element_equals_one(g.Y)) {
                    g.Z = g.Z.zero();
                } else {
                    g.Z = g.Z.one();
                }

                return g.is_well_formed() && g.is_in_safe_subgroup();
            }
        }
    }

    return false;
}

template<typename GroupT>
bool group_element_write(const GroupT &g, void *buffer, size_t buffer_size)
{
    constexpr size_t coordinate_size = sizeof(g.X);
    if (buffer_size == 2 * coordinate_size) {
        GroupT affine_p = g;
        affine_p.to_affine_coordinates();
        if (field_element_write(affine_p.X, buffer, coordinate_size)) {
            buffer = ((char *)buffer) + coordinate_size;
            return field_element_write(affine_p.Y, buffer, coordinate_size);
        }
    }

    return false;
}

} // namespace ffi

} // namespace libff

#endif // __LIBFF_FFI_FFI_SERIALIZATION_HPP__
