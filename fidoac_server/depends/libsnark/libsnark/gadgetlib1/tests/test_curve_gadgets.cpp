/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bw6_761_pairing_params.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/mnt/mnt_pairing_params.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/pairing_params.hpp"
#include "libsnark/zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/r1cs_gg_ppzksnark.hpp"

#include <gtest/gtest.h>
#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_pp.hpp>
#include <libff/algebra/curves/mnt/mnt4/mnt4_pp.hpp>
#include <libff/algebra/curves/mnt/mnt6/mnt6_pp.hpp>

using namespace libsnark;

using wpp = libff::bw6_761_pp;
using npp = other_curve<wpp>;

namespace
{

template<typename ppT>
void generate_and_check_proof(protoboard<libff::Fr<ppT>> &pb)
{
    // Generate and check the proof
    ASSERT_TRUE(pb.is_satisfied());
    const r1cs_gg_ppzksnark_keypair<ppT> keypair =
        r1cs_gg_ppzksnark_generator<ppT>(pb.get_constraint_system(), true);
    r1cs_primary_input<libff::Fr<wpp>> primary_input = pb.primary_input();
    r1cs_auxiliary_input<libff::Fr<ppT>> auxiliary_input = pb.auxiliary_input();
    r1cs_gg_ppzksnark_proof<ppT> proof = r1cs_gg_ppzksnark_prover(
        keypair.pk, primary_input, auxiliary_input, true);
    ASSERT_TRUE(r1cs_gg_ppzksnark_verifier_strong_IC<ppT>(
        keypair.vk, primary_input, proof));
}

template<typename ppT>
void test_G2_checker_gadget(const std::string &annotation)
{
    protoboard<libff::Fr<ppT>> pb;
    G2_variable<ppT> g(pb, "g");
    G2_checker_gadget<ppT> g_check(pb, g, "g_check");
    g_check.generate_r1cs_constraints();

    printf("positive test\n");
    g.generate_r1cs_witness(libff::G2<other_curve<ppT>>::one());
    g_check.generate_r1cs_witness();
    assert(pb.is_satisfied());

    printf("negative test\n");
    g.generate_r1cs_witness(libff::G2<other_curve<ppT>>::zero());
    g_check.generate_r1cs_witness();
    assert(!pb.is_satisfied());

    printf(
        "number of constraints for G2 checker (Fr is %s)  = %zu\n",
        annotation.c_str(),
        pb.num_constraints());
}

template<
    typename ppT,
    typename GroupT,
    typename VarA,
    typename VarB,
    typename VarR,
    typename AddGadgetT>
void test_add_gadget(
    const GroupT &a_val, const GroupT &b_val, const GroupT &expect_val)
{
    ASSERT_EQ(expect_val, a_val + b_val);

    protoboard<libff::Fr<wpp>> pb;
    VarA A(pb, "A");
    VarB B(pb, "B");
    VarR result(pb, "result");
    AddGadgetT add_gadget(pb, A, B, result, "add_gadget");

    add_gadget.generate_r1cs_constraints();

    A.generate_r1cs_witness(a_val);
    B.generate_r1cs_witness(b_val);
    add_gadget.generate_r1cs_witness();

    const GroupT result_val = result.get_element();
    ASSERT_TRUE(pb.is_satisfied());
    ASSERT_EQ(expect_val, result_val);
}

template<typename ppT, typename GroupT, typename VarT, typename DblGadgetT>
void test_dbl_gadget(const GroupT &a_val, const GroupT &expect_val)
{
    ASSERT_EQ(expect_val, a_val + a_val);

    protoboard<libff::Fr<wpp>> pb;
    VarT A(pb, "A");
    VarT result(pb, "result");
    DblGadgetT dbl_gadget(pb, A, result, "add_gadget");

    dbl_gadget.generate_r1cs_constraints();

    A.generate_r1cs_witness(a_val);
    dbl_gadget.generate_r1cs_witness();

    const GroupT result_val = result.get_element();
    ASSERT_TRUE(pb.is_satisfied());
    ASSERT_EQ(expect_val, result_val);
}

template<
    typename ppT,
    typename GroupT,
    typename VarA,
    typename VarB,
    typename VarR,
    typename SelectorGadgetT>
void test_selector_gadget(GroupT a_val, GroupT b_val)
{
    a_val.to_affine_coordinates();
    b_val.to_affine_coordinates();

    using Field = libff::Fr<ppT>;

    protoboard<Field> pb;

    VarA zero_case(pb, "zero_case");
    VarB one_case(pb, "one_case");

    pb_variable<Field> selector_zero;
    selector_zero.allocate(pb, "selector_zero");
    pb_variable<Field> selector_one;
    selector_one.allocate(pb, "selector_one");

    VarR zero_result(pb, "zero_result");
    VarR one_result(pb, "one_result");

    SelectorGadgetT selector_gadget_zero(
        pb,
        selector_zero,
        zero_case,
        one_case,
        zero_result,
        "selector_gadget_zero");
    SelectorGadgetT selector_gadget_one(
        pb,
        selector_one,
        zero_case,
        one_case,
        one_result,
        "selector_gadget_one");

    selector_gadget_zero.generate_r1cs_constraints();
    selector_gadget_one.generate_r1cs_constraints();

    // Assignments: zero_case = [2]_1, one_case = [-1]_1

    zero_case.generate_r1cs_witness(a_val);
    one_case.generate_r1cs_witness(b_val);
    pb.val(selector_zero) = Field::zero();
    pb.val(selector_one) = Field::one();
    selector_gadget_zero.generate_r1cs_witness();
    selector_gadget_one.generate_r1cs_witness();

    ASSERT_EQ(a_val, zero_result.get_element());
    ASSERT_EQ(b_val, one_result.get_element());

    // Verify in a proof
    generate_and_check_proof<wpp>(pb);
}

template<typename ppT, typename GroupT, typename VarT, typename SelectorGadgetT>
void test_variable_or_identity_selector_gadget()
{
    auto test_selector =
        test_selector_gadget<wpp, GroupT, VarT, VarT, VarT, SelectorGadgetT>;

    const GroupT a_val = GroupT::one() + GroupT::one();
    const GroupT b_val = -GroupT::one();

    test_selector(a_val, b_val);
    test_selector(GroupT::zero(), b_val);
    test_selector(a_val, GroupT::zero());
    test_selector(GroupT::zero(), GroupT::zero());
}

template<
    typename ppT,
    typename GroupT,
    typename VarA,
    typename VarB,
    typename VarR,
    typename SelectorGadgetT>
void test_variable_and_variable_or_identity_selector_gadget()
{
    auto test_selector =
        test_selector_gadget<wpp, GroupT, VarA, VarB, VarR, SelectorGadgetT>;

    const GroupT a_val = GroupT::one() + GroupT::one();
    const GroupT b_val = -GroupT::one();

    test_selector(a_val, b_val);
    test_selector(GroupT::zero(), b_val);
}

TEST(TestCurveGadgets, G2Checker)
{
    test_G2_checker_gadget<libff::mnt4_pp>("mnt4");
    test_G2_checker_gadget<libff::mnt6_pp>("mnt6");
}

TEST(TestCurveGadgets, G1SelectorGadget)
{
    using Group = libff::G1<npp>;
    auto test_selector = test_selector_gadget<
        wpp,
        Group,
        G1_variable<wpp>,
        G1_variable<wpp>,
        G1_variable<wpp>,
        G1_variable_selector_gadget<wpp>>;

    test_selector(Group::one() + Group::one(), -Group::one());
}

TEST(TestCurveGadgets, G2SelectorGadget)
{
    using Group = libff::G2<npp>;
    auto test_selector = test_selector_gadget<
        wpp,
        Group,
        G2_variable<wpp>,
        G2_variable<wpp>,
        G2_variable<wpp>,
        G2_variable_selector_gadget<wpp>>;

    test_selector(Group::one() + Group::one(), -Group::one());
}

TEST(TestCurveGadgets, G1VarOrIdentitySelectorGadget)
{
    test_variable_or_identity_selector_gadget<
        wpp,
        libff::G1<npp>,
        G1_variable_or_identity<wpp>,
        G1_variable_or_identity_selector_gadget<wpp>>();
}

TEST(TestCurveGadgets, G2VarOrIdentitySelectorGadget)
{
    test_variable_or_identity_selector_gadget<
        wpp,
        libff::G2<npp>,
        G2_variable_or_identity<wpp>,
        G2_variable_or_identity_selector_gadget<wpp>>();
}

TEST(TestCurveGadgets, G1VarAndVarOrIdentitySelectorGadget)
{
    test_variable_and_variable_or_identity_selector_gadget<
        wpp,
        libff::G1<npp>,
        G1_variable_or_identity<wpp>,
        G1_variable<wpp>,
        G1_variable_or_identity<wpp>,
        G1_variable_and_variable_or_identity_selector_gadget<wpp>>();
}

TEST(TestCurveGadgets, G2VarAndVarOrIdentitySelectorGadget)
{
    test_variable_and_variable_or_identity_selector_gadget<
        wpp,
        libff::G2<npp>,
        G2_variable_or_identity<wpp>,
        G2_variable<wpp>,
        G2_variable_or_identity<wpp>,
        G2_variable_and_variable_or_identity_selector_gadget<wpp>>();
}

TEST(TestCurveGadgets, G1AddGadget)
{
    test_add_gadget<
        wpp,
        libff::G1<npp>,
        G1_variable<wpp>,
        G1_variable<wpp>,
        G1_variable<wpp>,
        G1_add_gadget<wpp>>(
        libff::Fr<npp>(13) * libff::G1<npp>::one(),
        libff::Fr<npp>(12) * libff::G1<npp>::one(),
        libff::Fr<npp>(12 + 13) * libff::G1<npp>::one());
}

TEST(TestCurveGadgets, G2AddGadget)
{
    test_add_gadget<
        wpp,
        libff::G2<npp>,
        G2_variable<wpp>,
        G2_variable<wpp>,
        G2_variable<wpp>,
        G2_add_gadget<wpp>>(
        libff::Fr<npp>(13) * libff::G2<npp>::one(),
        libff::Fr<npp>(12) * libff::G2<npp>::one(),
        libff::Fr<npp>(12 + 13) * libff::G2<npp>::one());
}

TEST(TestCurveGadgets, G1AddVarOrIdentityGadget)
{
    auto test_add_variable_or_identity = test_add_gadget<
        wpp,
        libff::G1<npp>,
        G1_variable_or_identity<wpp>,
        G1_variable_or_identity<wpp>,
        G1_variable_or_identity<wpp>,
        G1_add_variable_or_identity_gadget<wpp>>;

    test_add_variable_or_identity(
        libff::Fr<npp>(13) * libff::G1<npp>::one(),
        libff::Fr<npp>(12) * libff::G1<npp>::one(),
        libff::Fr<npp>(12 + 13) * libff::G1<npp>::one());

    test_add_variable_or_identity(
        libff::Fr<npp>(0) * libff::G1<npp>::one(),
        libff::Fr<npp>(12) * libff::G1<npp>::one(),
        libff::Fr<npp>(12) * libff::G1<npp>::one());

    test_add_variable_or_identity(
        libff::Fr<npp>(13) * libff::G1<npp>::one(),
        libff::Fr<npp>(0) * libff::G1<npp>::one(),
        libff::Fr<npp>(13) * libff::G1<npp>::one());

    // Note, the 0 + 0 case is not supported.
}

TEST(TestCurveGadgets, G2AddVarOrIdentityGadget)
{
    auto test_add_variable_or_identity = test_add_gadget<
        wpp,
        libff::G2<npp>,
        G2_variable_or_identity<wpp>,
        G2_variable_or_identity<wpp>,
        G2_variable_or_identity<wpp>,
        G2_add_variable_or_identity_gadget<wpp>>;

    test_add_variable_or_identity(
        libff::Fr<npp>(13) * libff::G2<npp>::one(),
        libff::Fr<npp>(12) * libff::G2<npp>::one(),
        libff::Fr<npp>(12 + 13) * libff::G2<npp>::one());

    test_add_variable_or_identity(
        libff::Fr<npp>(0) * libff::G2<npp>::one(),
        libff::Fr<npp>(12) * libff::G2<npp>::one(),
        libff::Fr<npp>(12) * libff::G2<npp>::one());

    test_add_variable_or_identity(
        libff::Fr<npp>(13) * libff::G2<npp>::one(),
        libff::Fr<npp>(0) * libff::G2<npp>::one(),
        libff::Fr<npp>(13) * libff::G2<npp>::one());

    // Note, the 0 + 0 case is not supported.
}

TEST(TestCurveGadgets, G1AddVarAndVarOrIdentityGadget)
{
    auto test_add_variable_and_variable_or_identity = test_add_gadget<
        wpp,
        libff::G1<npp>,
        G1_variable_or_identity<wpp>,
        G1_variable<wpp>,
        G1_variable<wpp>,
        G1_add_variable_and_variable_or_identity_gadget<wpp>>;

    test_add_variable_and_variable_or_identity(
        libff::Fr<npp>(13) * libff::G1<npp>::one(),
        libff::Fr<npp>(12) * libff::G1<npp>::one(),
        libff::Fr<npp>(12 + 13) * libff::G1<npp>::one());

    test_add_variable_and_variable_or_identity(
        libff::Fr<npp>(0) * libff::G1<npp>::one(),
        libff::Fr<npp>(12) * libff::G1<npp>::one(),
        libff::Fr<npp>(12) * libff::G1<npp>::one());

    // Note, the 0 + 0 case is not supported.
}

TEST(TestCurveGadgets, G2AddVarAndVarOrIdentityGadget)
{
    auto test_add_variable_and_variable_or_identity = test_add_gadget<
        wpp,
        libff::G2<npp>,
        G2_variable_or_identity<wpp>,
        G2_variable<wpp>,
        G2_variable<wpp>,
        G2_add_variable_and_variable_or_identity_gadget<wpp>>;

    test_add_variable_and_variable_or_identity(
        libff::Fr<npp>(13) * libff::G2<npp>::one(),
        libff::Fr<npp>(12) * libff::G2<npp>::one(),
        libff::Fr<npp>(12 + 13) * libff::G2<npp>::one());

    test_add_variable_and_variable_or_identity(
        libff::Fr<npp>(0) * libff::G2<npp>::one(),
        libff::Fr<npp>(12) * libff::G2<npp>::one(),
        libff::Fr<npp>(12) * libff::G2<npp>::one());

    // Note, the 0 + 0 case is not supported.
}

TEST(TestCurveGadgets, G1DblGadget)
{
    test_dbl_gadget<wpp, libff::G1<npp>, G1_variable<wpp>, G1_dbl_gadget<wpp>>(
        libff::Fr<npp>(13) * libff::G1<npp>::one(),
        libff::Fr<npp>(13 + 13) * libff::G1<npp>::one());
}

TEST(TestCurveGadgets, G2DblGadget)
{
    test_dbl_gadget<wpp, libff::G2<npp>, G2_variable<wpp>, G2_dbl_gadget<wpp>>(
        libff::Fr<npp>(13) * libff::G2<npp>::one(),
        libff::Fr<npp>(13 + 13) * libff::G2<npp>::one());
}

TEST(TestCurveGadgets, G1DblVarOrIdentityGadget)
{
    auto test_dbl_variable_or_identity = test_dbl_gadget<
        wpp,
        libff::G1<npp>,
        G1_variable_or_identity<wpp>,
        G1_dbl_variable_or_identity_gadget<wpp>>;

    test_dbl_variable_or_identity(
        libff::Fr<npp>(13) * libff::G1<npp>::one(),
        libff::Fr<npp>(13 + 13) * libff::G1<npp>::one());

    test_dbl_variable_or_identity(
        libff::G1<npp>::zero(), libff::G1<npp>::zero());
}

TEST(TestCurveGadgets, G2DblVarOrIdentityGadget)
{
    auto test_dbl_variable_or_identity = test_dbl_gadget<
        wpp,
        libff::G2<npp>,
        G2_variable_or_identity<wpp>,
        G2_dbl_variable_or_identity_gadget<wpp>>;

    test_dbl_variable_or_identity(
        libff::Fr<npp>(13) * libff::G2<npp>::one(),
        libff::Fr<npp>(13 + 13) * libff::G2<npp>::one());

    test_dbl_variable_or_identity(
        libff::G2<npp>::zero(), libff::G2<npp>::zero());
}

TEST(TestCurveGadgets, G1MulByConstScalar)
{
    // Compute inputs and results
    const libff::G1<npp> P_val = libff::Fr<npp>(13) * libff::G1<npp>::one();
    const libff::Fr<npp> scalar_val_a = libff::Fr<npp>(127);
    const libff::G1<npp> expect_result_val_a = scalar_val_a * P_val;
    const libff::Fr<npp> scalar_val_b = libff::Fr<npp>(122);
    const libff::G1<npp> expect_result_val_b = scalar_val_b * P_val;
    // Circuit
    protoboard<libff::Fr<wpp>> pb;
    G1_variable<wpp> P(pb, "P");
    G1_variable<wpp> result_a(pb, "result");
    G1_mul_by_const_scalar_gadget<wpp, libff::Fr<npp>::num_limbs> mul_gadget_a(
        pb, scalar_val_a.as_bigint(), P, result_a, "mul_gadget_a");
    G1_variable<wpp> result_b(pb, "result");
    G1_mul_by_const_scalar_gadget<wpp, libff::Fr<npp>::num_limbs> mul_gadget_b(
        pb, scalar_val_b.as_bigint(), P, result_b, "mul_gadget_b");

    mul_gadget_a.generate_r1cs_constraints();
    mul_gadget_b.generate_r1cs_constraints();

    P.generate_r1cs_witness(P_val);
    mul_gadget_a.generate_r1cs_witness();
    mul_gadget_b.generate_r1cs_witness();

    ASSERT_TRUE(pb.is_satisfied());

    const libff::G1<npp> result_a_val = result_a.get_element();
    ASSERT_EQ(expect_result_val_a, result_a_val);
    const libff::G1<npp> result_b_val = result_b.get_element();
    ASSERT_EQ(expect_result_val_b, result_b_val);
}

TEST(TestCurveGadgets, G1MulByConstScalarWithKnownResult)
{
    // Compute inputs and results
    const libff::G1<npp> P_val = libff::Fr<npp>(13) * libff::G1<npp>::one();
    const libff::G1<npp> Q_val = libff::Fr<npp>(12) * libff::G1<npp>::one();
    const libff::Fr<npp> scalar_val = libff::Fr<npp>(127);
    const libff::G1<npp> result_val = scalar_val * P_val;

    // Valid case
    {
        // Circuit
        protoboard<libff::Fr<wpp>> pb;
        G1_variable<wpp> P(pb, "P");
        G1_variable<wpp> result(pb, "result");
        G1_mul_by_const_scalar_gadget<wpp, libff::Fr<npp>::num_limbs>
            mul_gadget(pb, scalar_val.as_bigint(), P, result, "mul_gadget");

        mul_gadget.generate_r1cs_constraints();

        // Witness the input, gadget AND output
        P.generate_r1cs_witness(P_val);
        mul_gadget.generate_r1cs_witness();
        result.generate_r1cs_witness(result_val);
        ASSERT_TRUE(pb.is_satisfied());
    }

    // Invalid case. Use the gadget to ensure a specific value in the result,
    // by assigning the expected value after the gadget.
    {
        // Circuit
        protoboard<libff::Fr<wpp>> pb;
        G1_variable<wpp> P(pb, "P");
        G1_variable<wpp> result(pb, "result");
        G1_mul_by_const_scalar_gadget<wpp, libff::Fr<npp>::num_limbs>
            mul_gadget(pb, scalar_val.as_bigint(), P, result, "mul_gadget");

        mul_gadget.generate_r1cs_constraints();

        // Witness the input, gadget AND (invalid) output
        P.generate_r1cs_witness(Q_val);
        mul_gadget.generate_r1cs_witness();
        result.generate_r1cs_witness(result_val);
        ASSERT_FALSE(pb.is_satisfied());
    }
}

TEST(TestCurveGadgets, G2MulByConstScalar)
{
    // Compute inputs and results
    const libff::G2<npp> P_val = libff::Fr<npp>(13) * libff::G2<npp>::one();
    const libff::Fr<npp> scalar_val = libff::Fr<npp>(127);
    const libff::G2<npp> expect_result_val = scalar_val * P_val;

    // Circuit
    protoboard<libff::Fr<wpp>> pb;
    G2_variable<wpp> P(pb, "P");
    G2_variable<wpp> result(pb, "result");
    G2_mul_by_const_scalar_gadget<wpp, libff::Fr<npp>::num_limbs> mul_gadget(
        pb, scalar_val.as_bigint(), P, result, "mul_gadget");

    mul_gadget.generate_r1cs_constraints();

    P.generate_r1cs_witness(P_val);
    mul_gadget.generate_r1cs_witness();

    ASSERT_TRUE(pb.is_satisfied());

    const libff::G2<npp> result_val = result.get_element();
    ASSERT_EQ(expect_result_val, result_val);
}

TEST(TestCurveGadgets, G2MulByConstScalarWithKnownResult)
{
    // Compute inputs and results
    const libff::G2<npp> P_val = libff::Fr<npp>(13) * libff::G2<npp>::one();
    const libff::G2<npp> Q_val = libff::Fr<npp>(12) * libff::G2<npp>::one();
    const libff::Fr<npp> scalar_val = libff::Fr<npp>(127);
    const libff::G2<npp> result_val = scalar_val * P_val;

    // Valid case
    {
        // Circuit
        protoboard<libff::Fr<wpp>> pb;
        G2_variable<wpp> P(pb, "P");
        G2_variable<wpp> result(pb, "result");
        G2_mul_by_const_scalar_gadget<wpp, libff::Fr<npp>::num_limbs>
            mul_gadget(pb, scalar_val.as_bigint(), P, result, "mul_gadget");

        mul_gadget.generate_r1cs_constraints();

        // Witness the input and output
        result.generate_r1cs_witness(result_val);
        P.generate_r1cs_witness(P_val);
        mul_gadget.generate_r1cs_witness();
        result.generate_r1cs_witness(result_val);
        ASSERT_TRUE(pb.is_satisfied());
    }

    // Invalid case
    {
        // Circuit
        protoboard<libff::Fr<wpp>> pb;
        G2_variable<wpp> P(pb, "P");
        G2_variable<wpp> result(pb, "result");
        G2_mul_by_const_scalar_gadget<wpp, libff::Fr<npp>::num_limbs>
            mul_gadget(pb, scalar_val.as_bigint(), P, result, "mul_gadget");

        mul_gadget.generate_r1cs_constraints();

        // Witness the input and output
        result.generate_r1cs_witness(result_val);
        P.generate_r1cs_witness(Q_val);
        mul_gadget.generate_r1cs_witness();
        result.generate_r1cs_witness(result_val);
        ASSERT_FALSE(pb.is_satisfied());
    }
}

TEST(TestCurveGadgets, G2EqualityGadget)
{
    // Compute inputs and results
    const libff::G2<npp> P_val = libff::Fr<npp>(13) * libff::G2<npp>::one();
    const libff::G2<npp> Q_val = libff::Fr<npp>(12) * libff::G2<npp>::one();

    // Circuit
    protoboard<libff::Fr<wpp>> pb;
    G2_variable<wpp> P(pb, "P");
    G2_variable<wpp> Q(pb, "Q");
    G2_equality_gadget<wpp> equality_gadget(pb, P, Q, "equality_gadget");

    equality_gadget.generate_r1cs_constraints();

    // P == Q case
    P.generate_r1cs_witness(P_val);
    Q.generate_r1cs_witness(P_val);
    equality_gadget.generate_r1cs_witness();
    ASSERT_TRUE(pb.is_satisfied());

    // P != Q case
    P.generate_r1cs_witness(P_val);
    Q.generate_r1cs_witness(Q_val);
    equality_gadget.generate_r1cs_witness();
    ASSERT_FALSE(pb.is_satisfied());
}

template<
    typename ppT,
    typename groupT,
    typename groupVarT,
    typename groupVarOrIdentityT,
    typename scalarMulGadgetT>
void test_mul_by_scalar_gadget(
    const libff::Fr<other_curve<ppT>> &base_scalar,
    const libff::Fr<other_curve<ppT>> &scalar)
{
    using nFr = libff::Fr<other_curve<ppT>>;

    // Compute input and expected result values

    groupT P_val = base_scalar * groupT::one();
    const groupT expect_result_val_p = scalar * P_val;
    P_val.to_affine_coordinates();

    std::cout << "scalar: ";
    scalar.print();
    std::cout << "\n";

    const size_t num_bits = nFr::num_bits;
    const libff::bigint<nFr::num_limbs> scalar_bi = scalar.as_bigint();
    for (size_t i = 0; i < num_bits; ++i) {
        std::cout << (scalar_bi.test_bit(i) ? "1" : "0");
    }
    std::cout << "\n";

    std::cout << "P: ";
    P_val.print();
    std::cout << "\n";

    // Circuit

    protoboard<libff::Fr<wpp>> pb;
    pb_variable<libff::Fr<wpp>> scalar_var;
    scalar_var.allocate(pb, "scalar_var");

    groupVarT P(pb, "P");
    groupVarOrIdentityT result_p(pb, "result_p");
    scalarMulGadgetT mul_gadget_p(pb, scalar_var, P, result_p, "mul_gadget_p");

    // Generate constraints

    mul_gadget_p.generate_r1cs_constraints();

    libff::Fr<wpp> w_scalar;
    fp_from_fp(w_scalar, scalar);

    pb.val(scalar_var) = w_scalar;
    P.generate_r1cs_witness(P_val);
    mul_gadget_p.generate_r1cs_witness();

    // Check result generated.

    ASSERT_EQ(expect_result_val_p, result_p.get_element());

    // Check circuit satisfaction and proof generation.

    ASSERT_TRUE(pb.is_satisfied());
    generate_and_check_proof<wpp>(pb);
}

TEST(TestCurveGadgets, G1MulScalarVar)
{
    auto test_g1_mul_by_scalar_gadget = test_mul_by_scalar_gadget<
        wpp,
        libff::G1<npp>,
        G1_variable<wpp>,
        G1_variable_or_identity<wpp>,
        G1_mul_by_scalar_gadget<wpp>>;

    auto test_g2_mul_by_scalar_gadget = test_mul_by_scalar_gadget<
        wpp,
        libff::G2<npp>,
        G2_variable<wpp>,
        G2_variable_or_identity<wpp>,
        G2_mul_by_scalar_gadget<wpp>>;

    test_g1_mul_by_scalar_gadget(libff::Fr<npp>(13), libff::Fr<npp>::zero());
    test_g1_mul_by_scalar_gadget(libff::Fr<npp>(13), libff::Fr<npp>(127));
    test_g1_mul_by_scalar_gadget(libff::Fr<npp>(13), -libff::Fr<npp>::one());

    test_g2_mul_by_scalar_gadget(libff::Fr<npp>(13), libff::Fr<npp>::zero());
    test_g2_mul_by_scalar_gadget(libff::Fr<npp>(13), libff::Fr<npp>(127));
    test_g2_mul_by_scalar_gadget(libff::Fr<npp>(13), -libff::Fr<npp>::one());
}

} // namespace

int main(int argc, char **argv)
{
    libff::bls12_377_pp::init_public_params();
    libff::bw6_761_pp::init_public_params();
    libff::mnt4_pp::init_public_params();
    libff::mnt6_pp::init_public_params();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
