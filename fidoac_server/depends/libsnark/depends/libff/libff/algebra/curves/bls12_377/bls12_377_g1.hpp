/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef BLS12_377_G1_HPP_
#define BLS12_377_G1_HPP_
#include <libff/algebra/curves/bls12_377/bls12_377_init.hpp>
#include <libff/algebra/curves/curve_utils.hpp>
#include <vector>

namespace libff
{

class bls12_377_G1;
std::ostream &operator<<(std::ostream &, const bls12_377_G1 &);
std::istream &operator>>(std::istream &, bls12_377_G1 &);

class bls12_377_G1
{
public:
#ifdef PROFILE_OP_COUNTS
    static long long add_cnt;
    static long long dbl_cnt;
#endif
    static std::vector<size_t> wnaf_window_table;
    static std::vector<size_t> fixed_base_exp_window_table;
    static bls12_377_G1 G1_zero;
    static bls12_377_G1 G1_one;
    static bls12_377_Fq coeff_a;
    static bls12_377_Fq coeff_b;

    typedef bls12_377_Fq base_field;
    typedef bls12_377_Fr scalar_field;

    // Cofactor
    static const mp_size_t h_bitcount = 125;
    static const mp_size_t h_limbs =
        (h_bitcount + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS;
    static bigint<h_limbs> h;

    bls12_377_Fq X, Y, Z;

    // using Jacobian coordinates
    bls12_377_G1();
    bls12_377_G1(
        const bls12_377_Fq &X, const bls12_377_Fq &Y, const bls12_377_Fq &Z)
        : X(X), Y(Y), Z(Z){};

    void print() const;
    void print_coordinates() const;

    void to_affine_coordinates();
    void to_special();
    bool is_special() const;

    bool is_zero() const;

    bool operator==(const bls12_377_G1 &other) const;
    bool operator!=(const bls12_377_G1 &other) const;

    bls12_377_G1 operator+(const bls12_377_G1 &other) const;
    bls12_377_G1 operator-() const;
    bls12_377_G1 operator-(const bls12_377_G1 &other) const;

    bls12_377_G1 add(const bls12_377_G1 &other) const;
    bls12_377_G1 mixed_add(const bls12_377_G1 &other) const;
    bls12_377_G1 dbl() const;

    // Multiply point by h, the cofactor, to eliminate the h-torsion components.
    bls12_377_G1 mul_by_cofactor() const;

    // Endomorphism (x, y) -> (\beta * x, y) for \beta an element of Fq with
    // order 3.
    bls12_377_G1 sigma() const;

    bool is_well_formed() const;
    bool is_in_safe_subgroup() const;

    // For P (this), return a point P' on the curve such that
    // P'.mul_by_cofactor() == P. In some contexts, this is useful to show that
    // P is in the safe subgroup G1, requiring only a small scalar
    // multiplication by the verifier. Note that (in spite of the type) the
    // point returned here is OUTSIDE of G1.
    bls12_377_G1 proof_of_safe_subgroup() const;

    static const bls12_377_G1 &zero();
    static const bls12_377_G1 &one();
    static bls12_377_G1 random_element();

    static size_t size_in_bits() { return base_field::size_in_bits() + 1; }
    static bigint<base_field::num_limbs> base_field_char()
    {
        return base_field::field_char();
    }
    static bigint<scalar_field::num_limbs> order()
    {
        return scalar_field::field_char();
    }

    void write_uncompressed(std::ostream &) const;
    void write_compressed(std::ostream &) const;
    static void read_uncompressed(std::istream &, bls12_377_G1 &);
    static void read_compressed(std::istream &, bls12_377_G1 &);

    static void batch_to_special_all_non_zeros(std::vector<bls12_377_G1> &vec);
};

template<mp_size_t m>
bls12_377_G1 operator*(const bigint<m> &lhs, const bls12_377_G1 &rhs)
{
    return scalar_mul<bls12_377_G1, m>(rhs, lhs);
}

template<mp_size_t m, const bigint<m> &modulus_p>
bls12_377_G1 operator*(
    const Fp_model<m, modulus_p> &lhs, const bls12_377_G1 &rhs)
{
    return scalar_mul<bls12_377_G1, m>(rhs, lhs.as_bigint());
}

} // namespace libff

#endif // BLS12_377_G1_HPP_
