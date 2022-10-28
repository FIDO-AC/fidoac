/**
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <gtest/gtest.h>
#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>
#include <libff/algebra/curves/edwards/edwards_pp.hpp>
#include <libff/algebra/curves/mnt/mnt4/mnt4_pp.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_pp.hpp>
#include <libff/algebra/fields/field_serialization.hpp>
#include <libff/algebra/fields/field_utils.hpp>
#include <libff/common/profiling.hpp>
#ifdef CURVE_BN128
#include <libff/algebra/curves/bn128/bn128_pp.hpp>
#endif
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libff/algebra/fields/fp12_2over3over2.hpp>
#include <libff/algebra/fields/fp6_3over2.hpp>

using namespace libff;

template<typename FieldT> void test_field()
{
    bigint<1> rand1 = bigint<1>("76749407");
    bigint<1> rand2 = bigint<1>("44410867");
    bigint<1> randsum = bigint<1>("121160274");

    FieldT zero = FieldT::zero();
    FieldT one = FieldT::one();
    FieldT a = FieldT::random_element();
    FieldT a_ser;
    a_ser = reserialize<FieldT>(a);
    ASSERT_EQ(a_ser, a);

    FieldT b = FieldT::random_element();
    FieldT c = FieldT::random_element();
    FieldT d = FieldT::random_element();

    ASSERT_NE(a, zero);
    ASSERT_NE(a, one);

    ASSERT_EQ(a * zero, zero);
    ASSERT_EQ(a + zero, a);
    ASSERT_EQ(a * one, a);

    ASSERT_EQ(a * a, a.squared());
    ASSERT_EQ(a * a, a.squared());
    ASSERT_EQ((a + b).squared(), a.squared() + a * b + b * a + b.squared());
    ASSERT_EQ((a + b) * (c + d), a * c + a * d + b * c + b * d);
    ASSERT_EQ(a - b, a + (-b));
    ASSERT_EQ(a - b, (-b) + a);

    ASSERT_EQ((a ^ rand1) * (a ^ rand2), (a ^ randsum));

    ASSERT_EQ(a * a.inverse(), one);
    ASSERT_EQ(
        (a + b) * c.inverse(), a * c.inverse() + (b.inverse() * c).inverse());
}

template<typename FieldT> void test_sqrt()
{
    for (size_t i = 0; i < 100; ++i) {
        FieldT a = FieldT::random_element();
        FieldT asq = a.squared();
        ASSERT_TRUE(asq.sqrt() == a || asq.sqrt() == -a);
    }
}

template<typename FieldT> void test_two_squarings()
{
    FieldT a = FieldT::random_element();
    ASSERT_EQ(a.squared(), a * a);
    ASSERT_EQ(a.squared(), a.squared_complex());
    ASSERT_EQ(a.squared(), a.squared_karatsuba());
}

template<typename FieldT> void test_Frobenius()
{
    FieldT a = FieldT::random_element();
    ASSERT_EQ(a.Frobenius_map(0), a);
    FieldT a_q = a ^ FieldT::base_field_char();
    for (size_t power = 1; power < 10; ++power) {
        const FieldT a_qi = a.Frobenius_map(power);
        ASSERT_EQ(a_qi, a_q);

        a_q = a_q ^ FieldT::base_field_char();
    }
}

template<typename FieldT> void test_unitary_inverse()
{
    ASSERT_EQ(FieldT::extension_degree() % 2, 0);
    FieldT a = FieldT::random_element();
    FieldT aqcubed_minus1 =
        a.Frobenius_map(FieldT::extension_degree() / 2) * a.inverse();
    ASSERT_EQ(aqcubed_minus1.inverse(), aqcubed_minus1.unitary_inverse());
}

template<typename FieldT> void test_cyclotomic_squaring();

template<> void test_cyclotomic_squaring<Fqk<edwards_pp>>()
{
    typedef Fqk<edwards_pp> FieldT;
    ASSERT_EQ(FieldT::extension_degree() % 2, 0);
    FieldT a = FieldT::random_element();
    FieldT a_unitary =
        a.Frobenius_map(FieldT::extension_degree() / 2) * a.inverse();
    // beta = a^((q^(k/2)-1)*(q+1))
    FieldT beta = a_unitary.Frobenius_map(1) * a_unitary;
    ASSERT_EQ(beta.cyclotomic_squared(), beta.squared());
}

template<> void test_cyclotomic_squaring<Fqk<mnt4_pp>>()
{
    typedef Fqk<mnt4_pp> FieldT;
    ASSERT_EQ(FieldT::extension_degree() % 2, 0);
    FieldT a = FieldT::random_element();
    FieldT a_unitary =
        a.Frobenius_map(FieldT::extension_degree() / 2) * a.inverse();
    // beta = a^(q^(k/2)-1)
    FieldT beta = a_unitary;
    ASSERT_EQ(beta.cyclotomic_squared(), beta.squared());
}

template<> void test_cyclotomic_squaring<Fqk<mnt6_pp>>()
{
    typedef Fqk<mnt6_pp> FieldT;
    ASSERT_EQ(FieldT::extension_degree() % 2, 0);
    FieldT a = FieldT::random_element();
    FieldT a_unitary =
        a.Frobenius_map(FieldT::extension_degree() / 2) * a.inverse();
    // beta = a^((q^(k/2)-1)*(q+1))
    FieldT beta = a_unitary.Frobenius_map(1) * a_unitary;
    ASSERT_EQ(beta.cyclotomic_squared(), beta.squared());
}

template<typename ppT> void test_all_fields()
{
    test_field<Fr<ppT>>();
    test_field<Fq<ppT>>();
    test_field<Fqe<ppT>>();
    test_field<Fqk<ppT>>();

    test_sqrt<Fr<ppT>>();
    test_sqrt<Fq<ppT>>();
    test_sqrt<Fqe<ppT>>();

    test_Frobenius<Fqe<ppT>>();
    test_Frobenius<Fqk<ppT>>();

    test_unitary_inverse<Fqk<ppT>>();
}

template<typename Fp4T> void test_Fp4_tom_cook()
{
    typedef typename Fp4T::my_Fp FieldT;
    for (size_t i = 0; i < 100; ++i) {
        const Fp4T a = Fp4T::random_element();
        const Fp4T b = Fp4T::random_element();
        const Fp4T correct_res = a * b;

        Fp4T res;

        const FieldT &a0 = a.coeffs[0].coeffs[0], &a1 = a.coeffs[1].coeffs[0],
                     &a2 = a.coeffs[0].coeffs[1], &a3 = a.coeffs[1].coeffs[1];

        const FieldT &b0 = b.coeffs[0].coeffs[0], &b1 = b.coeffs[1].coeffs[0],
                     &b2 = b.coeffs[0].coeffs[1], &b3 = b.coeffs[1].coeffs[1];

        FieldT &c0 = res.coeffs[0].coeffs[0], &c1 = res.coeffs[1].coeffs[0],
               &c2 = res.coeffs[0].coeffs[1], &c3 = res.coeffs[1].coeffs[1];

        const FieldT v0 = a0 * b0;
        const FieldT v1 = (a0 + a1 + a2 + a3) * (b0 + b1 + b2 + b3);
        const FieldT v2 = (a0 - a1 + a2 - a3) * (b0 - b1 + b2 - b3);
        const FieldT v3 =
            (a0 + FieldT(2) * a1 + FieldT(4) * a2 + FieldT(8) * a3) *
            (b0 + FieldT(2) * b1 + FieldT(4) * b2 + FieldT(8) * b3);
        const FieldT v4 =
            (a0 - FieldT(2) * a1 + FieldT(4) * a2 - FieldT(8) * a3) *
            (b0 - FieldT(2) * b1 + FieldT(4) * b2 - FieldT(8) * b3);
        const FieldT v5 =
            (a0 + FieldT(3) * a1 + FieldT(9) * a2 + FieldT(27) * a3) *
            (b0 + FieldT(3) * b1 + FieldT(9) * b2 + FieldT(27) * b3);
        const FieldT v6 = a3 * b3;

        const FieldT beta = Fp4T::non_residue;

        c0 = v0 + beta * (FieldT(4).inverse() * v0 -
                          FieldT(6).inverse() * (v1 + v2) +
                          FieldT(24).inverse() * (v3 + v4) - FieldT(5) * v6);
        c1 = -FieldT(3).inverse() * v0 + v1 - FieldT(2).inverse() * v2 -
             FieldT(4).inverse() * v3 + FieldT(20).inverse() * v4 +
             FieldT(30).inverse() * v5 - FieldT(12) * v6 +
             beta * (-FieldT(12).inverse() * (v0 - v1) +
                     FieldT(24).inverse() * (v2 - v3) -
                     FieldT(120).inverse() * (v4 - v5) - FieldT(3) * v6);
        c2 = -(FieldT(5) * (FieldT(4).inverse())) * v0 +
             (FieldT(2) * (FieldT(3).inverse())) * (v1 + v2) -
             FieldT(24).inverse() * (v3 + v4) + FieldT(4) * v6 + beta * v6;
        c3 = FieldT(12).inverse() * (FieldT(5) * v0 - FieldT(7) * v1) -
             FieldT(24).inverse() * (v2 - FieldT(7) * v3 + v4 + v5) +
             FieldT(15) * v6;

        ASSERT_EQ(res, correct_res);

        // {v0, v3, v4, v5}
        const FieldT u = (FieldT::one() - beta).inverse();
        ASSERT_EQ(
            v0,
            u * c0 + beta * u * c2 - beta * u * FieldT(2).inverse() * v1 -
                beta * u * FieldT(2).inverse() * v2 + beta * v6);
        ASSERT_EQ(
            v3,
            -FieldT(15) * u * c0 - FieldT(30) * u * c1 -
                FieldT(3) * (FieldT(4) + beta) * u * c2 -
                FieldT(6) * (FieldT(4) + beta) * u * c3 +
                (FieldT(24) - FieldT(3) * beta * FieldT(2).inverse()) * u * v1 +
                (-FieldT(8) + beta * FieldT(2).inverse()) * u * v2 -
                FieldT(3) * (-FieldT(16) + beta) * v6);
        ASSERT_EQ(
            v4,
            -FieldT(15) * u * c0 + FieldT(30) * u * c1 -
                FieldT(3) * (FieldT(4) + beta) * u * c2 +
                FieldT(6) * (FieldT(4) + beta) * u * c3 +
                (FieldT(24) - FieldT(3) * beta * FieldT(2).inverse()) * u * v2 +
                (-FieldT(8) + beta * FieldT(2).inverse()) * u * v1 -
                FieldT(3) * (-FieldT(16) + beta) * v6);
        ASSERT_EQ(
            v5,
            -FieldT(80) * u * c0 - FieldT(240) * u * c1 -
                FieldT(8) * (FieldT(9) + beta) * u * c2 -
                FieldT(24) * (FieldT(9) + beta) * u * c3 -
                FieldT(2) * (-FieldT(81) + beta) * u * v1 +
                (-FieldT(81) + beta) * u * v2 -
                FieldT(8) * (-FieldT(81) + beta) * v6);

        // c0 + beta c2 - (beta v1)/2 - (beta v2)/ 2 - (-1 + beta) beta v6,
        // -15 c0 - 30 c1 - 3 (4 + beta) c2 - 6 (4 + beta) c3 + (24 - (3
        // beta)/2) v1 + (-8 + beta/2) v2 + 3 (-16 + beta) (-1 + beta) v6, -15
        // c0 + 30 c1 - 3 (4 + beta) c2 + 6 (4 + beta) c3 + (-8 + beta/2) v1 +
        // (24 - (3 beta)/2) v2 + 3 (-16 + beta) (-1 + beta) v6, -80 c0 - 240 c1
        // - 8 (9 + beta) c2 - 24 (9 + beta) c3 - 2 (-81 + beta) v1 + (-81 +
        // beta) v2 + 8 (-81 + beta) (-1 + beta) v6
    }
}

template<typename Fp12T> void test_Fp12_2over3over2_mul_by_024()
{
    using FpT = typename Fp12T::my_Fp;
    using Fp2T = typename Fp12T::my_Fp2;
    using Fp6T = typename Fp12T::my_Fp6;

    // Let z be some Fp12 element, and x be a sparse element. Compute the
    // results of z * x in the long way, and using the mul_by_024 method.

    const Fp12T z(
        Fp6T(
            Fp2T(FpT("5"), FpT("6")),
            Fp2T(FpT("7"), FpT("8")),
            Fp2T(FpT("9"), FpT("10"))),
        Fp6T(
            Fp2T(FpT("21"), FpT("22")),
            Fp2T(FpT("23"), FpT("24")),
            Fp2T(FpT("25"), FpT("26"))));
    const Fp12T x(
        Fp6T(
            Fp2T(FpT("11"), FpT("12")),
            Fp2T::zero(),
            Fp2T(FpT("15"), FpT("16"))),
        Fp6T(Fp2T::zero(), Fp2T(FpT("3"), FpT("4")), Fp2T::zero()));

    const Fp12T result_slow = z * x;
    const Fp12T result_mul_024 = z.mul_by_024(
        x.coeffs[0].coeffs[0], x.coeffs[1].coeffs[1], x.coeffs[0].coeffs[2]);
    ASSERT_EQ(result_slow, result_mul_024);
}

void test_field_get_digit_alt_bn128()
{
    using FieldT = Fr<alt_bn128_pp>;

    {
        // 0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000000
        const auto value = FieldT(-1).as_bigint();

        // 127 -> 0, 126 -> 3, 125 -> 0
        ASSERT_EQ(0, field_get_digit(value, 2, 127));
        ASSERT_EQ(3, field_get_digit(value, 2, 126));
        ASSERT_EQ(0, field_get_digit(value, 2, 125));
    }

    {
        // 0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000000
        const auto value = FieldT(-1).as_bigint();

        ASSERT_EQ(0x3064, field_get_digit(value, 16, 15));
        ASSERT_EQ(0x4e72, field_get_digit(value, 16, 14));
        ASSERT_EQ(0xe131, field_get_digit(value, 16, 13));
        ASSERT_EQ(0xa029, field_get_digit(value, 16, 12));
        ASSERT_EQ(0xb850, field_get_digit(value, 16, 11));
        ASSERT_EQ(0x45b6, field_get_digit(value, 16, 10));
        ASSERT_EQ(0x8181, field_get_digit(value, 16, 9));
        ASSERT_EQ(0x585d, field_get_digit(value, 16, 8));
        ASSERT_EQ(0x2833, field_get_digit(value, 16, 7));
        ASSERT_EQ(0xe848, field_get_digit(value, 16, 6));
        ASSERT_EQ(0x79b9, field_get_digit(value, 16, 5));
        ASSERT_EQ(0x7091, field_get_digit(value, 16, 4));
        ASSERT_EQ(0x43e1, field_get_digit(value, 16, 3));
        ASSERT_EQ(0xf593, field_get_digit(value, 16, 2));
        ASSERT_EQ(0xf000, field_get_digit(value, 16, 1));
        ASSERT_EQ(0x0000, field_get_digit(value, 16, 0));
    }

    {
        // 0x30644e72e131a029b85045b68181585d2833e84879b9709143e1f593f0000000
        const auto value = FieldT(-1).as_bigint();

        ASSERT_EQ(0x003, field_get_digit(value, 12, 21));
        ASSERT_EQ(0x064, field_get_digit(value, 12, 20));
        ASSERT_EQ(0x4e7, field_get_digit(value, 12, 19));
        ASSERT_EQ(0x2e1, field_get_digit(value, 12, 18));
        ASSERT_EQ(0x31a, field_get_digit(value, 12, 17));
        ASSERT_EQ(0x029, field_get_digit(value, 12, 16));
        ASSERT_EQ(0xb85, field_get_digit(value, 12, 15));
        ASSERT_EQ(0x045, field_get_digit(value, 12, 14));
        ASSERT_EQ(0xb68, field_get_digit(value, 12, 13));
        ASSERT_EQ(0x181, field_get_digit(value, 12, 12));
        ASSERT_EQ(0x585, field_get_digit(value, 12, 11));
        ASSERT_EQ(0xd28, field_get_digit(value, 12, 10));
        ASSERT_EQ(0x33e, field_get_digit(value, 12, 9));
        ASSERT_EQ(0x848, field_get_digit(value, 12, 8));
        ASSERT_EQ(0x79b, field_get_digit(value, 12, 7));
        ASSERT_EQ(0x970, field_get_digit(value, 12, 6));
        ASSERT_EQ(0x914, field_get_digit(value, 12, 5));
        ASSERT_EQ(0x3e1, field_get_digit(value, 12, 4));
        ASSERT_EQ(0xf59, field_get_digit(value, 12, 3));
        ASSERT_EQ(0x3f0, field_get_digit(value, 12, 2));
        ASSERT_EQ(0x000, field_get_digit(value, 12, 1));
        ASSERT_EQ(0x000, field_get_digit(value, 12, 0));
    }
}

template<typename FieldT>
void do_test_signed_digits(const FieldT &value, const size_t digit_size)
{
    // The digits should satisfy:
    //   -2^{digit_size - 1} \leq digit \lt 2^{digit_size - 1}.
    const ssize_t digit_max = 1 << (digit_size - 1);
    const ssize_t digit_min = -digit_max;

    // Num digits must include an extra bit for overflow.
    const size_t num_digits =
        (FieldT::num_bits + 1 + digit_size - 1) / digit_size;

    // Compute all digits to a vector, for checking
    std::vector<ssize_t> all_digits(num_digits);
    field_get_signed_digits(all_digits, value, digit_size, num_digits);

    const auto v = value.as_bigint();

    // Compute
    //   \sum_{i=0}^{n-1} 2^{c * i} * d_i
    // where
    //   n = num_digits,
    //   c = digit size and
    //   d_i = i-th signed digit of size c
    FieldT accum = FieldT::zero();
    for (size_t i = 0; i < num_digits; ++i) {
        accum = accum * FieldT((long)1 << digit_size);
        ssize_t digit =
            field_get_signed_digit(v, digit_size, num_digits - 1 - i);
        accum = accum + FieldT((long)digit);

        // Assert digit value range
        ASSERT_LE(digit_min, digit);
        ASSERT_LT(digit, digit_max);

        // Compare to vector
        ASSERT_EQ(all_digits[num_digits - 1 - i], digit);
    }

    ASSERT_EQ(v, accum.as_bigint());
}

template<typename FieldT> void test_signed_digits()
{
    for (size_t i = 2; i < 22; ++i) {
        do_test_signed_digits(FieldT(-1), i);
        do_test_signed_digits(FieldT(-2), i);
    }
}

template<typename FieldT, encoding_t Enc, form_t Form>
void test_field_serialization_config()
{
    const FieldT a = FieldT::random_element();
    const FieldT b = FieldT::random_element();
    const FieldT c = FieldT::random_element();

    std::string buffer;
    {
        std::ostringstream ss;
        field_write<Enc, Form>(a, ss);
        field_write<Enc, Form>(b, ss);
        field_write<Enc, Form>(c, ss);
        buffer = ss.str();
    }

    FieldT a_dec;
    FieldT b_dec;
    FieldT c_dec;
    std::istringstream ss(buffer);
    field_read<Enc, Form>(a_dec, ss);
    field_read<Enc, Form>(b_dec, ss);
    field_read<Enc, Form>(c_dec, ss);

    ASSERT_EQ(a, a_dec);
    ASSERT_EQ(b, b_dec);
    ASSERT_EQ(c, c_dec);
}

template<typename FieldT> void test_field_serialization_all_configs()
{
    test_field_serialization_config<FieldT, encoding_json, form_plain>();
    test_field_serialization_config<FieldT, encoding_json, form_montgomery>();
    test_field_serialization_config<FieldT, encoding_binary, form_plain>();
    test_field_serialization_config<FieldT, encoding_binary, form_montgomery>();
}

template<typename ppT> void test_serialization()
{
    test_field_serialization_all_configs<Fr<ppT>>();
    test_field_serialization_all_configs<Fq<ppT>>();
    test_field_serialization_all_configs<Fqe<ppT>>();
    test_field_serialization_all_configs<Fqk<ppT>>();
}

TEST(FieldsTest, BigInt)
{
    const std::string a_str("0");
    const std::string b_str("1234567890123456789012345678901234567890");
    const std::string c_str("1");
    const std::string d_str("1000000000000000000000000000000000000000");

    const std::string a_hex(
        "0000000000000000000000000000000000000000000000000000000000000000");
    const std::string b_hex(
        "00000000000000000000000000000003a0c92075c0dbf3b8acbc5f96ce3f0ad2");
    const std::string c_hex(
        "0000000000000000000000000000000000000000000000000000000000000001");
    const std::string d_hex(
        "00000000000000000000000000000002f050fe938943acc45f65568000000000");

    const std::string a_hex_p(
        "0x0000000000000000000000000000000000000000000000000000000000000000");
    const std::string b_hex_p(
        "0x00000000000000000000000000000003a0c92075c0dbf3b8acbc5f96ce3f0ad2");
    const std::string c_hex_p(
        "0x0000000000000000000000000000000000000000000000000000000000000001");
    const std::string d_hex_p(
        "0x00000000000000000000000000000002f050fe938943acc45f65568000000000");

    bigint<4> a(a_str.c_str());
    bigint<4> b(b_str.c_str());
    bigint<4> c(c_str.c_str());
    bigint<4> d(d_str.c_str());
    bigint<4> a_2;
    bigint<4> b_2;
    bigint<4> c_2;
    bigint<4> d_2;

    // Decimal serialization
    ASSERT_EQ(a_str, bigint_to_dec(a));
    ASSERT_EQ(b_str, bigint_to_dec(b));
    ASSERT_EQ(c_str, bigint_to_dec(c));
    ASSERT_EQ(d_str, bigint_to_dec(d));

    bigint_from_dec(a_2, a_str);
    ASSERT_EQ(a, a_2);
    bigint_from_dec(b_2, b_str);
    ASSERT_EQ(b, b_2);
    bigint_from_dec(c_2, c_str);
    ASSERT_EQ(c, c_2);
    bigint_from_dec(d_2, d_str);
    ASSERT_EQ(d, d_2);

    // Hex serialization
    ASSERT_EQ(a_hex, bigint_to_hex(a));
    ASSERT_EQ(b_hex, bigint_to_hex(b));
    ASSERT_EQ(c_hex, bigint_to_hex(c));
    ASSERT_EQ(d_hex, bigint_to_hex(d));

    bigint_from_hex(a_2, a_hex);
    ASSERT_EQ(a, a_2);
    bigint_from_hex(b_2, b_hex);
    ASSERT_EQ(b, b_2);
    bigint_from_hex(c_2, c_hex);
    ASSERT_EQ(c, c_2);
    bigint_from_hex(d_2, d_hex);
    ASSERT_EQ(d, d_2);

    // Hex serialization with prefix
    ASSERT_EQ(a_hex_p, bigint_to_hex(a, true));
    ASSERT_EQ(b_hex_p, bigint_to_hex(b, true));
    ASSERT_EQ(c_hex_p, bigint_to_hex(c, true));
    ASSERT_EQ(d_hex_p, bigint_to_hex(d, true));

    bigint_from_hex(a_2, a_hex_p);
    ASSERT_EQ(a, a_2);
    bigint_from_hex(b_2, b_hex_p);
    ASSERT_EQ(b, b_2);
    bigint_from_hex(c_2, c_hex_p);
    ASSERT_EQ(c, c_2);
    bigint_from_hex(d_2, d_hex_p);
    ASSERT_EQ(d, d_2);
}

TEST(FieldsTest, Edwards)
{
    edwards_pp::init_public_params();
    test_serialization<edwards_pp>();
    test_all_fields<edwards_pp>();
    test_cyclotomic_squaring<Fqk<edwards_pp>>();
}

TEST(FieldsTest, MNT4)
{
    mnt4_pp::init_public_params();
    test_serialization<mnt4_pp>();
    test_all_fields<mnt4_pp>();
    test_Fp4_tom_cook<mnt4_Fq4>();
    test_two_squarings<Fqe<mnt4_pp>>();
    test_cyclotomic_squaring<Fqk<mnt4_pp>>();
}

TEST(FieldsTest, MNT6)
{
    mnt6_pp::init_public_params();
    test_serialization<mnt6_pp>();
    test_all_fields<mnt6_pp>();
    test_cyclotomic_squaring<Fqk<mnt6_pp>>();
}

TEST(FieldsTest, ALT_BN128)
{
    alt_bn128_pp::init_public_params();
    test_serialization<alt_bn128_pp>();
    test_field<alt_bn128_Fq6>();
    test_Frobenius<alt_bn128_Fq6>();
    test_all_fields<alt_bn128_pp>();
    test_Fp12_2over3over2_mul_by_024<alt_bn128_Fq12>();
    test_signed_digits<alt_bn128_Fr>();

    test_field_get_digit_alt_bn128();
}

TEST(FieldsTest, BLS12_377)
{
    bls12_377_pp::init_public_params();
    test_serialization<bls12_377_pp>();
    test_field<bls12_377_Fq6>();
    test_all_fields<bls12_377_pp>();
    test_Fp12_2over3over2_mul_by_024<bls12_377_Fq12>();
    test_signed_digits<bls12_377_Fr>();
}

// BN128 has fancy dependencies so it may be disabled
#ifdef CURVE_BN128
TEST(FieldsTest, BN128)
{
    bn128_pp::init_public_params();
    // TODO: Specialize the serialization test to compile for bn128_pp
    // (currently expects Fpe type).
    // test_serialization<bn128_pp>();
    test_field<Fr<bn128_pp>>();
    test_field<Fq<bn128_pp>>();
}
#endif
