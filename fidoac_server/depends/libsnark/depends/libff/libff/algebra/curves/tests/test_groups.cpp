/**
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <gtest/gtest.h>
#include <libff/algebra/curves/edwards/edwards_pp.hpp>
#include <libff/algebra/curves/mnt/mnt4/mnt4_pp.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_pp.hpp>
#ifdef CURVE_BN128
#include <libff/algebra/curves/bn128/bn128_pp.hpp>
#endif
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_pp.hpp>
#include <libff/algebra/curves/curve_serialization.hpp>
#include <libff/algebra/curves/curve_utils.hpp>
#include <sstream>

using namespace libff;

template<typename GroupT> void test_mixed_add()
{
    GroupT base, el, result;

    base = GroupT::zero();
    el = GroupT::zero();
    el.to_special();
    result = base.mixed_add(el);
    ASSERT_EQ(GroupT::zero(), result);

    base = GroupT::zero();
    el = GroupT::random_element();
    el.to_special();
    result = base.mixed_add(el);
    ASSERT_EQ(el, result);

    base = GroupT::random_element();
    el = GroupT::zero();
    el.to_special();
    result = base.mixed_add(el);
    ASSERT_EQ(base, result);

    base = GroupT::random_element();
    el = GroupT::random_element();
    el.to_special();
    result = base.mixed_add(el);
    ASSERT_EQ(base + el, result);

    base = GroupT::random_element();
    el = base;
    el.to_special();
    result = base.mixed_add(el);
    ASSERT_EQ(base.dbl(), result);
}

template<typename GroupT> void test_group()
{
    bigint<1> rand1 = bigint<1>("76749407");
    bigint<1> rand2 = bigint<1>("44410867");
    bigint<1> randsum = bigint<1>("121160274");

    GroupT zero = GroupT::zero();
    ASSERT_EQ(zero, zero);
    GroupT one = GroupT::one();
    ASSERT_EQ(one, one);
    GroupT two = bigint<1>(2l) * GroupT::one();
    ASSERT_EQ(two, two);
    GroupT five = bigint<1>(5l) * GroupT::one();

    GroupT three = bigint<1>(3l) * GroupT::one();
    GroupT four = bigint<1>(4l) * GroupT::one();

    ASSERT_EQ(three + four, two + five);

    GroupT a = GroupT::random_element();
    GroupT b = GroupT::random_element();

    ASSERT_NE(one, zero);
    ASSERT_NE(a, zero);
    ASSERT_NE(a, one);

    ASSERT_NE(b, zero);
    ASSERT_NE(b, one);

    ASSERT_EQ(a + a, a.dbl());
    ASSERT_EQ(b + b, b.dbl());
    ASSERT_EQ(three, one.add(two));
    ASSERT_EQ(three, two.add(one));
    ASSERT_EQ(b + a, a + b);
    ASSERT_EQ(zero, a - a);
    ASSERT_EQ(a + (-b), a - b);
    ASSERT_EQ((-b) + a, a - b);

    // handle special cases
    ASSERT_EQ(-a, zero + (-a));
    ASSERT_EQ(-a, zero - a);
    ASSERT_EQ(a, a - zero);
    ASSERT_EQ(a, a + zero);
    ASSERT_EQ(a, zero + a);

    ASSERT_EQ((a + b) + (b + a), (a + b).dbl());
    ASSERT_EQ((a + b) + (b + a), bigint<1>("2") * (a + b));

    ASSERT_EQ((randsum * a), (rand1 * a) + (rand2 * a));

    ASSERT_EQ(zero, GroupT::order() * a);
    ASSERT_EQ(zero, GroupT::order() * one);
    ASSERT_NE((GroupT::order() * a) - a, zero);
    ASSERT_NE((GroupT::order() * one) - one, zero);

    test_mixed_add<GroupT>();
}

template<typename GroupT> void test_mul_by_q()
{
    GroupT a = GroupT::random_element();
    ASSERT_EQ(a.mul_by_q(), (GroupT::base_field_char() * a));
}

template<typename GroupT> void test_mul_by_cofactor()
{
    const GroupT a = GroupT::random_element();
    const GroupT a_h = GroupT::h * a;
    ASSERT_EQ(a_h, a.mul_by_cofactor());
}

template<typename GroupT> void test_output()
{
    GroupT g = GroupT::zero();

    for (size_t i = 0; i < 1000; ++i) {
        std::stringstream ss;
        ss << g;
        g.write_compressed(ss);
        g.write_uncompressed(ss);

        GroupT gg;
        GroupT gg_comp;
        GroupT gg_uncomp;
        ss >> gg;
        GroupT::read_compressed(ss, gg_comp);
        GroupT::read_uncompressed(ss, gg_uncomp);

        ASSERT_EQ(g, gg);
        ASSERT_EQ(g, gg_comp);
        ASSERT_EQ(g, gg_uncomp);
        /* use a random point in next iteration */
        g = GroupT::random_element();
    }
}

