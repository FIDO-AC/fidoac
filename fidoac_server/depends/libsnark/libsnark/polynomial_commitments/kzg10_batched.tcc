/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef __LIBSNARK_POLYNOMIAL_COMMITMENTS_KZG10_BATCHED_TCC__
#define __LIBSNARK_POLYNOMIAL_COMMITMENTS_KZG10_BATCHED_TCC__

#include "libsnark/polynomial_commitments/kzg10_batched.hpp"

namespace libsnark
{

namespace internal
{

template<typename FieldT>
static polynomial<FieldT> polynomial_scalar_mul(
    const polynomial<FieldT> &poly, const FieldT &factor)
{
    polynomial<FieldT> out;
    out.reserve(poly.size());
    for (const FieldT &c : poly) {
        out.push_back(c * factor);
    }
    return out;
}

template<typename FieldT>
static void polynomial_scalar_mul_inplace(
    polynomial<FieldT> &poly, const FieldT &factor)
{
    for (FieldT &c : poly) {
        c *= factor;
    }
}

template<typename FieldT>
static void polynomial_add_inplace(
    polynomial<FieldT> &poly, const polynomial<FieldT> &other)
{
    // If `other` is longer, resize `polynomial` and copy any trailing
    // coefficients. We then add in the coefficients from other which were not
    // copied.
    const size_t p_orig_size = poly.size();
    if (p_orig_size < other.size()) {
        poly.resize(other.size());
        std::copy(
            other.begin() + p_orig_size,
            other.end(),
            poly.begin() + p_orig_size);
        for (size_t i = 0; i < p_orig_size; ++i) {
            poly[i] += other[i];
        }

        return;
    }

    // Add coefficients of `other` into `polynomial`.
    for (size_t i = 0; i < other.size(); ++i) {
        poly[i] += other[i];
    }
}

template<typename FieldT>
static polynomial<FieldT> polynomial_accumulate_with_power_factors(
    const std::vector<polynomial<FieldT>> &polys, const FieldT &factor)
{
    const size_t t = polys.size();
    if (t == 1) {
        return polys[0];
    }

    // Start with
    //   f_accum = polys[t-2] + factor * polys[t-1].
    //
    // For each remaining polynomial f:
    //   f_accum <- factor * f_accum + f

    size_t i = t - 1;
    polynomial<FieldT> f_accum = polynomial_scalar_mul(polys[i], factor);
    polynomial_add_inplace(f_accum, polys[--i]);
    while (i > 0) {
        polynomial_scalar_mul_inplace(f_accum, factor);
        polynomial_add_inplace(f_accum, polys[--i]);
    }

    return f_accum;
}

} // namespace internal

template<typename ppT>
kzg10_batched_2_point<ppT>::evaluations::evaluations(
    std::vector<Field> &&s_1s, std::vector<Field> &&s_2s)
    : s_1s(s_1s), s_2s(s_2s)
{
}

template<typename ppT>
kzg10_batched_2_point<ppT>::evaluation_witness::evaluation_witness(
    const libff::G1<ppT> &W_1, const libff::G1<ppT> &W_2)
    : W_1(W_1), W_2(W_2)
{
}

template<typename ppT>
typename kzg10_batched_2_point<ppT>::evaluations kzg10_batched_2_point<ppT>::
    evaluate_polynomials(
        const std::vector<polynomial<Field>> &fs,
        const std::vector<polynomial<Field>> &gs,
        const Field &z_1,
        const Field &z_2)
{
    // Denote the numbers of polynomials as t1 and t2, consistent with [GWC19].
    const size_t t1 = fs.size();
    const size_t t2 = gs.size();

    std::vector<Field> s_1s;
    s_1s.reserve(t1);
    for (const polynomial<Field> &f_i : fs) {
        s_1s.push_back(libfqfft::evaluate_polynomial(f_i.size(), f_i, z_1));
    }

    std::vector<Field> s_2s;
    s_2s.reserve(t2);
    for (const polynomial<Field> &g_i : gs) {
        s_2s.push_back(libfqfft::evaluate_polynomial(g_i.size(), g_i, z_2));
    }

    return evaluations(std::move(s_1s), std::move(s_2s));
}

template<typename ppT>
typename kzg10_batched_2_point<ppT>::evaluation_witness kzg10_batched_2_point<
    ppT>::
    create_evaluation_witness(
        const std::vector<polynomial<Field>> &fs,
        const std::vector<polynomial<Field>> &gs,
        const Field &z_1,
        const Field &z_2,
        const evaluations &evaluations,
        const srs &srs,
        const Field &gamma_1,
        const Field &gamma_2)
{
    assert(fs.size() == evaluations.s_1s.size());
    assert(gs.size() == evaluations.s_2s.size());

    // For convenience of variable naming, let $f_i \in fs$ and $g_i in gs$ be
    // the two sets of polynomials. These are denoted $f_i$ and $f'_i$ in
    // [GWC19]. Similarly, $h_1$ and $h_2$ are used in place of $h$ and
    // $h'$ in the paper, and $\gamma_1$ and $\gamma_2$ in place of
    // $\gamma$ and $\gamma'$.
    //
    // To generate the witness we must compute:
    //
    //   h_1(X) = \sum_{i=1}^{t_1} \gamma_1^{i-1} (f_i(X) - f_i(z_1))/(X - z_1)
    //
    //   h_2(X) = \sum_{i=1}^{t_2} \gamma_2^{i-1} (g_i(X) - g_i(z_2))/(X - z_2)
    //
    // then evaluate:
    //
    //   W_1 = [h_1(x)]_1
    //   W_2 = [h_2(x)]_2
    //
    // This is computed as follows:
    //
    //   f_accum(X) = \sum_{i=1}^{t_1} \gamma_1^{i-1} f_i(X)
    //   f_accum_eval = f_accum(z_1)
    //   W_1 = kzg10::create_evaluation_witness(f_accum, z_1, f_accum_eval)
    //       = [ (f_accum(X) - f_accum(z_1)) / (X - z_1) ]_1
    //       = [ h_1(X) ]_1
    //
    // Similarly for $\gamma_2$, $g_i$s and $z_2$.
    //
    // The evaluations `f_accum_eval` and `g_accum_eval` of the accumulated
    // polynomials `f_accum` and `g_accum` are computed as:
    //
    //   s_1[0] + s_1[1] * gamma_1 + ... + s_1[t1 - 1] gamma_1^{t1 - 1}
    //
    // which can be viewed as the evaluation at `gamma_1` of the polynomial
    // with coefficients `s_1`.
    //
    // Note that this approach assumes that the highest degree of the
    // polynomials f_s and g_s is larger than the number of polynomials, and
    // therefore it is more efficient to reuse the existing evaluations rather
    // than evaluating the accumulated polynomials.

    const polynomial<Field> f_accum =
        internal::polynomial_accumulate_with_power_factors(fs, gamma_1);
    const libff::Fr<ppT> f_accum_eval =
        kzg10<ppT>::evaluate_polynomial(evaluations.s_1s, gamma_1);
    const libff::G1<ppT> f_accum_witness =
        kzg10<ppT>::create_evaluation_witness(f_accum, z_1, f_accum_eval, srs);

    const polynomial<Field> g_accum =
        internal::polynomial_accumulate_with_power_factors(gs, gamma_2);
    const libff::Fr<ppT> g_accum_eval =
        kzg10<ppT>::evaluate_polynomial(evaluations.s_2s, gamma_2);
    const libff::G1<ppT> g_accum_witness =
        kzg10<ppT>::create_evaluation_witness(g_accum, z_2, g_accum_eval, srs);

    return evaluation_witness(f_accum_witness, g_accum_witness);
}

template<typename ppT>
bool kzg10_batched_2_point<ppT>::verify_evaluations(
    const Field &z_1,
    const Field &z_2,
    const evaluations &evaluations,
    const srs &srs,
    const Field &gamma_1,
    const Field &gamma_2,
    const evaluation_witness &witness,
    const std::vector<commitment> &cm_1s,
    const std::vector<commitment> &cm_2s,
    const Field &r)
{
    // See Section 3, p13 of [GWC19].

    const size_t t1 = cm_1s.size();
    const size_t t2 = cm_2s.size();
    assert(t1 == evaluations.s_1s.size());
    assert(t2 == evaluations.s_2s.size());

    // Compute:
    //
    //   F = \sum_{i=1}^{t1} \gamma_1^{i-1} (cm_1[i] - [s_1[i]]_1) +      (G)
    //       r \sum_{i=1}^{t2} \gamma_2^{i-1} (cm_2[i] - [s_2[i]]_1)      (H)

    const std::vector<Field> &s_1s = evaluations.s_1s;
    const std::vector<Field> &s_2s = evaluations.s_2s;

    // Compute:
    //
    //   s_1_accum  = \sum_i \gamma_1^{i-1} s_1[i]    (in the scalar field)
    //   cm_1_accum = \sum_i \gamma_1^{i-1} cm_1[i]   (in G1)
    //   G          = cm_1_accum - s_1_accum * G1::one()

    Field s_1_accum = s_1s[t1 - 1];
    libff::G1<ppT> cm_1_accum = cm_1s[t1 - 1];
    // Note use of underflow to terminate after i = 0.
    for (size_t i = t1 - 2; i < t1; --i) {
        cm_1_accum = (gamma_1 * cm_1_accum) + cm_1s[i];
        s_1_accum = (s_1_accum * gamma_1) + s_1s[i];
    }
    const libff::G1<ppT> G = cm_1_accum - s_1_accum * libff::G1<ppT>::one();

    // Similarly:
    //
    //   s_2_accum  = \sum_i \gamma_2^{i-1} s_2[i]    (in the scalar field)
    //   cm_2_accum = \sum_i \gamma_2^{i-1} cm_2[i]   (in G1)
    //   H          = cm_2_accum - s_2_accum * G1::one()

    Field s_2_accum = s_2s[t2 - 1];
    libff::G1<ppT> cm_2_accum = cm_2s[t2 - 1];
    for (size_t i = t2 - 2; i < t2; --i) {
        cm_2_accum = gamma_2 * cm_2_accum + cm_2s[i];
        s_2_accum = (s_2_accum * gamma_2) + s_2s[i];
    }
    const libff::G1<ppT> H =
        r * (cm_2_accum - s_2_accum * libff::G1<ppT>::one());

    const libff::G1<ppT> F = G + H;

    // The pairing check takes the form:
    //
    //   e(F + z_1 * W_1 + r * z_2 * W_2, G2::one())
    //   * e(-W_1 - r * W_2, srs.alpha_g2) == 1
    //
    // (see p13 of [GWC19]).

    //   A = F + z_1 * W_1 + r * z_2 * W2
    //   B = G2::one()
    //   C = -W_1 - r * W_2
    //   D = srs.alpha_g2

    const libff::G1<ppT> &W_1 = witness.W_1;
    const libff::G1<ppT> &W_2 = witness.W_2;

    const libff::G1<ppT> r_W_2 = r * W_2;
    const libff::G1<ppT> A = F + z_1 * W_1 + z_2 * r_W_2;
    const libff::G1<ppT> C = -(W_1 + r_W_2);

    const libff::G1_precomp<ppT> _A = ppT::precompute_G1(A);
    const libff::G2_precomp<ppT> _B = ppT::precompute_G2(libff::G2<ppT>::one());
    const libff::G1_precomp<ppT> _C = ppT::precompute_G1(C);
    const libff::G2_precomp<ppT> _D = ppT::precompute_G2(srs.alpha_g2);

    const libff::Fqk<ppT> miller_result =
        ppT::double_miller_loop(_A, _B, _C, _D);
    const libff::GT<ppT> result = ppT::final_exponentiation(miller_result);
    return result == libff::GT<ppT>::one();
}

} // namespace libsnark

#endif // __LIBSNARK_POLYNOMIAL_COMMITMENTS_KZG10_BATCHED_TCC__
