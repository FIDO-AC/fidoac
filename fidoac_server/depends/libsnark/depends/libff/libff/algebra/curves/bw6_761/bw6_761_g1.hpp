#ifndef BW6_761_G1_HPP_
#define BW6_761_G1_HPP_
#include <libff/algebra/curves/bw6_761/bw6_761_init.hpp>
#include <libff/algebra/curves/curve_utils.hpp>
#include <vector>

namespace libff
{

class bw6_761_G1;
std::ostream &operator<<(std::ostream &, const bw6_761_G1 &);
std::istream &operator>>(std::istream &, bw6_761_G1 &);

class bw6_761_G1
{
public:
#ifdef PROFILE_OP_COUNTS
    static long long add_cnt;
    static long long dbl_cnt;
#endif
    static std::vector<size_t> wnaf_window_table;
    static std::vector<size_t> fixed_base_exp_window_table;
    static bw6_761_G1 G1_zero;
    static bw6_761_G1 G1_one;
    static bw6_761_Fq coeff_a;
    static bw6_761_Fq coeff_b;

    typedef bw6_761_Fq base_field;
    typedef bw6_761_Fr scalar_field;

    // Cofactor
    static const mp_size_t h_bitcount = 384;
    static const mp_size_t h_limbs =
        (h_bitcount + GMP_NUMB_BITS - 1) / GMP_NUMB_BITS;
    static bigint<h_limbs> h;

    bw6_761_Fq X, Y, Z;

    // using projective coordinates
    bw6_761_G1();
    bw6_761_G1(const bw6_761_Fq &X, const bw6_761_Fq &Y, const bw6_761_Fq &Z)
        : X(X), Y(Y), Z(Z){};

    void print() const;
    void print_coordinates() const;

    void to_affine_coordinates();
    void to_special();
    bool is_special() const;

    bool is_zero() const;

    bool operator==(const bw6_761_G1 &other) const;
    bool operator!=(const bw6_761_G1 &other) const;

    bw6_761_G1 operator+(const bw6_761_G1 &other) const;
    bw6_761_G1 operator-() const;
    bw6_761_G1 operator-(const bw6_761_G1 &other) const;

    bw6_761_G1 add(const bw6_761_G1 &other) const;
    bw6_761_G1 mixed_add(const bw6_761_G1 &other) const;
    bw6_761_G1 dbl() const;
    bw6_761_G1 mul_by_cofactor() const;

    bool is_well_formed() const;
    bool is_in_safe_subgroup() const;

    static const bw6_761_G1 &zero();
    static const bw6_761_G1 &one();
    static bw6_761_G1 random_element();

    static size_t size_in_bits() { return base_field::size_in_bits() + 1; }
    static bigint<base_field::num_limbs> base_field_char()
    {
        return base_field::field_char();
    }
    static bigint<scalar_field::num_limbs> order()
    {
        return scalar_field::field_char();
    }

    friend std::ostream &operator<<(std::ostream &out, const bw6_761_G1 &g);
    friend std::istream &operator>>(std::istream &in, bw6_761_G1 &g);

    void write_uncompressed(std::ostream &) const;
    void write_compressed(std::ostream &) const;
    static void read_uncompressed(std::istream &, bw6_761_G1 &);
    static void read_compressed(std::istream &, bw6_761_G1 &);

    static void batch_to_special_all_non_zeros(std::vector<bw6_761_G1> &vec);
};

template<mp_size_t m>
bw6_761_G1 operator*(const bigint<m> &lhs, const bw6_761_G1 &rhs)
{
    return scalar_mul<bw6_761_G1, m>(rhs, lhs);
}

template<mp_size_t m, const bigint<m> &modulus_p>
bw6_761_G1 operator*(const Fp_model<m, modulus_p> &lhs, const bw6_761_G1 &rhs)
{
    return scalar_mul<bw6_761_G1, m>(rhs, lhs.as_bigint());
}

} // namespace libff

#endif // BW6_761_G1_HPP_