template<
    encoding_t Enc = encoding_binary,
    form_t Form = form_plain,
    compression_t Comp = compression_on,
    typename GroupT>
void test_serialize_group_config(const GroupT &v)
{
    GroupT v_aff(v);
    v_aff.to_affine_coordinates();

    std::string buffer;
    {
        std::ostringstream ss;
        group_write<Enc, Form, Comp>(v, ss);
        buffer = ss.str();
    }

    GroupT v_dec;
    {
        std::istringstream ss(buffer);
        group_read<Enc, Form, Comp>(v_dec, ss);
    }

    ASSERT_EQ(v, v_dec);
}

template<typename GroupT> void test_serialize_group_element(const GroupT &v)
{
    // Missing combinations are unsupported.

    test_serialize_group_config<encoding_binary, form_plain, compression_on>(v);
    test_serialize_group_config<encoding_binary, form_plain, compression_off>(
        v);
    test_serialize_group_config<
        encoding_binary,
        form_montgomery,
        compression_on>(v);
    test_serialize_group_config<
        encoding_binary,
        form_montgomery,
        compression_off>(v);

    test_serialize_group_config<encoding_json, form_plain, compression_off>(v);
    test_serialize_group_config<
        encoding_json,
        form_montgomery,
        compression_off>(v);
}

template<typename GroupT> void test_serialize_group()
{
    test_serialize_group_element(GroupT::zero());
    test_serialize_group_element(GroupT::one());
    test_serialize_group_element(GroupT::zero() - GroupT::one());
    test_serialize_group_element(
        GroupT::zero() - GroupT::one() - GroupT::one());
    test_serialize_group_element(GroupT::random_element());
    test_serialize_group_element(GroupT::random_element());
    test_serialize_group_element(GroupT::random_element());
    test_serialize_group_element(GroupT::random_element());
}

template<typename ppT> void test_serialize()
{
    test_serialize_group<G1<ppT>>();
    test_serialize_group<G2<ppT>>();
}

template<typename GroupT> void test_group_membership_valid()
{
    for (size_t i = 0; i < 1000; ++i) {
        GroupT g = GroupT::random_element();
        ASSERT_TRUE(g.is_in_safe_subgroup());
    }
}

template<typename GroupT> void test_group_membership_proof_valid()
{
    for (size_t i = 0; i < 1000; ++i) {
        const GroupT g = GroupT::random_element();
        const GroupT membership_proof = g.proof_of_safe_subgroup();
        ASSERT_EQ(g, membership_proof.mul_by_cofactor());
    }
}

template<typename GroupT>
void test_group_membership_invalid_g1(const typename GroupT::base_field &x)
{
    const GroupT g1_invalid = g1_curve_point_at_x<GroupT>(x);
    ASSERT_TRUE(g1_invalid.is_well_formed());
    ASSERT_FALSE(g1_invalid.is_in_safe_subgroup());
}

template<typename GroupT>
void test_group_membership_proof_invalid_g1(
    const typename GroupT::base_field &x)
{
    const GroupT g1_invalid = g1_curve_point_at_x<GroupT>(x);
    const GroupT proof_of_membership = g1_invalid.proof_of_safe_subgroup();
    ASSERT_NE(g1_invalid, proof_of_membership.mul_by_cofactor());
}

template<typename GroupT>
void test_group_membership_invalid_g2(const typename GroupT::twist_field &x)
{
    const GroupT g2_invalid = g2_curve_point_at_x<GroupT>(x);
    ASSERT_TRUE(g2_invalid.is_well_formed());
    ASSERT_FALSE(g2_invalid.is_in_safe_subgroup());
}

template<typename ppT> void test_check_membership()
{
    test_group_membership_valid<G1<ppT>>();
    test_group_membership_valid<G2<ppT>>();
}

template<> void test_check_membership<alt_bn128_pp>()
{
    test_group_membership_valid<alt_bn128_G1>();
    test_group_membership_valid<alt_bn128_G2>();
    // Skip the G1 check (there are no points on the curve over Fq which are
    // not in the subgroup).
    test_group_membership_invalid_g2<alt_bn128_G2>(alt_bn128_Fq2::one());
}

template<> void test_check_membership<bls12_377_pp>()
{
    test_group_membership_valid<bls12_377_G1>();
    test_group_membership_valid<bls12_377_G2>();
    test_group_membership_invalid_g1<bls12_377_G1>(bls12_377_Fq(3));
    test_group_membership_invalid_g2<bls12_377_G2>(
        bls12_377_Fq(3) * bls12_377_Fq2::one());

    test_group_membership_proof_valid<bls12_377_G1>();
    test_group_membership_proof_invalid_g1<bls12_377_G1>(bls12_377_Fq(3));
}

template<> void test_check_membership<bw6_761_pp>()
{
    test_group_membership_valid<bw6_761_G1>();
    test_group_membership_valid<bw6_761_G2>();
    test_group_membership_invalid_g1<bw6_761_G1>(bw6_761_Fq(6));
    test_group_membership_invalid_g2<bw6_761_G2>(bw6_761_Fq(0));
}

