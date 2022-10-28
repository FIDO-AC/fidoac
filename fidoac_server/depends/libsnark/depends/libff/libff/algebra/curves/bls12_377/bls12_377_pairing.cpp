#include <cassert>
#include <libff/algebra/curves/bls12_377/bls12_377_g1.hpp>
#include <libff/algebra/curves/bls12_377/bls12_377_g2.hpp>
#include <libff/algebra/curves/bls12_377/bls12_377_init.hpp>
#include <libff/algebra/curves/bls12_377/bls12_377_pairing.hpp>
#include <libff/common/profiling.hpp>

namespace libff
{

bool bls12_377_ate_G1_precomp::operator==(
    const bls12_377_ate_G1_precomp &other) const
{
    return (this->PX == other.PX && this->PY == other.PY);
}

std::ostream &operator<<(
    std::ostream &out, const bls12_377_ate_G1_precomp &prec_P)
{
    out << prec_P.PX << OUTPUT_SEPARATOR << prec_P.PY;

    return out;
}

std::istream &operator>>(std::istream &in, bls12_377_ate_G1_precomp &prec_P)
{
    in >> prec_P.PX;
    consume_OUTPUT_SEPARATOR(in);
    in >> prec_P.PY;

    return in;
}

bool bls12_377_ate_ell_coeffs::operator==(
    const bls12_377_ate_ell_coeffs &other) const
{
    return (
        this->ell_0 == other.ell_0 && this->ell_VW == other.ell_VW &&
        this->ell_VV == other.ell_VV);
}

std::ostream &operator<<(std::ostream &out, const bls12_377_ate_ell_coeffs &c)
{
    out << c.ell_0 << OUTPUT_SEPARATOR << c.ell_VW << OUTPUT_SEPARATOR
        << c.ell_VV;
    return out;
}

std::istream &operator>>(std::istream &in, bls12_377_ate_ell_coeffs &c)
{
    in >> c.ell_0;
    consume_OUTPUT_SEPARATOR(in);
    in >> c.ell_VW;
    consume_OUTPUT_SEPARATOR(in);
    in >> c.ell_VV;

    return in;
}

bool bls12_377_ate_G2_precomp::operator==(
    const bls12_377_ate_G2_precomp &other) const
{
    return (
        this->QX == other.QX && this->QY == other.QY &&
        this->coeffs == other.coeffs);
}

std::ostream &operator<<(
    std::ostream &out, const bls12_377_ate_G2_precomp &prec_Q)
{
    out << prec_Q.QX << OUTPUT_SEPARATOR << prec_Q.QY << "\n";
    out << prec_Q.coeffs.size() << "\n";
    for (const bls12_377_ate_ell_coeffs &c : prec_Q.coeffs) {
        out << c << OUTPUT_NEWLINE;
    }
    return out;
}

std::istream &operator>>(std::istream &in, bls12_377_ate_G2_precomp &prec_Q)
{
    in >> prec_Q.QX;
    consume_OUTPUT_SEPARATOR(in);
    in >> prec_Q.QY;
    consume_newline(in);

    prec_Q.coeffs.clear();
    size_t s;
    in >> s;

    consume_newline(in);

    prec_Q.coeffs.reserve(s);

    for (size_t i = 0; i < s; ++i) {
        bls12_377_ate_ell_coeffs c;
        in >> c;
        consume_OUTPUT_NEWLINE(in);
        prec_Q.coeffs.emplace_back(c);
    }

    return in;
}

// Below is the code related to the final exponentiation
// References:
// - https://eprint.iacr.org/2012/232.pdf
// - https://eprint.iacr.org/2016/130.pdf

bls12_377_Fq12 bls12_377_final_exponentiation_first_chunk(
    const bls12_377_Fq12 &elt)
{
    // The content of this file follows: https://eprint.iacr.org/2016/130.pdf
    //
    // Note: in the alt_bn_128 implementation the libff authors used a trick by
    // Beuchat et al. to compute the first chunk of the exponentiation which
    // seems faster. Look into that and use it if it applies here too.
    //
    // TODO: Look into this.
    enter_block("Call to bls12_377_final_exponentiation_first_chunk");

    // elt^(q^6)
    const bls12_377_Fq12 A = elt.Frobenius_map(6);
    // elt^(-1)
    const bls12_377_Fq12 B = elt.inverse();
    // elt^(q^6 - 1)
    const bls12_377_Fq12 C = A * B;
    // (elt^(q^6 - 1))^(q^2) = elt^((q^6 - 1) * (q^2))
    const bls12_377_Fq12 D = C.Frobenius_map(2);
    // elt^((q^6 - 1) * (q^2) + (q^6 - 1)) = elt^((q^6 - 1) * (q^2 + 1))
    const bls12_377_Fq12 result = D * C;

    leave_block("Call to bls12_377_final_exponentiation_first_chunk");

    return result;
}

bls12_377_Fq12 bls12_377_exp_by_z(const bls12_377_Fq12 &elt)
{
    enter_block("Call to bls12_377_exp_by_z");

    bls12_377_Fq12 result = elt.cyclotomic_exp(bls12_377_final_exponent_z);
    if (bls12_377_final_exponent_is_z_neg) {
        result = result.unitary_inverse();
    }

    leave_block("Call to bls12_377_exp_by_z");

    return result;
}

bls12_377_Fq12 bls12_377_final_exponentiation_last_chunk(
    const bls12_377_Fq12 &elt)
{
    enter_block("Call to bls12_377_final_exponentiation_last_chunk");

    // In the following, we follow the Algorithm 1 described in Table 1 of:
    // https://eprint.iacr.org/2016/130.pdf in order to compute the
    // hard part of the final exponentiation
    //
    // Note: As shown Table 3: https://eprint.iacr.org/2016/130.pdf this
    // algorithm isn't optimal since Algorithm 2 allows to have less temp.
    // variables and has a better complexity.
    //
    // In the following we denote by [x] = elt^(x):
    // A = [-2]
    const bls12_377_Fq12 A = elt.cyclotomic_squared().unitary_inverse();
    // B = [z]
    const bls12_377_Fq12 B = bls12_377_exp_by_z(elt);
    // C = [2z]
    const bls12_377_Fq12 C = B.cyclotomic_squared();
    // D = [z-2]
    const bls12_377_Fq12 D = A * B;
    // E = [z^2-2z]
    const bls12_377_Fq12 E = bls12_377_exp_by_z(D);
    // F = [z^3-2z^2]
    const bls12_377_Fq12 F = bls12_377_exp_by_z(E);
    // G = [z^4-2z^3]
    const bls12_377_Fq12 G = bls12_377_exp_by_z(F);
    // H = [z^4-2z^3+2z]
    const bls12_377_Fq12 H = G * C;
    // I = [z^5-2z^4+2z^2]
    const bls12_377_Fq12 I = bls12_377_exp_by_z(H);
    // J = [-z+2]
    const bls12_377_Fq12 J = D.unitary_inverse();
    // K = [z^5-2z^4+2z^2-z+2]
    const bls12_377_Fq12 K = I * J;
    // L = [z^5-2z^4+2z^2-z+3] = [\lambda_0]
    const bls12_377_Fq12 L = K * elt;
    // M = [-1]
    const bls12_377_Fq12 M = elt.unitary_inverse();
    // N = [z^2-2z+1] = [\lambda_3]
    const bls12_377_Fq12 N = E * elt;
    // O = [(z^2-2z+1) * (q^3)]
    const bls12_377_Fq12 O = N.Frobenius_map(3);
    // P = [z^4-2z^3+2z-1] = [\lambda_1]
    const bls12_377_Fq12 P = H * M;
    // Q = [(z^4-2z^3+2z-1) * q]
    const bls12_377_Fq12 Q = P.Frobenius_map(1);
    // R = [z^3-2z^2+z] = [\lambda_2]
    const bls12_377_Fq12 R = F * B;
    // S = [(z^3-2z^2+z) * (q^2)]
    const bls12_377_Fq12 S = R.Frobenius_map(2);
    // T = [(z^2-2z+1) * (q^3) + (z^3-2z^2+z) * (q^2)]
    const bls12_377_Fq12 T = O * S;
    // U = [(z^2-2z+1) * (q^3) + (z^3-2z^2+z) * (q^2) + (z^4-2z^3+2z-1) * q]
    const bls12_377_Fq12 U = T * Q;
    // result = [(z^2-2z+1) * (q^3) + (z^3-2z^2+z) * (q^2) + (z^4-2z^3+2z-1) * q
    // + z^5-2z^4+2z^2-z+3]
    //        = [(p^4 - p^2 + 1)/r].
    const bls12_377_Fq12 result = U * L;

    leave_block("Call to bls12_377_final_exponentiation_last_chunk");

    return result;
}

bls12_377_GT bls12_377_final_exponentiation(const bls12_377_Fq12 &elt)
{
    // We know that:
    // (p^12 - 1) / r = (p^6 - 1) (p^2 + 1) ((p^4 - p^2 + 1) / r)
    //                  |_________________|  |__________________|
    //                       easy part            hard part
    // where:
    // sage: cyclotomic_polynomial(12) # = x^4 - x^2 + 1
    enter_block("Call to bls12_377_final_exponentiation");

    // Compute the easy part
    bls12_377_Fq12 first = bls12_377_final_exponentiation_first_chunk(elt);
    // Finish the final exponentiation by computing the hard part
    bls12_377_GT result = bls12_377_final_exponentiation_last_chunk(first);

    leave_block("Call to bls12_377_final_exponentiation");

    return result;
}

// Below is the code related to the computation of the Ate pairing

void bls12_377_doubling_step_for_miller_loop(
    const bls12_377_Fq two_inv,
    bls12_377_G2 &current,
    bls12_377_ate_ell_coeffs &c)
{
    // Below we assume that `current` = (X, Y, Z) \in E'(Fp2) is a point
    // in homogeneous projective coordinates.
    const bls12_377_Fq2 X = current.X, Y = current.Y, Z = current.Z;

    // Compute the line function

    // A = (X * Y)/ 2
    const bls12_377_Fq2 A = two_inv * (X * Y);
    // B = Y^2
    const bls12_377_Fq2 B = Y.squared();
    // C = Z^2
    const bls12_377_Fq2 C = Z.squared();
    // D = 3 * C
    const bls12_377_Fq2 D = C + C + C;
    // E = bls12_377_twist_coeff_b * D
    const bls12_377_Fq2 E = bls12_377_twist_coeff_b * D;
    // F = 3 * E
    const bls12_377_Fq2 F = E + E + E;
    // G = (B + F)/2
    const bls12_377_Fq2 G = two_inv * (B + F);
    // H = (Y + Z)^2 - (B + C) = 2YZ
    const bls12_377_Fq2 H = (Y + Z).squared() - (B + C);
    // I = E - B
    const bls12_377_Fq2 I = E - B;
    // J = X^2
    const bls12_377_Fq2 J = X.squared();
    // E_squared = E^2
    const bls12_377_Fq2 E_squared = E.squared();

    // X = A * (B-F)
    //   = ((X * Y)/ 2) * (Y^2 - 3 * (bls12_377_twist_coeff_b * 3 * Z^2)
    //   = ((X * Y)/ 2) * (Y^2 - 9 * (bls12_377_twist_coeff_b * Z^2)
    current.X = A * (B - F);
    // Y = G^2 - 3*E^2
    //   = (Y^2 + 9 * bls12_377_twist_coeff_b * Z^2)/2 -
    //   27*(bls12_377_twist_coeff_b^2 * Z^4)
    current.Y = G.squared() - (E_squared + E_squared + E_squared);
    // Z = B * H
    //   = Y^2 * 2YZ
    //   = 2Y^3 * Z
    current.Z = B * H;

    // Note: We use a D-type twist
    //
    // See: Equation (2) https://eprint.iacr.org/2010/526.pdf
    //
    // The tangent line evaluated at the twisting point P = (x_p, y_p) is
    // computed as:
    //   (-2YZ*y_p)vw +                            <-- -H
    //   (3*X^2 * x_p) * v^2 +                     <-- 3*J
    //   bls12_377_twist * bls12_377_twist *       <-- bls12_377_twist * I
    //     (3*bls12_377_twist_coeff_b*Z^2 - Y^2)
    c.ell_VW = -H;
    c.ell_VV = J + J + J;
    c.ell_0 = bls12_377_twist * I;
}

void bls12_377_mixed_addition_step_for_miller_loop(
    const bls12_377_G2 &base,
    bls12_377_G2 &current,
    bls12_377_ate_ell_coeffs &c)
{
    const bls12_377_Fq2 &X1 = current.X, &Y1 = current.Y, &Z1 = current.Z;
    const bls12_377_Fq2 &X2 = base.X, &Y2 = base.Y;

    const bls12_377_Fq2 A = Y2 * Z1;
    const bls12_377_Fq2 B = X2 * Z1;
    const bls12_377_Fq2 theta = Y1 - A;
    const bls12_377_Fq2 lambda = X1 - B;
    const bls12_377_Fq2 C = theta.squared();
    const bls12_377_Fq2 D = lambda.squared();
    const bls12_377_Fq2 E = lambda * D;
    const bls12_377_Fq2 F = Z1 * C;
    const bls12_377_Fq2 G = X1 * D;
    const bls12_377_Fq2 H = E + F - (G + G);
    const bls12_377_Fq2 I = Y1 * E;
    const bls12_377_Fq2 J = theta * X2 - lambda * Y2;

    current.X = lambda * H;
    current.Y = theta * (G - H) - I;
    current.Z = Z1 * E;

    c.ell_0 = bls12_377_twist * J;
    // VV gets multiplied to xP during line evaluation at P
    c.ell_VV = -theta;
    // VW gets multiplied to yP during line evaluation at P
    c.ell_VW = lambda;
}

bls12_377_ate_G1_precomp bls12_377_ate_precompute_G1(const bls12_377_G1 &P)
{
    enter_block("Call to bls12_377_ate_precompute_G1");

    bls12_377_G1 Pcopy = P;
    Pcopy.to_affine_coordinates();

    bls12_377_ate_G1_precomp result;
    result.PX = Pcopy.X;
    result.PY = Pcopy.Y;

    leave_block("Call to bls12_377_ate_precompute_G1");
    return result;
}

// Note the code below can be refactored to follow the approach
// detailled in Section 5: https://eprint.iacr.org/2019/077.pdf
// and simplify the Miller loop by calculating and storing the line
// functions in an array.
//
// Below, the computation of the pairing is done as follows:
// 1. Precompute the set of points R:
// s.t. init: R <- Q, double: R <- 2R, add: R <- R + Q
// 2. Use the set of precomputed Rs in the Miller Loop
// 3. Carry out the final exponentiation on the result of the Miller loop
// The last step is carried out in 2 sub-steps (note: is_even(k) = True since k
// = 12):
//      - Carry out the easy part of the exponentiation
//      - Carry out the hard part of the exponentiation
//
bls12_377_ate_G2_precomp bls12_377_ate_precompute_G2(const bls12_377_G2 &Q)
{
    enter_block("Call to bls12_377_ate_precompute_G2");

    bls12_377_G2 Qcopy(Q);
    Qcopy.to_affine_coordinates();

    // could add to global params if needed
    bls12_377_Fq two_inv = (bls12_377_Fq("2").inverse());

    bls12_377_ate_G2_precomp result;
    result.QX = Qcopy.X;
    result.QY = Qcopy.Y;

    bls12_377_G2 R;
    R.X = Qcopy.X;
    R.Y = Qcopy.Y;
    R.Z = bls12_377_Fq2::one();

    const bigint<bls12_377_Fq::num_limbs> &loop_count =
        bls12_377_ate_loop_count;
    bool found_one = false;
    bls12_377_ate_ell_coeffs c;

    for (long i = loop_count.max_bits(); i >= 0; --i) {
        const bool bit = loop_count.test_bit(i);
        if (!found_one) {
            // This skips the MSB itself
            found_one |= bit;
            continue;
        }

        bls12_377_doubling_step_for_miller_loop(two_inv, R, c);
        result.coeffs.push_back(c);

        if (bit) {
            bls12_377_mixed_addition_step_for_miller_loop(Qcopy, R, c);
            result.coeffs.push_back(c);
        }
    }

    leave_block("Call to bls12_377_ate_precompute_G2");
    return result;
}

bls12_377_Fq12 bls12_377_ate_miller_loop(
    const bls12_377_ate_G1_precomp &prec_P,
    const bls12_377_ate_G2_precomp &prec_Q)
{
    enter_block("Call to bls12_377_ate_miller_loop");

    bls12_377_Fq12 f = bls12_377_Fq12::one();

    bool found_one = false;
    size_t idx = 0;

    const bigint<bls12_377_Fq::num_limbs> &loop_count =
        bls12_377_ate_loop_count;
    bls12_377_ate_ell_coeffs c;

    // Added for DEBUG purpose
    // TODO: Remove the 2 variables below
    int nb_double = 0;
    int nb_add = 0;

    // The loop length of the Miller algorithm is floor(log_2(u)), where
    // u is the "curve parameter" used to sample the curve from the
    // BLS12 family
    for (long i = loop_count.max_bits(); i >= 0; --i) {
        const bool bit = loop_count.test_bit(i);
        if (!found_one) {
            // This skips the MSB itself
            found_one |= bit;
            continue;
        }

        // The code below gets executed for all bits (EXCEPT the MSB itself)
        // of the binary representation of bls12_377_ate_loop_count
        // (skipping leading zeros) in MSB to LSB order
        c = prec_Q.coeffs[idx++];
        f = f.squared();
        // Sparse multiplication in Fq12
        f = f.mul_by_024(c.ell_0, prec_P.PY * c.ell_VW, prec_P.PX * c.ell_VV);
        nb_double++;

        if (bit) {
            c = prec_Q.coeffs[idx++];
            f = f.mul_by_024(
                c.ell_0, prec_P.PY * c.ell_VW, prec_P.PX * c.ell_VV);
            nb_add++;
        }
    }

    // Note: bls12_377_ate_is_loop_count_neg = false BUT this is not the case
    // for BLS12-381!

    std::cout << "[DEBUG] NB_DOUBLE (Should be 63): " << nb_double
              << " NB_ADD (Should be 7): " << nb_add << std::endl;
    leave_block("Call to bls12_377_ate_miller_loop");
    return f;
}

bls12_377_Fq12 bls12_377_ate_double_miller_loop(
    const bls12_377_ate_G1_precomp &prec_P1,
    const bls12_377_ate_G2_precomp &prec_Q1,
    const bls12_377_ate_G1_precomp &prec_P2,
    const bls12_377_ate_G2_precomp &prec_Q2)
{
    enter_block("Call to bls12_377_ate_double_miller_loop");

    bls12_377_Fq12 f = bls12_377_Fq12::one();

    bool found_one = false;
    size_t idx = 0;

    const bigint<bls12_377_Fq::num_limbs> &loop_count =
        bls12_377_ate_loop_count;
    for (long i = loop_count.max_bits(); i >= 0; --i) {
        const bool bit = loop_count.test_bit(i);
        if (!found_one) {
            // This skips the MSB itself
            found_one |= bit;
            continue;
        }

        // The code below gets executed for all bits (EXCEPT the MSB itself)
        // of the binary representation of bls12_377_ate_loop_count
        // (skipping leading zeros) in MSB to LSB order
        bls12_377_ate_ell_coeffs c1 = prec_Q1.coeffs[idx];
        bls12_377_ate_ell_coeffs c2 = prec_Q2.coeffs[idx];
        ++idx;

        f = f.squared();

        f = f.mul_by_024(
            c1.ell_0, prec_P1.PY * c1.ell_VW, prec_P1.PX * c1.ell_VV);
        f = f.mul_by_024(
            c2.ell_0, prec_P2.PY * c2.ell_VW, prec_P2.PX * c2.ell_VV);

        if (bit) {
            bls12_377_ate_ell_coeffs c1 = prec_Q1.coeffs[idx];
            bls12_377_ate_ell_coeffs c2 = prec_Q2.coeffs[idx];
            ++idx;

            f = f.mul_by_024(
                c1.ell_0, prec_P1.PY * c1.ell_VW, prec_P1.PX * c1.ell_VV);
            f = f.mul_by_024(
                c2.ell_0, prec_P2.PY * c2.ell_VW, prec_P2.PX * c2.ell_VV);
        }
    }

    leave_block("Call to bls12_377_ate_double_miller_loop");

    return f;
}

bls12_377_Fq12 bls12_377_ate_pairing(
    const bls12_377_G1 &P, const bls12_377_G2 &Q)
{
    enter_block("Call to bls12_377_ate_pairing");
    bls12_377_ate_G1_precomp prec_P = bls12_377_ate_precompute_G1(P);
    bls12_377_ate_G2_precomp prec_Q = bls12_377_ate_precompute_G2(Q);
    bls12_377_Fq12 result = bls12_377_ate_miller_loop(prec_P, prec_Q);
    leave_block("Call to bls12_377_ate_pairing");
    return result;
}

bls12_377_GT bls12_377_ate_reduced_pairing(
    const bls12_377_G1 &P, const bls12_377_G2 &Q)
{
    enter_block("Call to bls12_377_ate_reduced_pairing");
    const bls12_377_Fq12 f = bls12_377_ate_pairing(P, Q);
    const bls12_377_GT result = bls12_377_final_exponentiation(f);
    leave_block("Call to bls12_377_ate_reduced_pairing");
    return result;
}

/* choice of pairing */

bls12_377_G1_precomp bls12_377_precompute_G1(const bls12_377_G1 &P)
{
    return bls12_377_ate_precompute_G1(P);
}

bls12_377_G2_precomp bls12_377_precompute_G2(const bls12_377_G2 &Q)
{
    return bls12_377_ate_precompute_G2(Q);
}

bls12_377_Fq12 bls12_377_miller_loop(
    const bls12_377_G1_precomp &prec_P, const bls12_377_G2_precomp &prec_Q)
{
    return bls12_377_ate_miller_loop(prec_P, prec_Q);
}

bls12_377_Fq12 bls12_377_double_miller_loop(
    const bls12_377_G1_precomp &prec_P1,
    const bls12_377_G2_precomp &prec_Q1,
    const bls12_377_G1_precomp &prec_P2,
    const bls12_377_G2_precomp &prec_Q2)
{
    return bls12_377_ate_double_miller_loop(prec_P1, prec_Q1, prec_P2, prec_Q2);
}

bls12_377_Fq12 bls12_377_pairing(const bls12_377_G1 &P, const bls12_377_G2 &Q)
{
    return bls12_377_ate_pairing(P, Q);
}

bls12_377_GT bls12_377_reduced_pairing(
    const bls12_377_G1 &P, const bls12_377_G2 &Q)
{
    return bls12_377_ate_reduced_pairing(P, Q);
}
} // namespace libff
