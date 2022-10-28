/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef __LIBSNARK_POLYNOMIAL_COMMITMENTS_BDFG21_TCC__
#define __LIBSNARK_POLYNOMIAL_COMMITMENTS_BDFG21_TCC__

#include "libsnark/polynomial_commitments/bdfg21.hpp"
#include "libsnark/polynomial_commitments/kzg10_batched.hpp"

namespace libsnark
{

namespace internal
{

/// Convenience function to compute the polynomial:
///
///   start_factor * \sum_i factor^{i-1} polys[i]
template<typename FieldT>
static polynomial<FieldT> polynomial_accumulate_with_power_factors(
    const std::vector<polynomial<FieldT>> &polys,
    const FieldT &start_factor,
    const FieldT &factor)
{
    const size_t t = polys.size();
    if (t == 1) {
        if (start_factor != FieldT::one()) {
            return polynomial_scalar_mul(polys[0], start_factor);
        }
        return polys[0];
    }

    // Leverage the fact that t > 1 to avoid a copy:
    //
    //   f_accum <- factor * polys[t-1] + polys[t-2]
    //   for i = t-3, ... 0:
    //     f_accum <- factor * f_accum + polys[i]
    //   f_accum <- start_factor * f_accum

    size_t i = t - 1;
    polynomial<FieldT> f_accum = polynomial_scalar_mul(polys[i], factor);
    polynomial_add_inplace(f_accum, polys[--i]);

    while (i > 0) {
        polynomial_scalar_mul_inplace(f_accum, factor);
        polynomial_add_inplace(f_accum, polys[--i]);
    }

    if (start_factor != FieldT::one()) {
        polynomial_scalar_mul_inplace(f_accum, start_factor);
    }

    return f_accum;
}

/// Let $T = \{z_0, ...., z_{n-1}\}$ be a sequence of field elements, and $z$
/// another field element. Return a vector in which the i-th entry is:
///
///   (z - z_0) * ... * (z - z_{i-1}) * (z - z_{i+1}) * ... * (z - z_{n-1})
///
/// i.e. the evaluation at $z$ of the polynomial of degree $n-1$ which is zero
/// at all $z_j$ for $j \neq i$.
template<typename FieldT>
std::vector<FieldT> compute_Z_T_minus_z_j_values(
    const std::vector<FieldT> &T, const FieldT &z)
{
    const size_t num_entries = T.size();
    std::vector<FieldT> Y;
    Y.reserve(num_entries);

    // Compute:
    //
    //   Y = [1, (z-z_0), (z-z_0)(z-z_1), ..., (z-z_0)..(z-z_{n-2})]
    //
    //   Y[0] = 1
    //   for i = 1 to n-1:
    //     Y[i] = Y[i-1] * (z - T[i-1])

    Y.push_back(FieldT::one());
    for (size_t i = 1; i < num_entries; ++i) {
        Y.push_back(Y.back() * (z - T[i - 1]));
    }

    // Then, iterate through the vector in reverse:
    //
    //   zz <- 1
    //   for i = n-2, ... 0:
    //     zz <- zz * (z-z_{i+1})
    //     Y[i] <- Y[i] * zz;

    FieldT zz = FieldT::one();
    for (size_t i = num_entries - 2; i < num_entries; --i) {
        zz *= (z - T[i + 1]);
        Y[i] *= zz;
    }

    return Y;
}

/// Compute polynomials of the form:
///
///   H_J = start_factor * \sum_{i \in F_j} factor^{i-1} (f_i(X) - f_i(z_j))
///
/// where evals contains the precomputed $f_i(z_j)$ values. This also
/// corresponds to terms of the form:
///
///   start_factor * \sum_{i \in F_j} factor^{i-1} (f_i(X) - r_i(X))
///
/// since $r_i(X)$ (the polynomial which attains $f_i(z_j)$ at $z_j$ is a
/// constant.
template<typename FieldT>
std::vector<FieldT> compute_bdfg21_f_minus_r_polynomial(
    const std::vector<polynomial<FieldT>> &f_set,
    const std::vector<FieldT> &evals,
    const FieldT &start_factor,
    const FieldT &factor)
{
    const size_t num_polys = f_set.size();
    assert(evals.size() == num_polys);

    // Compute
    //
    //   A(X) = start_factor * \sum_{i \in F_j} factor^{i-1} f_i(X)

    polynomial<FieldT> A = internal::polynomial_accumulate_with_power_factors(
        f_set, start_factor, factor);

    // Compute
    //
    //   B = start_factor * \sum_{i \in F_j} factor^{i-1} f_i(z_j)

    FieldT alpha = start_factor;
    FieldT B = alpha * evals[0];

    for (size_t i = 1; i < num_polys; ++i) {
        alpha *= factor;
        B += alpha * evals[i];
    }

    // Compute (A)-(B)
    A[0] -= B;
    return A;
}

} // namespace internal

template<typename ppT>
bdfg21<ppT>::phase_1_output::phase_1_output(
    const libff::G1<ppT> &witness_phase_1,
    polynomial<Field> &&f_over_Z_T_polynomial)
    : public_witness_phase_1(witness_phase_1)
    , private_f_over_Z_T_polynomial(f_over_Z_T_polynomial)
{
}

template<typename ppT>
bdfg21<ppT>::evaluation_witness::evaluation_witness(
    const libff::G1<ppT> &W, const libff::G1<ppT> &W_prime)
    : W(W), W_prime(W_prime)
{
}

template<typename ppT>
typename bdfg21<ppT>::evaluations bdfg21<ppT>::evaluate_polynomials(
    const std::vector<std::vector<polynomial<Field>>> &f_sets,
    const std::vector<Field> z_s)
{
    assert(f_sets.size() == z_s.size());

#ifdef MULTICORE
#pragma omp parallel for
#endif
    evaluations evals;
    evals.resize(z_s.size());
    for (size_t i = 0; i < z_s.size(); ++i) {
        const std::vector<polynomial<Field>> &f_set = f_sets[i];
        const Field &z = z_s[i];

        std::vector<Field> z_evals;
        z_evals.reserve(f_set.size());
        for (const polynomial<Field> f : f_set) {
            z_evals.push_back(libfqfft::evaluate_polynomial(f.size(), f, z));
        }

        assert(z_evals.size() == f_set.size());
        evals[i] = std::move(z_evals);
    }

    return evals;
}

template<typename ppT>
typename bdfg21<ppT>::phase_1_output bdfg21<ppT>::
    create_evaluation_witness_phase_1(
        const std::vector<std::vector<polynomial<Field>>> &f_sets,
        const std::vector<Field> &T,
        const typename bdfg21<ppT>::evaluations &evaluations,
        const typename bdfg21<ppT>::srs &srs,
        const libff::Fr<ppT> &gamma)
{
    assert(f_sets.size() == T.size());
    assert(evaluations.size() == T.size());

    // See Section 4.1 [BDFG21].
    //
    // The algorithm in the paper requires that the polynomial:
    //
    //   f = \sum_{i = 1}^k \gamma^{i-1} Z_{T\S_i} (f_i - r_i)
    //
    // is computed, and then divided by $Z_T$. The first part of the witness is
    // then computed as $[ (f/Z_T)(x) ]_1$, and the polynomial $f$ is reused in
    // a later phase.
    //
    // In our setting, for each $j = 1, ..., |T|$, let $z_j \in T$ be the
    // $j$-th point in $T$, and let $F_j$ be the set of indices $i$ of
    // polynomials s.t. $f_i$ is to be evaluated at $z_j$.
    //
    // Then f can be written as:
    //
    //   f = \sum_j \sum_{i \in F_j} \gamma^{i-1} Z_{T\S_i} (f_i - r_i)
    //
    // and f/Z_T as:
    //
    //   f/Z_T =
    //     \sum_j \sum_{i \in F_j} \gamma^{i-1} (f_i - r_i) Z_{T\S_i} / Z_T
    //
    // Note that for $i \in F_j$ we have:
    //
    //   Z_{T\S_i}(X) / Z_T(X) = 1 / (X - z_j)
    //
    // and therefore we can compute (f / Z_T)(X) as:
    //
    //   (f/Z_T)(X) = \sum_j G_j(X)
    //
    // where
    //
    //   G_j(X) = H_j(X) / (X - z_j)
    //   H_j(X) = \sum_{i \in F_j} \gamma^{i-1} (f_i(X) - f_i(z_j))

    polynomial<Field> f_over_Z_T{0};
    Field gamma_power = Field::one();

    for (size_t j = 0; j < T.size(); ++j) {
        const Field &z_j = T[j];

        // Compute the polynomial H_j and G_j = H_j / (X - z_j) polynomials
        // (see above)

        polynomial<Field> H_j = internal::compute_bdfg21_f_minus_r_polynomial(
            f_sets[j], evaluations[j], gamma_power, gamma);
        std::vector<Field> G_j;
        std::vector<Field> remainder;
        libfqfft::_polynomial_division(G_j, remainder, H_j, {-z_j, 1});
        assert(libfqfft::_is_zero(remainder));

        // Update gamma_power
        for (size_t i = 0; i < f_sets[j].size(); ++i) {
            gamma_power *= gamma;
        }

        // The polynomial f is the sum of the g_j's
        internal::polynomial_add_inplace(f_over_Z_T, G_j);
    }

    const libff::G1<ppT> f_over_Z_T_witness =
        kzg10<ppT>::commit(srs, f_over_Z_T);
    return phase_1_output(f_over_Z_T_witness, std::move(f_over_Z_T));
}

template<typename ppT>
typename bdfg21<ppT>::evaluation_witness bdfg21<ppT>::create_evaluation_witness(
    const std::vector<std::vector<polynomial<Field>>> &f_sets,
    const std::vector<Field> &T,
    const evaluations &evaluations,
    const srs &srs,
    const Field &gamma,
    const phase_1_output &phase_1_out,
    const Field &z)
{
    assert(f_sets.size() == T.size());
    assert(f_sets.size() == evaluations.size());

    // See Section 4.1 [BDFG21].
    //
    // Compute the polynomial
    //
    //   L(X)
    //   = \sum_{i=1}^k \gamma^{i-1} Z_{T\S_i}(z) (f_i(X) - r_i(z))
    //     - Z_T(z) (f / Z_T)(X)
    //
    // $L$ has a root at $z$ and is therefore divisble by $(X - z)$. Compute
    // $[ (L / (x - z))(x) ]_1$, the second part of the evaluation witness.

    // Note that, using the notation of create_evaluation_witness_phase_1, this
    // is equal to:
    //
    //   L(X) = \sum_j Z_{T\{z_j}}(z)
    //            \sum_{i \in F_j} \gamma^{i-1} (f_i(X) - f_i(z_i))
    //          - Z_T(z) (f / Z_T)(X)

    // Compute the values Z_{T\{z_j}}(z)
    const std::vector<Field> Z_T_minus_z_j_at_z =
        internal::compute_Z_T_minus_z_j_values(T, z);
    const Field Z_T_at_z = Z_T_minus_z_j_at_z[0] * (z - T[0]);
    assert(Z_T_at_z == Z_T_minus_z_j_at_z[1] * (z - T[1]));

    // Compute L
    Field gamma_power(1);
    polynomial<Field> L = internal::polynomial_scalar_mul(
        phase_1_out.private_f_over_Z_T_polynomial, -Z_T_at_z);

    for (size_t j = 0; j < T.size(); ++j) {

        // Compute intermediate polynomials:
        //
        //   H(X) = Z_{T\{z_j}}(z) *
        //            \sum_{i \in F_j} \gamma^{i-1} (f_i(X) - f_i(z_i))

        // TODO: We could cache the polynomials computed during phase1 and
        // multiply them by Z_T_minus_z_j_at_z[j] here (leveraging the fact
        // that r_i is always a constant in out setting). For now, just
        // recompute (including Z_T_minus_z_j_at_z in the start_factor)

        polynomial<Field> H_j = internal::compute_bdfg21_f_minus_r_polynomial(
            f_sets[j],
            evaluations[j],
            gamma_power * Z_T_minus_z_j_at_z[j],
            gamma);
        internal::polynomial_add_inplace(L, H_j);

        // Update gamma_power
        for (size_t i = 0; i < f_sets[j].size(); ++i) {
            gamma_power *= gamma;
        }
    }

    // Compute L(X) / (X - z)
    assert(Field::zero() == libfqfft::evaluate_polynomial(L.size(), L, z));
    std::vector<Field> L_over_X_minus_z;
    std::vector<Field> remainder;
    libfqfft::_polynomial_division(L_over_X_minus_z, remainder, L, {-z, 1});
    assert(libfqfft::_is_zero(remainder));

    libff::G1<ppT> W_prime = kzg10<ppT>::commit(srs, L_over_X_minus_z);
    return evaluation_witness(phase_1_out.public_witness_phase_1, W_prime);
}

template<typename ppT>
bool bdfg21<ppT>::verify_evaluations(
    const std::vector<libff::Fr<ppT>> &T,
    const typename bdfg21<ppT>::evaluations &evaluations,
    const srs &srs,
    const libff::Fr<ppT> &gamma,
    const libff::Fr<ppT> &z,
    const typename bdfg21<ppT>::evaluation_witness &witness,
    const std::vector<std::vector<commitment>> &cm_sets)
{
    // See Section 4.1 [BDFG21].
    //
    // The verifier accepts if:
    //
    //   e(F + z W', [1]_2) = e(W', [x]_2)
    //
    // where
    //
    //   F = \sum_{i=1}^k \gamma^{i-1} Z_{T\S_i}(z) cm_i
    //       - [ \sum_{i=1}^k \gamma^{i-1} Z_{T\S_i}(z) r_i(z) ]_1
    //       - Z_T(z) W
    //
    // as in create_evaluation_witness,
    //
    //   r_i(X) = f_i(z_j) = evaluation[j][k]
    //
    // for appropriate j and k.

    // Compute Z_T(z) and Z_{T\S_i}(z).

    const std::vector<Field> Z_T_minus_z_j_at_z =
        internal::compute_Z_T_minus_z_j_values(T, z);
    const Field Z_T_at_z = Z_T_minus_z_j_at_z[0] * (z - T[0]);
    assert(Z_T_at_z == Z_T_minus_z_j_at_z[1] * (z - T[1]));

    // For i = 1, ...,k we sum value
    //
    //   G_i = \gamma^{i-1} Z_{T\S_i}(z) cm_i     (in G1)
    //
    // and
    //
    //   H_i = \gamma^{i-1} Z_{T\S_i}(z) r_i(z)   (in Fr)
    //
    // to obtain G and H. This is done by iterating through the z_j values in T
    // (for j = 0, ..., T.size()-1), and through the polynomials
    // evaluations[j][k] (for k = 0, ..., evaluations[j].size()-1). The index i
    // is implicit.

    // TODO: For a large number of polynomials it may be faster to compute all
    // the scalar factors and compute A as an MSM (although this requires
    // allocation and copying).

    Field gamma_power = 1;

    libff::G1<ppT> G = libff::G1<ppT>::zero();
    Field H = Field::zero();

    for (size_t j = 0; j < T.size(); ++j) {
        const Field &Z_T_minus_S_at_z = Z_T_minus_z_j_at_z[j];
        const std::vector<Field> &evals = evaluations[j];
        const std::vector<commitment> &cms = cm_sets[j];
        assert(cms.size() == evals.size());

        for (size_t k = 0; k < evals.size(); ++k) {
            const Field factor = gamma_power * Z_T_minus_S_at_z;

            // G = G + ( \gamma^{i-1} Z_{T\S_i}(z) ) cm_i
            G = G + (factor * cms[k]);

            // H = H + \gamma^{i-1} Z_{T\S_i}(z) r_i(z)
            H += factor * evals[k];

            gamma_power *= gamma;
        }
    }

    // F = G + H * [1]_1 - Z_T_at_z * W
    const libff::G1<ppT> F =
        G - H * libff::G1<ppT>::one() - Z_T_at_z * witness.W;

    // The pairing check can be performed as:
    //
    //   e(A, B) * e(C, D) = 1
    //
    // where
    //
    //   A = F + z W'
    //   B = -[1]_2
    //   C = W'
    //   D = [x]_2

    const libff::G1_precomp<ppT> A =
        ppT::precompute_G1(F + z * witness.W_prime);
    const libff::G2_precomp<ppT> B = ppT::precompute_G2(-libff::G2<ppT>::one());
    const libff::G1_precomp<ppT> C = ppT::precompute_G1(witness.W_prime);
    const libff::G2_precomp<ppT> D = ppT::precompute_G2(srs.alpha_g2);

    const libff::Fqk<ppT> miller_result = ppT::double_miller_loop(A, B, C, D);
    const libff::GT<ppT> result = ppT::final_exponentiation(miller_result);
    return result == libff::GT<ppT>::one();
}

} // namespace libsnark

#endif // __LIBSNARK_POLYNOMIAL_COMMITMENTS_BDFG21_TCC__
