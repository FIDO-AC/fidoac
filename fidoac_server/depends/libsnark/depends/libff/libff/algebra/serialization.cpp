/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include "libff/algebra/serialization.hpp"

#include <assert.h>
#include <stdexcept>

namespace libff
{

// Converts a single character to a nibble. Throws std::invalid_argument if the
// character is not hex.
uint8_t char_to_nibble(const char c)
{
    const char cc = (char)std::tolower(c);
    if (cc < '0') {
        throw std::invalid_argument("invalid hex character");
    }
    if (cc <= '9') {
        return cc - '0';
    }
    if (cc < 'a') {
        throw std::invalid_argument("invalid hex character");
    }
    if (cc <= 'f') {
        return cc - 'a' + 10;
    }
    throw std::invalid_argument("invalid hex character");
}

static uint8_t chars_to_byte(const char *cs)
{
    const uint8_t *data = (const uint8_t *)cs;
    return (char_to_nibble(data[0]) << 4) | char_to_nibble(data[1]);
}

static char nibble_hex(const uint8_t nibble)
{
    assert((nibble & 0xf0) == 0);
    if (nibble > 9) {
        return (char)('a' + nibble - 10);
    }

    return (char)('0' + nibble);
}

/// Return a pointer to the beginning of the actual hex characters (removing
/// any `0x` prefix), and ensure that the length is as expected.
static const char *find_hex_string_of_length(
    const std::string &hex, const size_t bytes)
{
    if ('0' == hex[0] && 'x' == hex[1]) {
        if (hex.size() != 2 + bytes * 2) {
            throw std::invalid_argument("invalid hex length");
        }
        return hex.c_str() + 2;
    }

    if (hex.size() != bytes * 2) {
        throw std::invalid_argument("invalid hex length");
    }

    return hex.c_str();
}

void hex_to_bytes_reversed(const std::string &hex, void *dest, size_t bytes)
{
    if (bytes == 0) {
        return;
    }
    const char *cur = find_hex_string_of_length(hex, bytes);
    uint8_t *const dest_bytes_end = (uint8_t *)dest;
    uint8_t *dest_bytes = dest_bytes_end + bytes;
    do {
        --dest_bytes;
        *dest_bytes = chars_to_byte(cur);
        cur += 2;
    } while (dest_bytes > dest_bytes_end);
}

std::string bytes_to_hex_reversed(
    const void *bytes, size_t num_bytes, bool prefix)
{
    if (num_bytes == 0) {
        return "";
    }

    std::string out;
    if (prefix) {
        out.reserve(num_bytes * 2 + 2);
        out.push_back('0');
        out.push_back('x');
    } else {
        out.reserve(num_bytes * 2);
    }

    const uint8_t *const src_bytes_end = (const uint8_t *)bytes;
    const uint8_t *src_bytes = src_bytes_end + num_bytes;
    do {
        --src_bytes;
        const uint8_t byte = *src_bytes;
        out.push_back(nibble_hex(byte >> 4));
        out.push_back(nibble_hex(byte & 0x0f));
    } while (src_bytes > src_bytes_end);

    return out;
}

} // namespace libff
