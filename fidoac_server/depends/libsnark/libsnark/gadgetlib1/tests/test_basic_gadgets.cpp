/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include "libsnark/gadgetlib1/gadgets/basic_gadgets.hpp"

#include <gtest/gtest.h>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>

namespace libsnark
{

template<typename FieldT> void test_disjunction_gadget(const size_t n)
{
    printf("testing disjunction_gadget on all %zu bit strings\n", n);

    protoboard<FieldT> pb;
    pb_variable_array<FieldT> inputs;
    inputs.allocate(pb, n, "inputs");

    pb_variable<FieldT> output;
    output.allocate(pb, "output");

    disjunction_gadget<FieldT> d(pb, inputs, output, "d");
    d.generate_r1cs_constraints();

    for (size_t w = 0; w < 1ul << n; ++w) {
        for (size_t j = 0; j < n; ++j) {
            pb.val(inputs[j]) = FieldT((w & (1ul << j)) ? 1 : 0);
        }

        d.generate_r1cs_witness();

#ifdef DEBUG
        printf("positive test for %zu\n", w);
#endif
        ASSERT_EQ(pb.val(output), (w ? FieldT::one() : FieldT::zero()));
        ASSERT_TRUE(pb.is_satisfied());

#ifdef DEBUG
        printf("negative test for %zu\n", w);
#endif
        pb.val(output) = (w ? FieldT::zero() : FieldT::one());
        ASSERT_FALSE(pb.is_satisfied());
    }

    libff::print_time("disjunction tests successful");
}

template<typename FieldT> void test_conjunction_gadget(const size_t n)
{
    printf("testing conjunction_gadget on all %zu bit strings\n", n);

    protoboard<FieldT> pb;
    pb_variable_array<FieldT> inputs;
    inputs.allocate(pb, n, "inputs");

    pb_variable<FieldT> output;
    output.allocate(pb, "output");

    conjunction_gadget<FieldT> c(pb, inputs, output, "c");
    c.generate_r1cs_constraints();

    for (size_t w = 0; w < 1ul << n; ++w) {
        for (size_t j = 0; j < n; ++j) {
            pb.val(inputs[j]) =
                (w & (1ul << j)) ? FieldT::one() : FieldT::zero();
        }

        c.generate_r1cs_witness();

#ifdef DEBUG
        printf("positive test for %zu\n", w);
#endif
        ASSERT_EQ(
            pb.val(output),
            (w == (1ul << n) - 1 ? FieldT::one() : FieldT::zero()));
        ASSERT_TRUE(pb.is_satisfied());

#ifdef DEBUG
        printf("negative test for %zu\n", w);
#endif
        pb.val(output) = (w == (1ul << n) - 1 ? FieldT::zero() : FieldT::one());
        ASSERT_FALSE(pb.is_satisfied());
    }

    libff::print_time("conjunction tests successful");
}

template<typename FieldT> void test_comparison_gadget(const size_t n)
{
    printf("testing comparison_gadget on all %zu bit inputs\n", n);

    protoboard<FieldT> pb;

    pb_variable<FieldT> A, B, less, less_or_eq;
    A.allocate(pb, "A");
    B.allocate(pb, "B");
    less.allocate(pb, "less");
    less_or_eq.allocate(pb, "less_or_eq");

    comparison_gadget<FieldT> cmp(pb, n, A, B, less, less_or_eq, "cmp");
    cmp.generate_r1cs_constraints();

    for (size_t a = 0; a < 1ul << n; ++a) {
        for (size_t b = 0; b < 1ul << n; ++b) {
            pb.val(A) = FieldT(a);
            pb.val(B) = FieldT(b);

            cmp.generate_r1cs_witness();

#ifdef DEBUG
            printf("positive test for %zu < %zu\n", a, b);
#endif
            ASSERT_EQ(pb.val(less), (a < b ? FieldT::one() : FieldT::zero()));
            ASSERT_EQ(
                pb.val(less_or_eq), (a <= b ? FieldT::one() : FieldT::zero()));
            ASSERT_TRUE(pb.is_satisfied());
        }
    }

    libff::print_time("comparison tests successful");
}

template<typename FieldT> void test_inner_product_gadget(const size_t n)
{
    printf("testing inner_product_gadget on all %zu bit strings\n", n);

    protoboard<FieldT> pb;
    pb_variable_array<FieldT> A;
    A.allocate(pb, n, "A");
    pb_variable_array<FieldT> B;
    B.allocate(pb, n, "B");

    pb_variable<FieldT> result;
    result.allocate(pb, "result");

    inner_product_gadget<FieldT> g(pb, A, B, result, "g");
    g.generate_r1cs_constraints();

    for (size_t i = 0; i < 1ul << n; ++i) {
        for (size_t j = 0; j < 1ul << n; ++j) {
            size_t correct = 0;
            for (size_t k = 0; k < n; ++k) {
                pb.val(A[k]) =
                    (i & (1ul << k) ? FieldT::one() : FieldT::zero());
                pb.val(B[k]) =
                    (j & (1ul << k) ? FieldT::one() : FieldT::zero());
                correct += ((i & (1ul << k)) && (j & (1ul << k)) ? 1 : 0);
            }

            g.generate_r1cs_witness();
#ifdef DEBUG
            printf("positive test for (%zu, %zu)\n", i, j);
#endif
            ASSERT_EQ(pb.val(result), FieldT(correct));
            ASSERT_TRUE(pb.is_satisfied());
            libff::UNUSED(correct);

#ifdef DEBUG
            printf("negative test for (%zu, %zu)\n", i, j);
#endif
            pb.val(result) = FieldT(100 * n + 19);
            ASSERT_FALSE(pb.is_satisfied());
        }
    }

    libff::print_time("inner_product_gadget tests successful");
}

template<typename FieldT> void test_loose_multiplexing_gadget(const size_t n)
{
    printf(
        "testing loose_multiplexing_gadget on 2**%zu pb_variable<FieldT> array "
        "inputs\n",
        n);
    protoboard<FieldT> pb;

    pb_variable_array<FieldT> arr;
    arr.allocate(pb, 1ul << n, "arr");
    pb_variable<FieldT> index, result, success_flag;
    index.allocate(pb, "index");
    result.allocate(pb, "result");
    success_flag.allocate(pb, "success_flag");

    loose_multiplexing_gadget<FieldT> g(
        pb, arr, index, result, success_flag, "g");
    g.generate_r1cs_constraints();

    for (size_t i = 0; i < 1ul << n; ++i) {
        pb.val(arr[i]) = FieldT((19 * i) % (1ul << n));
    }

    for (int idx = -1; idx <= (int)(1ul << n); ++idx) {
        pb.val(index) = FieldT(idx);
        g.generate_r1cs_witness();

        if (0 <= idx && idx <= (int)(1ul << n) - 1) {
            printf("demuxing element %d (in bounds)\n", idx);
            ASSERT_EQ(pb.val(result), FieldT((19 * idx) % (1ul << n)));
            ASSERT_EQ(pb.val(success_flag), FieldT::one());
            ASSERT_TRUE(pb.is_satisfied());
            pb.val(result) -= FieldT::one();
            ASSERT_FALSE(pb.is_satisfied());
        } else {
            printf("demuxing element %d (out of bounds)\n", idx);
            ASSERT_EQ(pb.val(success_flag), FieldT::zero());
            ASSERT_TRUE(pb.is_satisfied());
            pb.val(success_flag) = FieldT::one();
            ASSERT_FALSE(pb.is_satisfied());
        }
    }
    printf("loose_multiplexing_gadget tests successful\n");
}

TEST(BasicGadgetTests, TestDisjuctionGadget)
{
    test_disjunction_gadget<libff::alt_bn128_Fr>(10);
}

TEST(BasicGadgetTests, TestConjuectionGadget)
{
    test_conjunction_gadget<libff::alt_bn128_Fr>(10);
}

TEST(BasicGadgetTests, TestComparisonGadget)
{
    test_comparison_gadget<libff::alt_bn128_Fr>(4);
}

TEST(BasicGadgetTests, TestInnerProductGadget)
{
    test_inner_product_gadget<libff::alt_bn128_Fr>(4);
}

TEST(BasicGadgetTests, TestLooseMultiplexingGadget)
{
    test_loose_multiplexing_gadget<libff::alt_bn128_Fr>(4);
}

} // namespace libsnark

int main(int argc, char **argv)
{
    libff::alt_bn128_pp::init_public_params();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
