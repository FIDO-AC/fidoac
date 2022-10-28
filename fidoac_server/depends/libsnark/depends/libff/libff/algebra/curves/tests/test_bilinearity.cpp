/**
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <gtest/gtest.h>
#include <libff/algebra/curves/edwards/edwards_pp.hpp>
#include <libff/common/profiling.hpp>
#ifdef CURVE_BN128
#include <libff/algebra/curves/bn128/bn128_pp.hpp>
#endif
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_pp.hpp>
#include <libff/algebra/curves/mnt/mnt4/mnt4_pp.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_pp.hpp>

using namespace libff;

template<typename ppT> void pairing_test()
{
    GT<ppT> GT_one = GT<ppT>::one();

    printf("Running bilinearity tests:\n");
    G1<ppT> P = (Fr<ppT>::random_element()) * G1<ppT>::one();
    G2<ppT> Q = (Fr<ppT>::random_element()) * G2<ppT>::one();

    printf("P:\n");
    P.print();
    P.print_coordinates();
    printf("Q:\n");
    Q.print();
    Q.print_coordinates();
    printf("\n\n");

    Fr<ppT> s = Fr<ppT>::random_element();
    G1<ppT> sP = s * P;
    G2<ppT> sQ = s * Q;

    printf("Pairing bilinearity tests (three must match):\n");
    GT<ppT> ans1 = ppT::reduced_pairing(sP, Q);
    GT<ppT> ans2 = ppT::reduced_pairing(P, sQ);
    GT<ppT> ans3 = ppT::reduced_pairing(P, Q) ^ s;
    printf("ans1:\n");
    ans1.print();
    printf("ans2:\n");
    ans2.print();
    printf("ans3:\n");
    ans3.print();
    ASSERT_EQ(ans1, ans2);
    ASSERT_EQ(ans2, ans3);
    std::cout << "**** RES (ans1 == ans2) : " << (ans1 == ans2) << " ****"
              << std::endl;
    std::cout << "**** RES (ans2 == ans3) : " << (ans2 == ans3) << " ****"
              << std::endl;

    ASSERT_NE(ans1, GT_one);
    ASSERT_EQ(ans1 ^ Fr<ppT>::field_char(), GT_one);
    printf("\n\n");

    Fr<ppT> r = Fr<ppT>::random_element();
    G1<ppT> rP = r * P;
    G2<ppT> r_minus_1_Q = (r - 1) * Q;

    printf("Pairing bilinearity tests (two must NOT match):\n");
    GT<ppT> res1 = ppT::reduced_pairing(rP, Q);
    GT<ppT> res2 = ppT::reduced_pairing(P, r_minus_1_Q);
    printf("res1:\n");
    res1.print();
    printf("res2:\n");
    res2.print();
    ASSERT_NE(res1, res2);
    std::cout << "**** RES (res1 != res2) : " << (res1 != res2) << " ****"
              << std::endl;
}

template<typename ppT> void double_miller_loop_test()
{
    const G1<ppT> P1 = (Fr<ppT>::random_element()) * G1<ppT>::one();
    const G1<ppT> P2 = (Fr<ppT>::random_element()) * G1<ppT>::one();
    const G2<ppT> Q1 = (Fr<ppT>::random_element()) * G2<ppT>::one();
    const G2<ppT> Q2 = (Fr<ppT>::random_element()) * G2<ppT>::one();

    const G1_precomp<ppT> prec_P1 = ppT::precompute_G1(P1);
    const G1_precomp<ppT> prec_P2 = ppT::precompute_G1(P2);
    const G2_precomp<ppT> prec_Q1 = ppT::precompute_G2(Q1);
    const G2_precomp<ppT> prec_Q2 = ppT::precompute_G2(Q2);

    const Fqk<ppT> ans_1 = ppT::miller_loop(prec_P1, prec_Q1);
    const Fqk<ppT> ans_2 = ppT::miller_loop(prec_P2, prec_Q2);
    const Fqk<ppT> ans_12 =
        ppT::double_miller_loop(prec_P1, prec_Q1, prec_P2, prec_Q2);
    ASSERT_EQ(ans_1 * ans_2, ans_12);
}

template<typename ppT> void affine_pairing_test()
{
    GT<ppT> GT_one = GT<ppT>::one();

    printf("Running bilinearity tests:\n");
    G1<ppT> P = (Fr<ppT>::random_element()) * G1<ppT>::one();
    G2<ppT> Q = (Fr<ppT>::random_element()) * G2<ppT>::one();

    printf("P:\n");
    P.print();
    printf("Q:\n");
    Q.print();
    printf("\n\n");

    Fr<ppT> s = Fr<ppT>::random_element();
    G1<ppT> sP = s * P;
    G2<ppT> sQ = s * Q;

    GT<ppT> ans1 = ppT::affine_reduced_pairing(sP, Q);
    GT<ppT> ans2 = ppT::affine_reduced_pairing(P, sQ);
    GT<ppT> ans3 = ppT::affine_reduced_pairing(P, Q) ^ s;
    ans1.print();
    ans2.print();
    ans3.print();
    ASSERT_EQ(ans1, ans2);
    ASSERT_EQ(ans2, ans3);

    ASSERT_NE(ans1, GT_one);
    ASSERT_EQ((ans1 ^ Fr<ppT>::field_char()), GT_one);
}

TEST(TestBiliearity, Edwards)
{
    edwards_pp::init_public_params();
    pairing_test<edwards_pp>();
    double_miller_loop_test<edwards_pp>();
}

TEST(TestBiliearity, Mnt6)
{
    mnt6_pp::init_public_params();
    pairing_test<mnt6_pp>();
    double_miller_loop_test<mnt6_pp>();
    affine_pairing_test<mnt6_pp>();
}

TEST(TestBiliearity, Mnt4)
{
    mnt4_pp::init_public_params();
    pairing_test<mnt4_pp>();
    double_miller_loop_test<mnt4_pp>();
    affine_pairing_test<mnt4_pp>();
}

TEST(TestBiliearity, Alt_BN128)
{
    alt_bn128_pp::init_public_params();
    pairing_test<alt_bn128_pp>();
    double_miller_loop_test<alt_bn128_pp>();
}

TEST(TestBiliearity, BLS12_377)
{
    bls12_377_pp::init_public_params();
    pairing_test<bls12_377_pp>();
    double_miller_loop_test<bls12_377_pp>();
}

TEST(TestBiliearity, BW6_761)
{
    bw6_761_pp::init_public_params();
    pairing_test<bw6_761_pp>();
    double_miller_loop_test<bw6_761_pp>();
}

// BN128 has fancy dependencies so it may be disabled
#ifdef CURVE_BN128
TEST(TestBiliearity, BN128)
{
    bn128_pp::init_public_params();
    pairing_test<bn128_pp>();
    double_miller_loop_test<bn128_pp>();
}
#endif
