/** @file
 *****************************************************************************
 Implementation of arithmetic in the finite field F[p^2].
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef FP2_HPP_
#define FP2_HPP_
#include <libff/algebra/fields/fp.hpp>
#include <vector>

namespace libff
{

template<mp_size_t n, const bigint<n> &modulus> class Fp2_model;

template<mp_size_t n, const bigint<n> &modulus>
std::ostream &operator<<(std::ostream &, const Fp2_model<n, modulus> &);

template<mp_size_t n, const bigint<n> &modulus>
std::istream &operator>>(std::istream &, Fp2_model<n, modulus> &);

/// Arithmetic in the field F[p^2].
///
/// Let p := modulus. This interface provides arithmetic for the extension
/// field. Fp2 = Fp[U]/(U^2-non_residue), where non_residue is in Fp.
///
/// ASSUMPTION: p = 1 (mod 6)
template<mp_size_t n, const bigint<n> &modulus> class Fp2_model
{
public:
    typedef Fp_model<n, modulus> my_Fp;

    // Exposing the field extension degree via a static member
    // allows to retrieve the value from the type. This can be useful.
    // Note that, the degree can be retrieved from a Fp2_model value
    // by invoking the `size()` method on the `coeffs` container.
    static const size_t tower_extension_degree = 2;

    static void static_init();

    /// (modulus^2-1)/2
    static bigint<2 * n> euler;
    /// modulus^2 = 2^s * t + 1
    static size_t s;
    /// with t odd
    static bigint<2 * n> t;
    /// (t-1)/2
    static bigint<2 * n> t_minus_1_over_2;
    /// X^4-non_residue irreducible over Fp; used for constructing Fp2 = Fp[X] /
    /// (X^2 - non_residue)
    static my_Fp non_residue;
    /// a quadratic nonresidue in Fp2
    static Fp2_model<n, modulus> nqr;
    /// nqr^t
    static Fp2_model<n, modulus> nqr_to_t;
    /// non_residue^((modulus^i-1)/2) for i=0,1
    static my_Fp Frobenius_coeffs_c1[2];

    my_Fp coeffs[2];
    Fp2_model(){};
    // Fp2_model(const my_Fp& c0, const my_Fp& c1) : coeffs(c0, c1) {};
    Fp2_model(const my_Fp &c0, const my_Fp &c1)
    {
        this->coeffs[0] = c0;
        this->coeffs[1] = c1;
        return;
    };

    void clear()
    {
        coeffs[0].clear();
        coeffs[1].clear();
    }
    void print() const
    {
        printf("c0/c1:\n");
        coeffs[0].print();
        coeffs[1].print();
    }

    static const Fp2_model<n, modulus> &zero();
    static const Fp2_model<n, modulus> &one();
    static Fp2_model<n, modulus> random_element();

    bool is_zero() const { return coeffs[0].is_zero() && coeffs[1].is_zero(); }
    bool operator==(const Fp2_model &other) const;
    bool operator!=(const Fp2_model &other) const;

    Fp2_model operator+(const Fp2_model &other) const;
    Fp2_model operator-(const Fp2_model &other) const;
    Fp2_model operator*(const Fp2_model &other) const;
    Fp2_model operator-() const;
    /// default is squared_complex
    Fp2_model squared() const;
    Fp2_model inverse() const;
    Fp2_model Frobenius_map(unsigned long power) const;
    /// HAS TO BE A SQUARE (else does not terminate)
    Fp2_model sqrt() const;
    Fp2_model squared_karatsuba() const;
    Fp2_model squared_complex() const;

    template<mp_size_t m> Fp2_model operator^(const bigint<m> &other) const;

    static size_t size_in_bits() { return 2 * my_Fp::size_in_bits(); }
    static bigint<n> base_field_char() { return modulus; }
    static constexpr size_t extension_degree() { return 2; }

protected:
    static bool s_initialized;
    static Fp2_model<n, modulus> s_zero;
    static Fp2_model<n, modulus> s_one;

    friend std::ostream &operator<<<n, modulus>(
        std::ostream &out, const Fp2_model<n, modulus> &el);
    friend std::istream &operator>>
        <n, modulus>(std::istream &in, Fp2_model<n, modulus> &el);
};

template<mp_size_t n, const bigint<n> &modulus>
std::ostream &operator<<(
    std::ostream &out, const std::vector<Fp2_model<n, modulus>> &v);

template<mp_size_t n, const bigint<n> &modulus>
std::istream &operator>>(
    std::istream &in, std::vector<Fp2_model<n, modulus>> &v);

template<mp_size_t n, const bigint<n> &modulus>
Fp2_model<n, modulus> operator*(
    const Fp_model<n, modulus> &lhs, const Fp2_model<n, modulus> &rhs);

template<mp_size_t n, const bigint<n> &modulus>
bigint<2 * n> Fp2_model<n, modulus>::euler;

template<mp_size_t n, const bigint<n> &modulus> size_t Fp2_model<n, modulus>::s;

template<mp_size_t n, const bigint<n> &modulus>
bigint<2 * n> Fp2_model<n, modulus>::t;

template<mp_size_t n, const bigint<n> &modulus>
bigint<2 * n> Fp2_model<n, modulus>::t_minus_1_over_2;

template<mp_size_t n, const bigint<n> &modulus>
Fp_model<n, modulus> Fp2_model<n, modulus>::non_residue;

template<mp_size_t n, const bigint<n> &modulus>
Fp2_model<n, modulus> Fp2_model<n, modulus>::nqr;

template<mp_size_t n, const bigint<n> &modulus>
Fp2_model<n, modulus> Fp2_model<n, modulus>::nqr_to_t;

template<mp_size_t n, const bigint<n> &modulus>
Fp_model<n, modulus> Fp2_model<n, modulus>::Frobenius_coeffs_c1[2];

} // namespace libff
#include <libff/algebra/fields/fp2.tcc>

#endif // FP2_HPP_
