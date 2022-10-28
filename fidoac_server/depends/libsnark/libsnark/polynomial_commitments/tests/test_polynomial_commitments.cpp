/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <algorithm>
#include <gtest/gtest.h>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>
#include <libsnark/polynomial_commitments/bdfg21.hpp>
#include <libsnark/polynomial_commitments/kzg10.hpp>
#include <libsnark/polynomial_commitments/kzg10_batched.hpp>
#include <libsnark/polynomial_commitments/tests/polynomial_commitment_test_utils.hpp>

const size_t MAX_DEGREE = 254;

namespace libsnark
{

template<typename ppT, typename scheme> void test_basic_commitment()
{
    const polynomial<libff::Fr<ppT>> phi = gen_polynomial<ppT>(MAX_DEGREE);
    const polynomial<libff::Fr<ppT>> phi_2 = gen_polynomial<ppT>(MAX_DEGREE);
    const typename scheme::srs srs = scheme::setup(MAX_DEGREE);
    const typename scheme::commitment C = scheme::commit(srs, phi);

    ASSERT_TRUE(scheme::verify_poly(srs, C, phi));
    ASSERT_FALSE(scheme::verify_poly(srs, C, phi_2));
}

template<typename ppT, typename scheme> void test_eval_commitment()
{
    using Field = libff::Fr<ppT>;

    const polynomial<Field> phi = gen_polynomial<ppT>(MAX_DEGREE);
    const polynomial<Field> phi_2 = gen_polynomial<ppT>(MAX_DEGREE);
    const typename scheme::srs srs = scheme::setup(MAX_DEGREE);
    const typename scheme::commitment C = scheme::commit(srs, phi);
    const Field i = Field::random_element();
    const Field eval = scheme::evaluate_polynomial(phi, i);
    const typename scheme::evaluation_witness eval_witness =
        scheme::create_evaluation_witness(phi, i, eval, srs);

    ASSERT_TRUE(scheme::verify_evaluation(i, eval, srs, eval_witness, C));
    ASSERT_FALSE(scheme::verify_evaluation(
        i + Field::one(), eval, srs, eval_witness, C));
    ASSERT_FALSE(scheme::verify_evaluation(
        i, eval + Field::one(), srs, eval_witness, C));
}

template<typename ppT> void test_kzg10_commitment_with_known_secret()
{
    using Field = libff::Fr<ppT>;
    using scheme = kzg10<ppT>;

    // Dummy polynomial
    //   phi(x) = -1 + x + 2x^2 + 3x^3
    //
    //   phi(x) - phi(i) = (x - i) + 2(x^2 - i^2) + 3(x^3 - i^3)
    //   phi(x) - phi(i)/(x - i) =
    //     (x - i)/(x - i) + 2(x^2 - i^2)/(x - i) + 3(x^3 - i^3)/(x - i)
    //   = 1 + 2(x + i) + 3(x^2 + ix + i^2)
    //   = (1 + 2i + 3i^2) + (2 + 3i)x + 3x^2

    // Dummy secret and evaluation point
    //   alpha = 10
    //   i = 2
    //
    //   phi(alpha) = -1 + 10 + 200 + 3000 = 3209
    //   phi(i) = -1 + 2 + 8 + 24 = 33
    //
    //   psi(x) = (phi(x) - phi(i)/(x - i)
    //          = 17 + 8x + 3x^2
    //   psi(alpha) = 17 + 80 + 300 = 397

    const Field alpha = Field(10);
    const Field i = Field(2);
    const polynomial<Field> phi = {-Field("1"), Field(1), Field(2), Field(3)};

    // Check the srs
    const typename scheme::srs srs = scheme::setup_from_secret(16, alpha);
    ASSERT_EQ(libff::G1<ppT>::one(), srs.alpha_powers_g1[0]);
    ASSERT_EQ(Field(10) * libff::G1<ppT>::one(), srs.alpha_powers_g1[1]);
    ASSERT_EQ(Field(100) * libff::G1<ppT>::one(), srs.alpha_powers_g1[2]);
    ASSERT_EQ(Field(1000) * libff::G1<ppT>::one(), srs.alpha_powers_g1[3]);
    ASSERT_EQ(alpha * libff::G2<ppT>::one(), srs.alpha_g2);

    // Check the commitment
    const typename scheme::commitment C = scheme::commit(srs, phi);
    ASSERT_EQ(Field(3209) * libff::G1<ppT>::one(), C);

    // Check the evaluation with witness
    const Field eval = scheme::evaluate_polynomial(phi, i);
    const typename scheme::evaluation_witness eval_witness =
        scheme::create_evaluation_witness(phi, i, eval, srs);

    ASSERT_EQ(Field(33), eval);
    ASSERT_EQ(Field(397) * libff::G1<ppT>::one(), eval_witness);
}

template<typename ppT> void test_kzg10_batched_2_point()
{
    using Field = libff::Fr<ppT>;
    using scheme = kzg10<ppT>;
    using batch_scheme = kzg10_batched_2_point<ppT>;

    static const size_t MAX_DEGREE_MULTI = 8;
    const Field secret = Field(7);

    // Generate 2 sets of polynomials
    const std::vector<polynomial<Field>> fs{{
        {{1, 2, 3, 4, 5, 6, 7, 8}},
        {{11, 12, 13, 14, 15, 16, 17, 18}},
        {{21, 22, 23, 24, 25, 26, 27, 28}},
        {{31, 32, 33, 34, 35, 36, 37, 38}},
    }};

    const std::vector<polynomial<Field>> gs{{
        {{71, 72, 73, 74, 75, 76, 77, 78}},
        {{81, 82, 83, 84, 85, 86, 87, 88}},
        {{91, 92, 93, 94, 95, 96, 97, 98}},
    }};

    // srs
    const typename scheme::srs srs =
        scheme::setup_from_secret(MAX_DEGREE_MULTI, secret);

    // commitments
    std::vector<typename scheme::commitment> cm_1s;
    cm_1s.reserve(fs.size());
    for (const polynomial<Field> &f : fs) {
        cm_1s.push_back(scheme::commit(srs, f));
    }
    ASSERT_EQ(fs.size(), cm_1s.size());

    std::vector<typename scheme::commitment> cm_2s;
    cm_2s.reserve(gs.size());
    for (const polynomial<Field> &g : gs) {
        cm_2s.push_back(scheme::commit(srs, g));
    }
    ASSERT_EQ(gs.size(), gs.size());

    // Evaluation points
    const Field z_1("123");
    const Field z_2("456");

    // Evaluations
    const typename batch_scheme::evaluations evaluations =
        batch_scheme::evaluate_polynomials(fs, gs, z_1, z_2);
    ASSERT_EQ(fs.size(), evaluations.s_1s.size());
    ASSERT_EQ(gs.size(), evaluations.s_2s.size());

    // Verifier's random challenges
    const Field gamma_1(54321);
    const Field gamma_2(98760);

    // Witness for evaluations
    const typename batch_scheme::evaluation_witness witness =
        batch_scheme::create_evaluation_witness(
            fs, gs, z_1, z_2, evaluations, srs, gamma_1, gamma_2);

    // Check evaluations are correct.
    {
        // Naively evaluate the h_1(X) and h_2(X) polynomials at the secret x.
        // W_1 and W_2 should be these values encoded in G1.

        Field h_1_x = Field::zero();
        for (size_t i = 0; i < fs.size(); ++i) {
            const polynomial<Field> &f_i = fs[i];
            const Field f_x_minus_f_z_1 =
                libfqfft::evaluate_polynomial(f_i.size(), f_i, secret) -
                libfqfft::evaluate_polynomial(f_i.size(), f_i, z_1);
            const Field gamma_power = gamma_1 ^ i;
            h_1_x += gamma_power * f_x_minus_f_z_1 * ((secret - z_1).inverse());
        }
        ASSERT_EQ(h_1_x * libff::G1<ppT>::one(), witness.W_1);

        Field h_2_x = Field::zero();
        for (size_t i = 0; i < gs.size(); ++i) {
            const polynomial<Field> &g_i = gs[i];
            const Field g_x_minus_g_z_2 =
                libfqfft::evaluate_polynomial(g_i.size(), g_i, secret) -
                libfqfft::evaluate_polynomial(g_i.size(), g_i, z_2);
            const Field gamma_power = gamma_2 ^ i;
            h_2_x += gamma_power * g_x_minus_g_z_2 * ((secret - z_2).inverse());
        }
        ASSERT_EQ(h_2_x * libff::G1<ppT>::one(), witness.W_2);
    }

    // Verify the witnesses
    const Field r = Field(23546);
    ASSERT_TRUE(batch_scheme::verify_evaluations(
        z_1,
        z_2,
        evaluations,
        srs,
        gamma_1,
        gamma_2,
        witness,
        cm_1s,
        cm_2s,
        r));
}

template<typename ppT> void test_polynomial_inplace_operations()
{
    using Field = libff::Fr<ppT>;

    const std::vector<polynomial<Field>> f_set{
        {1, 2, 3, 4, 5, 6, 7, 8},
        {11, 12, 0, 14, 15, 16, 17},
    };

    {
        polynomial<Field> A{1, 2, 3, 4, 5, 6, 7, 8};
        const polynomial<Field> B{11, 12, 0, 14, 15, 16, 17};
        const polynomial<Field> expect{12, 14, 3, 18, 20, 22, 24, 8};
        internal::polynomial_add_inplace(A, B);
        ASSERT_EQ(expect, A);
    }
    {
        const polynomial<Field> A{1, 2, 3, 4, 5, 6, 7, 8};
        polynomial<Field> B{11, 12, 0, 14, 15, 16, 17};
        const polynomial<Field> expect{12, 14, 3, 18, 20, 22, 24, 8};
        internal::polynomial_add_inplace(B, A);
        ASSERT_EQ(expect, B);
    }

    {
        polynomial<Field> A{1, 2, 3, 4, 5, 6, 7, 8};
        const polynomial<Field> expect{3, 6, 9, 12, 15, 18, 21, 24};
        internal::polynomial_scalar_mul_inplace(A, Field(3));
        ASSERT_EQ(expect, A);
    }
}

template<typename ppT> void test_polynomial_accumulate_with_power_factors()
{
    using Field = libff::Fr<ppT>;

    // Given polynomials {f_1, ..., f_n} and some factors \alpha and \beta,
    // check that polynomial_accumulate_with_power_factors returns the
    // polynomial:
    //
    //   \alpha * \sum_{i=1}^n \beta^{i-1} f_i

    // Single polynomial edge case
    {
        const std::vector<polynomial<Field>> polynomials = {{
            {1, 2, 3, 4}, // 1 + 2x + 2x^2 + 3x^3
        }};
        const Field alpha(21);
        const Field beta(29);

        const polynomial<Field> expect_result = {
            alpha * 1,
            alpha * 2,
            alpha * 3,
            alpha * 4,
        };

        ASSERT_EQ(
            expect_result,
            internal::polynomial_accumulate_with_power_factors(
                polynomials, alpha, beta));
    }

    // Multiple polynomials
    {
        const std::vector<polynomial<Field>> polynomials = {{
            {1, 2, 3, 4}, // 1 + 2x + 2x^2 + 3x^3
            {5, 6, 7},
            {8, 9, 10, 11, 12},
        }};
        const Field alpha(21);
        const Field beta(29);

        //  (alpha + 5 alpha beta + 8 alpha beta^2) +
        //  (2 alpha + 6 alpha beta + 9 alpha beta^2) x +
        //  (3 alpha + 7 alpha beta + 10 alpha beta^2) x^2 +
        //  (4 alpha                + 11 alpha beta^2) x^3 +
        //  (                         12 alpha beta^2) x^4
        const polynomial<Field> expect_result = {
            alpha * (Field(1) + (beta * 5) + (beta * beta * 8)),
            alpha * (Field(2) + (beta * 6) + (beta * beta * 9)),
            alpha * (Field(3) + (beta * 7) + (beta * beta * 10)),
            alpha * (Field(4) + (beta * 0) + (beta * beta * 11)),
            alpha * (Field(0) + (beta * 0) + (beta * beta * 12)),
        };

        ASSERT_EQ(
            expect_result,
            internal::polynomial_accumulate_with_power_factors(
                polynomials, alpha, beta));
    }

    // Edge case
    {
        const std::vector<polynomial<Field>> f_set{
            {1, 2, 3, 4, 5, 6, 7, 8},
            {11, 12, 0, 14, 15, 16, 17},
        };
        const Field alpha(21);
        const Field beta(23);

        const polynomial<Field> expect{
            alpha * (Field(1) + beta * 11),
            alpha * (Field(2) + beta * 12),
            alpha * (Field(3) + beta * 0),
            alpha * (Field(4) + beta * 14),
            alpha * (Field(5) + beta * 15),
            alpha * (Field(6) + beta * 16),
            alpha * (Field(7) + beta * 17),
            alpha * (Field(8) + beta * 0)};

        const polynomial<Field> actual =
            internal::polynomial_accumulate_with_power_factors(
                f_set, alpha, beta);

        ASSERT_EQ(expect, actual);
    }
}

template<typename ppT> void test_compute_Z_T_minus_z_j_values()
{
    using Field = libff::Fr<ppT>;

    // Very simple case
    {
        const std::vector<Field> T{1, 2};
        const Field z(3);
        const std::vector<Field> expected_result{(3 - 2), (3 - 1)};
        ASSERT_EQ(
            expected_result,
            internal::compute_Z_T_minus_z_j_values<Field>(T, z));
    }

    // Slightly more complex case
    {
        const std::vector<Field> T{1, 2, 3, 4, 5, 6};
        const Field z(7);
        const std::vector<Field> expected_result{
            (7 - 2) * (7 - 3) * (7 - 4) * (7 - 5) * (7 - 6),
            (7 - 1) * (7 - 3) * (7 - 4) * (7 - 5) * (7 - 6),
            (7 - 1) * (7 - 2) * (7 - 4) * (7 - 5) * (7 - 6),
            (7 - 1) * (7 - 2) * (7 - 3) * (7 - 5) * (7 - 6),
            (7 - 1) * (7 - 2) * (7 - 3) * (7 - 4) * (7 - 6),
            (7 - 1) * (7 - 2) * (7 - 3) * (7 - 4) * (7 - 5),
        };
        ASSERT_EQ(
            expected_result,
            internal::compute_Z_T_minus_z_j_values<Field>(T, z));
    }
}

// Completely naive implementation of compute_bdfg21_f_minus_r_polynomial to
// compare results with the actual implementation.
template<typename FieldT>
polynomial<FieldT> naive_compute_bdfg21_f_minus_r_polynomial(
    const std::vector<polynomial<FieldT>> &f_set,
    const std::vector<FieldT> &evals,
    const FieldT &start_factor,
    const FieldT &factor)
{
    polynomial<FieldT> result;

    for (size_t i = 0; i < f_set.size(); ++i) {
        const FieldT alpha = start_factor * (factor ^ i);

        polynomial<FieldT> f_minus_f_eval;
        libfqfft::_polynomial_subtraction(f_minus_f_eval, f_set[i], {evals[i]});

        assert(f_minus_f_eval[0] == ((f_set[i])[0] - evals[i]));

        polynomial<FieldT> f_minus_f_eval_times_alpha;
        libfqfft::_polynomial_multiplication(
            f_minus_f_eval_times_alpha, f_minus_f_eval, {alpha});
        assert(f_minus_f_eval_times_alpha[0] == alpha * f_minus_f_eval[0]);

        // Inefficient, but completely avoids any ambiguitiy w.r.t. to
        // overlapping target / source polynomials.

        polynomial<FieldT> sum;
        libfqfft::_polynomial_addition(sum, result, f_minus_f_eval_times_alpha);
        result = sum;
    }

    return result;
}

template<typename ppT> void test_compute_bdfg21_f_minus_r_polynomial()
{
    using Field = libff::Fr<ppT>;

    const Field z_j = 63;
    const std::vector<polynomial<Field>> f_set{
        {1, 2, 3, 4, 5, 6, 7, 8},
        {11, 12, 0, 14, 15, 16, 17},
        {0, 22, 23, 24, 25},
        {31, 32, 33, 34, 35, 36, 0, 38, 39, 40, 41},
    };
    const std::vector<Field> evals{
        libfqfft::evaluate_polynomial(f_set[0].size(), f_set[0], z_j),
        libfqfft::evaluate_polynomial(f_set[1].size(), f_set[1], z_j),
        libfqfft::evaluate_polynomial(f_set[2].size(), f_set[2], z_j),
        libfqfft::evaluate_polynomial(f_set[3].size(), f_set[3], z_j),
    };

    const Field alpha(127);
    const Field beta(255);

    {
        // Sanity check. The results should match
        // polynomial_accumulate_with_power_factors if the evaluations are 0.
        // Test with the first 2 polynomials for convenience.

        std::vector<Field> expect_zero_constants{
            alpha * (Field(1) + beta * 11),
            alpha * (Field(2) + beta * 12),
            alpha * (Field(3) + beta * 0),
            alpha * (Field(4) + beta * 14),
            alpha * (Field(5) + beta * 15),
            alpha * (Field(6) + beta * 16),
            alpha * (Field(7) + beta * 17),
            alpha * (Field(8) + beta * 0)};
        ASSERT_EQ(
            expect_zero_constants,
            naive_compute_bdfg21_f_minus_r_polynomial(
                {f_set[0], f_set[1]}, std::vector<Field>{0, 0}, alpha, beta));
        ASSERT_EQ(
            expect_zero_constants,
            internal::polynomial_accumulate_with_power_factors(
                {f_set[0], f_set[1]}, alpha, beta));
        ASSERT_EQ(
            expect_zero_constants,
            internal::compute_bdfg21_f_minus_r_polynomial(
                {f_set[0], f_set[1]}, std::vector<Field>{0, 0}, alpha, beta));

        // Should match just the constant terms if the polynomials are 0.
        ASSERT_EQ(
            std::vector<Field>{-(alpha * evals[0]) - (beta * alpha * evals[1])},
            naive_compute_bdfg21_f_minus_r_polynomial(
                {{0}, {0}}, {evals[0], evals[1]}, alpha, beta));
        ASSERT_EQ(
            std::vector<Field>{-(alpha * evals[0]) - (beta * alpha * evals[1])},
            internal::compute_bdfg21_f_minus_r_polynomial(
                {{0}, {0}}, {evals[0], evals[1]}, alpha, beta));
    }

    // Compute the expected result naively.
    const polynomial<Field> expect_result =
        naive_compute_bdfg21_f_minus_r_polynomial(f_set, evals, alpha, beta);
    const polynomial<Field> result =
        internal::compute_bdfg21_f_minus_r_polynomial<Field>(
            f_set, evals, alpha, beta);

    // Check both polynomials have some simple required properties (just in
    // case the equality check later fails).

    ASSERT_FALSE(libfqfft::_is_zero(expect_result));
    ASSERT_EQ(
        Field::zero(),
        libfqfft::evaluate_polynomial(
            expect_result.size(), expect_result, z_j));

    ASSERT_FALSE(libfqfft::_is_zero(result));
    ASSERT_EQ(
        Field::zero(),
        libfqfft::evaluate_polynomial(result.size(), result, z_j));

    ASSERT_EQ(expect_result, result);
}

template<typename ppT> void test_bdfg21()
{
    using Field = libff::Fr<ppT>;
    using scheme = kzg10<ppT>;
    using batch_scheme = bdfg21<ppT>;

    static const size_t MAX_DEGREE_MULTI = 8;
    const Field secret = Field(7);

    // Generate 3 sets of polynomials, where each set is to be evaluated at one
    // point.
    const std::vector<std::vector<polynomial<Field>>> f_sets{
        {
            {{1, 2, 3, 4, 5, 6, 7, 8}},
            {{11, 12, 13, 14, 15, 16, 17, 18}},
            {{21, 22, 23, 24, 25, 26, 27, 28}},
            {{31, 32, 33, 34, 35, 36, 37, 38}},
        },
        {
            {{71, 72, 73, 74, 75, 76, 77, 78}},
            {{81, 82, 83, 84, 85, 86, 87, 88}},
            {{91, 92, 93, 94, 95, 96, 97, 98}},
        },
        {
            {{41, 42, 43, 44, 45, 46, 47, 48}},
            {{51, 52, 53, 54, 55, 56, 57, 58}},
            {{61, 62, 63, 64, 65, 66, 67, 68}},
        },
    };

    // Evaluation points
    const std::vector<Field> z_s{
        Field("123"),
        Field("456"),
        Field("789"),
    };
    ASSERT_EQ(f_sets.size(), z_s.size());

    // Evaluations
    const typename batch_scheme::evaluations evaluations =
        batch_scheme::evaluate_polynomials(f_sets, z_s);
    ASSERT_EQ(f_sets.size(), evaluations.size());
    for (size_t i = 0; i < f_sets.size(); ++i) {
        const std::vector<polynomial<Field>> &f_set = f_sets[i];
        const std::vector<Field> &evals = evaluations[i];
        ASSERT_EQ(f_set.size(), evals.size());
    }

#if 0
    // Expected values only valid for bls12-377, computed in
    // libsnark/polynomial_commitments/polynomial_commitments.sage. Disabled by
    // default, but retained for debugging.
    {
        const std::vector<std::vector<Field>> expect_evaluations{
            {Field("3431830628027164"),
             Field("7726018695917564"),
             Field("12020206763807964"),
             Field("16314394831698364")},
            {Field("320471719126573875911"),
             Field("361558994677474468641"),
             Field("402646270228375061371")},
            {Field("9147861136168044621476"),
             Field("11053715926983267941076"),
             Field("12959570717798491260676")}};
        ASSERT_EQ(expect_evaluations, evaluations);
    }
#endif

    // Run the scheme

    const typename scheme::srs srs =
        scheme::setup_from_secret(MAX_DEGREE_MULTI, secret);

    // Compute polynomial commitments
    std::vector<std::vector<typename scheme::commitment>> cm_sets;
    cm_sets.reserve(f_sets.size());
    for (const std::vector<polynomial<Field>> &f_set : f_sets) {
        std::vector<typename scheme::commitment> cm_set;
        cm_set.reserve(f_set.size());

        for (const polynomial<Field> &f : f_set) {
            cm_set.push_back(scheme::commit(srs, f));
        }
        ASSERT_EQ(f_set.size(), cm_set.size());

        cm_sets.push_back(std::move(cm_set));
    }
    ASSERT_EQ(f_sets.size(), cm_sets.size());

    // Verifier's first challenge
    const Field gamma(54321);

    // Witness computation phase 1
    const typename batch_scheme::phase_1_output witness_phase_1 =
        batch_scheme::create_evaluation_witness_phase_1(
            f_sets, z_s, evaluations, srs, gamma);

#if 0
    // Expected values only valid for bls12-377, computed in
    // libsnark/polynomial_commitments/polynomial_commitments.sage. Disabled by
    // default, but retained for debugging.
    {
        const polynomial<Field> expect_f_over_Z_T{
            Field("676441491944216471798398784990466022356424142020487295908994"
                  "29"),
            Field(
                "85734029397239117226633870068185345343898799872829700374812"),
            Field("108661634217032818892793583038491674996106135933918480248"),
            Field("137720702429365435024122950420399222657744845553991245"),
            Field("174550953310110237824130513412737561501102571943535"),
            Field("221230268064867176725771659421732696326574889746"),
            Field("280043527041999666711238714561094658243797330"),
        };
        const libff::G1<ppT> expect_witness_phase_1(
            libff::Fq<ppT>("198268198105568738415379617548183096177834717064778"
                           "947082926486124"
                           "183904578002794150419211651921656245514219035756"),
            libff::Fq<ppT>("957000280654872567501698447184011143282240328311382"
                           "594826583232683"
                           "97569587248918366363233462871261831146144088638"),
            libff::Fq<ppT>::one());
        ASSERT_EQ(
            expect_f_over_Z_T, witness_phase_1.private_f_over_Z_T_polynomial);
        ASSERT_EQ(
            expect_witness_phase_1, witness_phase_1.public_witness_phase_1);
    }
#endif

    // Verifier's chosen evaluation point.
    const Field z(98765);

    // Final witness
    const typename batch_scheme::evaluation_witness witness =
        batch_scheme::create_evaluation_witness(
            f_sets, z_s, evaluations, srs, gamma, witness_phase_1, z);

    // Verify the final witnesses
    ASSERT_TRUE(batch_scheme::verify_evaluations(
        z_s, evaluations, srs, gamma, z, witness, cm_sets));
}

template<typename ppT> void test_for_curve()
{
    // Execute all tests for the given curve.

    ppT::init_public_params();

    test_basic_commitment<ppT, kzg10<ppT>>();
    test_eval_commitment<ppT, kzg10<ppT>>();
    test_kzg10_commitment_with_known_secret<ppT>();
    test_kzg10_batched_2_point<ppT>();
    test_polynomial_inplace_operations<ppT>();
    test_polynomial_accumulate_with_power_factors<ppT>();
    test_compute_Z_T_minus_z_j_values<ppT>();
    test_compute_bdfg21_f_minus_r_polynomial<ppT>();
    test_bdfg21<ppT>();
}

TEST(TestPolynomialCommitments, ALT_BN128)
{
    test_for_curve<libff::alt_bn128_pp>();
}

TEST(TestPolynomialCommitments, BLS12_377)
{
    test_for_curve<libff::bls12_377_pp>();
}

} // namespace libsnark