void test_bls12_377()
{
    const bls12_377_G1 g1 = bls12_377_G1::random_element();

    // Ensure sigma endomorphism results in multiplication by expected lambda.
    const bls12_377_G1 sigma_g1 = g1.sigma();
    ASSERT_EQ(
        (bls12_377_Fr("91893752504881257701523279626832445440") * g1),
        sigma_g1);

    // Ensure untwist-frobenius-twist operation \psi satisfies:
    //   \psi^2(P) - [t] \psi(P) + [q]P = zero
    const bls12_377_G2 a = bls12_377_G2::random_element();
    const bls12_377_G2 uft = a.untwist_frobenius_twist();
    const bls12_377_G2 uft_2 = uft.untwist_frobenius_twist();
    const bls12_377_G2 z = uft_2 - (bls12_377_trace_of_frobenius * uft) +
                           (bls12_377_modulus_q * a);
    ASSERT_EQ(bls12_377_G2::zero(), z);
}

TEST(TestGroups, Edwards)
{
    edwards_pp::init_public_params();
    test_group<G1<edwards_pp>>();
    test_output<G1<edwards_pp>>();
    test_group<G2<edwards_pp>>();
    test_output<G2<edwards_pp>>();
    test_mul_by_q<G2<edwards_pp>>();
}

TEST(TestGroups, Mnt4)
{
    mnt4_pp::init_public_params();
    test_group<G1<mnt4_pp>>();
    test_output<G1<mnt4_pp>>();
    test_group<G2<mnt4_pp>>();
    test_output<G2<mnt4_pp>>();
    test_serialize<mnt4_pp>();
    test_mul_by_q<G2<mnt4_pp>>();
    test_check_membership<mnt4_pp>();
    test_mul_by_cofactor<G1<mnt4_pp>>();
    test_mul_by_cofactor<G2<mnt4_pp>>();
}

TEST(TestGroups, Mnt6)
{
    mnt6_pp::init_public_params();
    test_group<G1<mnt6_pp>>();
    test_output<G1<mnt6_pp>>();
    test_group<G2<mnt6_pp>>();
    test_output<G2<mnt6_pp>>();
    test_serialize<mnt6_pp>();
    test_mul_by_q<G2<mnt6_pp>>();
    test_check_membership<mnt6_pp>();
    test_mul_by_cofactor<G1<mnt6_pp>>();
    test_mul_by_cofactor<G2<mnt6_pp>>();
}

TEST(TestGroups, Alt_BN128)
{
    alt_bn128_pp::init_public_params();
    test_group<G1<alt_bn128_pp>>();
    test_output<G1<alt_bn128_pp>>();
    test_group<G2<alt_bn128_pp>>();
    test_output<G2<alt_bn128_pp>>();
    test_serialize<alt_bn128_pp>();
    test_mul_by_q<G2<alt_bn128_pp>>();
    test_check_membership<alt_bn128_pp>();
    test_mul_by_cofactor<G1<alt_bn128_pp>>();
    test_mul_by_cofactor<G2<alt_bn128_pp>>();
}

TEST(TestGroups, BLS12_377)
{
    bls12_377_pp::init_public_params();
    test_bls12_377();
    test_group<G1<bls12_377_pp>>();
    test_output<G1<bls12_377_pp>>();
    test_group<G2<bls12_377_pp>>();
    test_output<G2<bls12_377_pp>>();
    test_serialize<bls12_377_pp>();
    test_mul_by_q<G2<bls12_377_pp>>();
    test_check_membership<bls12_377_pp>();
    test_mul_by_cofactor<G1<bls12_377_pp>>();
    test_mul_by_cofactor<G2<bls12_377_pp>>();
}

TEST(TestGroups, BW6_761)
{
    bw6_761_pp::init_public_params();
    test_group<G1<bw6_761_pp>>();
    test_output<G1<bw6_761_pp>>();
    test_group<G2<bw6_761_pp>>();
    test_output<G2<bw6_761_pp>>();
    test_serialize<bw6_761_pp>();
    test_mul_by_q<G2<bw6_761_pp>>();
    test_check_membership<bw6_761_pp>();
    test_mul_by_cofactor<G1<bw6_761_pp>>();
    test_mul_by_cofactor<G2<bw6_761_pp>>();
}

// BN128 has fancy dependencies so it may be disabled
#ifdef CURVE_BN128
TEST(TestGroups, BN128)
{
    bn128_pp::init_public_params();
    test_group<G1<bn128_pp>>();
    test_output<G1<bn128_pp>>();
    test_group<G2<bn128_pp>>();
    test_output<G2<bn128_pp>>();
    test_check_membership<bn128_pp>();
    test_mul_by_cofactor<G1<bn128_pp>>();
    test_mul_by_cofactor<G2<bn128_pp>>();
}
#endif
