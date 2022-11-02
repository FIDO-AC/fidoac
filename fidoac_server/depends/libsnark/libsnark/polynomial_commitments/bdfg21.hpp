/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef __LIBSNARK_POLYNOMIAL_COMMITMENTS_BDFG21_HPP__
#define __LIBSNARK_POLYNOMIAL_COMMITMENTS_BDFG21_HPP__

#include "libsnark/polynomial_commitments/kzg10.hpp"

/// Reference:
/// - [BDFG21]:
///   Title: "Efficient polynomial commitment schemes for multiple points and
///           polynomials"
///   eprint: https://eprint.iacr.org/2020/081.pdf

namespace libsnark
{

/// Batched polynomial commitment scheme from [BDFG21], using concepts from
/// [KZG10] and [GWC19]. Supports evaluation of an arbitrary number of
/// polynomials at arbitrary points, with a fixed-size witness. See [BDFG21]
/// Section 4.
///
/// The srs and commitments are the same as KZG10.
template<typename ppT> class bdfg21
{
public:
    using Field = libff::Fr<ppT>;

    /// The srs generated by the setup step. This is denoted PK in [KZG10]
    /// section 3.2.
    using srs = typename kzg10<ppT>::srs;

    using commitment = typename kzg10<ppT>::commitment;

    /// In [BDFG21], the evaluation points and polynomials are specified as:
    ///
    ///   - a set $\{ f_i \}$ of $k$ polynomials
    ///
    ///   - a set $T$ of all evaluation points
    ///
    ///   - the set $\{ S_i \}$ of subsets $S_i$ of $T$ (for $T$ a subset of
    ///     the field of interest) where each polynomial $f_i$ will be
    ///     evaluated at points in $S_i$.
    ///
    /// To match the expected use-cases, we assume that each polynomial $f_i$
    /// is evaluated at exactly one point $z_i \in T$, and use a specification
    /// which is efficient for this case.  Namely:
    ///
    ///   - `std::vector<Field> T` is a set of evaluation points.
    ///
    ///   - `std::vector<std::vector<polynomial>> f_sets` is the set of sets
    ///     where `f_sets[i]` contains the polynomials to be evaluated at
    ///     `T[i]`.
    ///
    /// Repeated polynomials (that is, a polynomial `f` which is to be
    /// evaluated at multiple points) must appear in multiple entries of
    /// `f_sets`, and is therefore inefficient in this formulation. This case
    /// is expected to be rare.
    ///
    /// Other formulations can be supported as they are required.

    /// The `i`-th entry is evaluations of polynomials in `f_sets[i]` at `z_i`.
    using evaluations = std::vector<std::vector<Field>>;

    using evaluation_witness_phase_1 = libff::G1<ppT>;

    /// Contains the result of phase1 of the witness generation.
    /// `public_witness_phase_1` is to be shared with the verifier, whereas
    /// everything else is private context for use by the prover.
    class phase_1_output
    {
    public:
        libff::G1<ppT> public_witness_phase_1;
        polynomial<Field> private_f_over_Z_T_polynomial;

        phase_1_output(
            const libff::G1<ppT> &witness_phase_1,
            polynomial<Field> &&f_over_Z_T_polynomial);
    };

    /// Proof that a given set of evaluations is correct. We adopt the W and W'
    /// (called `W_prime` here) notation of [BDFG21].
    class evaluation_witness
    {
    public:
        libff::G1<ppT> W;
        libff::G1<ppT> W_prime;

        evaluation_witness(
            const libff::G1<ppT> &W, const libff::G1<ppT> &W_prime);
    };

    static evaluations evaluate_polynomials(
        const std::vector<std::vector<polynomial<Field>>> &f_sets,
        const std::vector<Field> z_s);

    /// Compute the first response from the prover, given the first random
    /// value $\gamma$ sampled by the verifier.
    static phase_1_output create_evaluation_witness_phase_1(
        const std::vector<std::vector<polynomial<Field>>> &f_sets,
        const std::vector<Field> &T,
        const evaluations &evaluations,
        const srs &srs,
        const Field &gamma);

    /// Compute the final witness, given the second random value $z$ from the
    /// verifier.
    static evaluation_witness create_evaluation_witness(
        const std::vector<std::vector<polynomial<Field>>> &f_sets,
        const std::vector<Field> &T,
        const evaluations &evaluations,
        const srs &srs,
        const Field &gamma,
        const phase_1_output &witness_phase_1,
        const Field &z);

    /// Verify an evaluation witness using polynomial commitments `cm_sets`.
    /// Commitments are arranged similarly to polynomials in f_sets, so that
    /// `cm_sets[i][j]` is a commitment to `f_sets[i][j]` (the j-th polynomial
    /// of the i-th set).
    static bool verify_evaluations(
        const std::vector<Field> &T,
        const evaluations &evaluations,
        const srs &srs,
        const Field &gamma,
        const Field &z,
        const evaluation_witness &witness,
        const std::vector<std::vector<commitment>> &cm_sets);
};

} // namespace libsnark

#include "libsnark/polynomial_commitments/bdfg21.tcc"

#endif // __LIBSNARK_POLYNOMIAL_COMMITMENTS_BDFG21_HPP__
