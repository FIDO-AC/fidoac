#include <cassert>
#include <libff/algebra/curves/bw6_761/bw6_761_g1.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_g2.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_init.hpp>
#include <libff/algebra/curves/bw6_761/bw6_761_pairing.hpp>
#include <libff/common/profiling.hpp>

namespace libff
{

bool bw6_761_ate_G1_precomp::operator==(
    const bw6_761_ate_G1_precomp &other) const
{
    return (this->PX == other.PX && this->PY == other.PY);
}

std::ostream &operator<<(
    std::ostream &out, const bw6_761_ate_G1_precomp &prec_P)
{
    out << prec_P.PX << OUTPUT_SEPARATOR << prec_P.PY;

    return out;
}

std::istream &operator>>(std::istream &in, bw6_761_ate_G1_precomp &prec_P)
{
    in >> prec_P.PX;
    consume_OUTPUT_SEPARATOR(in);
    in >> prec_P.PY;

    return in;
}

bool bw6_761_ate_ell_coeffs::operator==(
    const bw6_761_ate_ell_coeffs &other) const
{
    return (
        this->ell_0 == other.ell_0 && this->ell_VW == other.ell_VW &&
        this->ell_VV == other.ell_VV);
}

std::ostream &operator<<(std::ostream &out, const bw6_761_ate_ell_coeffs &c)
{
    out << c.ell_0 << OUTPUT_SEPARATOR << c.ell_VW << OUTPUT_SEPARATOR
        << c.ell_VV;
    return out;
}

std::istream &operator>>(std::istream &in, bw6_761_ate_ell_coeffs &c)
{
    in >> c.ell_0;
    consume_OUTPUT_SEPARATOR(in);
    in >> c.ell_VW;
    consume_OUTPUT_SEPARATOR(in);
    in >> c.ell_VV;

    return in;
}

bool bw6_761_ate_G2_precomp_iteration::operator==(
    const bw6_761_ate_G2_precomp_iteration &other) const
{
    return (
        this->QX == other.QX && this->QY == other.QY &&
        this->coeffs == other.coeffs);
}

std::ostream &operator<<(
    std::ostream &out, const bw6_761_ate_G2_precomp_iteration &prec_Q)
{
    out << prec_Q.QX << OUTPUT_SEPARATOR << prec_Q.QY << "\n";
    out << prec_Q.coeffs.size() << "\n";
    for (const bw6_761_ate_ell_coeffs &c : prec_Q.coeffs) {
        out << c << OUTPUT_NEWLINE;
    }
    return out;
}

std::istream &operator>>(
    std::istream &in, bw6_761_ate_G2_precomp_iteration &prec_Q)
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
        bw6_761_ate_ell_coeffs c;
        in >> c;
        consume_OUTPUT_NEWLINE(in);
        prec_Q.coeffs.emplace_back(c);
    }

    return in;
}

bool bw6_761_ate_G2_precomp::operator==(
    const bw6_761_ate_G2_precomp &other) const
{
    return (
        this->precomp_1 == other.precomp_1 &&
        this->precomp_2 == other.precomp_2);
}

std::ostream &operator<<(
    std::ostream &out, const bw6_761_ate_G2_precomp &prec_Q)
{
    out << prec_Q.precomp_1 << OUTPUT_SEPARATOR << prec_Q.precomp_2 << "\n";
    return out;
}

std::istream &operator>>(std::istream &in, bw6_761_ate_G2_precomp &prec_Q)
{
    in >> prec_Q.precomp_1;
    consume_OUTPUT_SEPARATOR(in);
    in >> prec_Q.precomp_2;
    consume_newline(in);
    return in;
}

// Below is the code related to the final exponentiation

bw6_761_Fq6 bw6_761_final_exponentiation_first_chunk(const bw6_761_Fq6 &elt)
{
    // Compute elt^{(q^3-1)*(q+1)}
    enter_block("Call to bw6_761_final_exponentiation_first_chunk");

    // A = elt^(q^3)
    const bw6_761_Fq6 A = elt.Frobenius_map(3);
    // B = elt^(q^3-1)
    const bw6_761_Fq6 elt_inv = elt.inverse();
    const bw6_761_Fq6 B = A * elt_inv;
    // D = elt^((q^3-1) * q)
    const bw6_761_Fq6 D = B.Frobenius_map(1);
    // result = elt^{(q^3-1)*(q+1)}
    const bw6_761_Fq6 result = D * B;

    leave_block("Call to bw6_761_final_exponentiation_first_chunk");
    return result;
}

bw6_761_Fq6 bw6_761_exp_by_z(const bw6_761_Fq6 &elt)
{
    enter_block("Call to bw6_761_exp_by_z");

    bw6_761_Fq6 result = elt.cyclotomic_exp(bw6_761_final_exponent_z);
    if (bw6_761_final_exponent_is_z_neg) {
        result = result.unitary_inverse();
    }

    leave_block("Call to bw6_761_exp_by_z");

    return result;
}

// See Algorithm 6, Appendix B: https://eprint.iacr.org/2020/351.pdf
// This function computes:
// elt^{σ′(u)}, where σ′(u) = R_0(u) + q*R_1(u)
// and where:
//  - R0(x) := (-103*x^7 + 70*x^6 + 269*x^5 - 197*x^4 - 314*x^3 - 73*x^2 - 263*x
//  - 220)
//  - R1(x) := (103*x^9 - 276*x^8 + 77*x^7 + 492*x^6 - 445*x^5 - 65*x^4 +
//  452*x^3 - 181*x^2 + 34*x + 229)
bw6_761_Fq6 bw6_761_final_exponentiation_last_chunk(const bw6_761_Fq6 &elt)
{
    enter_block("Call to bw6_761_final_exponentiation_last_chunk");

    // Step 1
    const bw6_761_Fq6 f0 = elt;
    const bw6_761_Fq6 f0p = f0.Frobenius_map(1);

    // Step 2-3: For loop
    const bw6_761_Fq6 f1 = bw6_761_exp_by_z(f0);
    const bw6_761_Fq6 f1p = f1.Frobenius_map(1);
    const bw6_761_Fq6 f2 = bw6_761_exp_by_z(f1);
    const bw6_761_Fq6 f2p = f2.Frobenius_map(1);
    const bw6_761_Fq6 f3 = bw6_761_exp_by_z(f2);
    const bw6_761_Fq6 f3p = f3.Frobenius_map(1);
    const bw6_761_Fq6 f4 = bw6_761_exp_by_z(f3);
    const bw6_761_Fq6 f4p = f4.Frobenius_map(1);
    const bw6_761_Fq6 f5 = bw6_761_exp_by_z(f4);
    const bw6_761_Fq6 f5p = f5.Frobenius_map(1);
    const bw6_761_Fq6 f6 = bw6_761_exp_by_z(f5);
    const bw6_761_Fq6 f6p = f6.Frobenius_map(1);
    const bw6_761_Fq6 f7 = bw6_761_exp_by_z(f6);
    const bw6_761_Fq6 f7p = f7.Frobenius_map(1);

    // Step 4
    const bw6_761_Fq6 f8p = bw6_761_exp_by_z(f7p);
    const bw6_761_Fq6 f9p = bw6_761_exp_by_z(f8p);

    // Step 5
    const bw6_761_Fq6 result1 = f3p * f6p * f5p.Frobenius_map(3);

    // Step 6
    const bw6_761_Fq6 result2 = result1.squared();
    const bw6_761_Fq6 f4_2p = f4 * f2p;
    const bw6_761_Fq6 result3 =
        result2 * f5 * f0p * (f0 * f1 * f3 * f4_2p * f8p).Frobenius_map(3);

    // Step 7
    const bw6_761_Fq6 result4 = result3.squared();
    const bw6_761_Fq6 result5 = result4 * f9p * f7.Frobenius_map(3);

    // Step 8
    const bw6_761_Fq6 result6 = result5.squared();
    const bw6_761_Fq6 f2_4p = f2 * f4p;
    const bw6_761_Fq6 f4_2p_5p = f4_2p * f5p;
    const bw6_761_Fq6 result7 =
        result6 * f4_2p_5p * f6 * f7p * (f2_4p * f3 * f3p).Frobenius_map(3);

    // Step 9
    const bw6_761_Fq6 result8 = result7.squared();
    const bw6_761_Fq6 result9 =
        result8 * f0 * f7 * f1p * (f0p * f9p).Frobenius_map(3);

    // Step 10
    const bw6_761_Fq6 result10 = result9.squared();
    const bw6_761_Fq6 f6p_8p = f6p * f8p;
    const bw6_761_Fq6 f5_7p = f5 * f7p;
    const bw6_761_Fq6 result11 =
        result10 * f5_7p * f2p * (f6p_8p).Frobenius_map(3);

    // Step 11
    const bw6_761_Fq6 result12 = result11.squared();
    const bw6_761_Fq6 f3_6 = f3 * f6;
    const bw6_761_Fq6 f1_7 = f1 * f7;
    const bw6_761_Fq6 result13 =
        result12 * f3_6 * f9p * (f1_7 * f2).Frobenius_map(3);

    // Step 12
    const bw6_761_Fq6 result14 = result13.squared();
    const bw6_761_Fq6 result15 = result14 * f0 * f0p * f3p * f5p *
                                 (f4_2p * f5_7p * f6p_8p).Frobenius_map(3);

    // Step 13
    const bw6_761_Fq6 result16 = result15.squared();
    const bw6_761_Fq6 result17 = result16 * f1p * (f3_6).Frobenius_map(3);

    // Step 14
    const bw6_761_Fq6 result18 = result17.squared();
    const bw6_761_Fq6 result19 = result18 * f1_7 * f5_7p * f0p *
                                 (f2_4p * f4_2p_5p * f9p).Frobenius_map(3);

    leave_block("Call to bw6_761_final_exponentiation_last_chunk");

    return result19;
}

bw6_761_GT bw6_761_final_exponentiation(const bw6_761_Fq6 &elt)
{
    enter_block("Call to bw6_761_final_exponentiation");

    bw6_761_Fq6 elt_to_first_chunk =
        bw6_761_final_exponentiation_first_chunk(elt);
    bw6_761_GT result =
        bw6_761_final_exponentiation_last_chunk(elt_to_first_chunk);

    leave_block("Call to bw6_761_final_exponentiation");

    return result;
}

// Below is the code related to the computation of the Ate pairing

void doubling_step_for_miller_loop(
    bw6_761_G2 &current, bw6_761_ate_ell_coeffs &c)
{
    const bw6_761_Fq X = current.X, Y = current.Y, Z = current.Z;

    // A = X1 * Y1
    const bw6_761_Fq A = X * Y;
    // B = Y1^2
    const bw6_761_Fq B = Y.squared();
    // B4 = 4 * Y1^2
    const bw6_761_Fq B4 = B + B + B + B;
    // C = Z1^2
    const bw6_761_Fq C = Z.squared();
    // D = 3 * C
    const bw6_761_Fq D = C + C + C;
    // E = bw6_761_twist_coeff_b * D
    const bw6_761_Fq E = bw6_761_twist_coeff_b * D;
    // F = 3 * E
    const bw6_761_Fq F = E + E + E;
    // G = B+F
    const bw6_761_Fq G = B + F;
    // H = (Y1+Z1)^2-(B+C)
    const bw6_761_Fq H = (Y + Z).squared() - (B + C);
    // I = E-B
    const bw6_761_Fq I = E - B;
    // J = X1^2
    const bw6_761_Fq J = X.squared();
    // E2_squared = (2E)^2
    const bw6_761_Fq E2_squared = (E + E).squared();

    // X3 = 2A * (B-F)
    current.X = (A + A) * (B - F);
    // Y3 = G^2 - 3*E2^2
    current.Y = G.squared() - (E2_squared + E2_squared + E2_squared);
    // Z3 = 4 * B * H
    current.Z = B4 * H;

    // ell_0 = I
    c.ell_0 = I;
    // ell_VW = -xi * H (later: * yP)
    c.ell_VW = -bw6_761_twist * H;
    // ell_VV = 3*J (later: * xP)
    c.ell_VV = J + J + J;
}

void mixed_addition_step_for_miller_loop(
    const bw6_761_G2 base, bw6_761_G2 &current, bw6_761_ate_ell_coeffs &c)
{
    const bw6_761_Fq X1 = current.X, Y1 = current.Y, Z1 = current.Z;
    const bw6_761_Fq &X2 = base.X, &Y2 = base.Y;

    // D = X1 - X2*Z1
    const bw6_761_Fq D = X1 - X2 * Z1;
    // E = Y1 - Y2*Z1
    const bw6_761_Fq E = Y1 - Y2 * Z1;
    // F = D^2
    const bw6_761_Fq F = D.squared();
    // G = E^2
    const bw6_761_Fq G = E.squared();
    // H = D*F
    const bw6_761_Fq H = D * F;
    // I = X1 * F
    const bw6_761_Fq I = X1 * F;
    // J = H + Z1*G - (I+I)
    const bw6_761_Fq J = H + Z1 * G - (I + I);

    // X3 = D*J
    current.X = D * J;
    // Y3 = E*(I-J)-(H*Y1)
    current.Y = E * (I - J) - (H * Y1);
    // Z3 = Z1*H
    current.Z = Z1 * H;

    c.ell_0 = E * X2 - D * Y2;
    // VV gets multiplied to xP during line evaluation at P
    c.ell_VV = -E;
    // VW gets multiplied to yP during line evaluation at P
    c.ell_VW = bw6_761_twist * D;
}

bw6_761_ate_G1_precomp bw6_761_ate_precompute_G1(const bw6_761_G1 &P)
{
    enter_block("Call to bw6_761_ate_precompute_G1");

    bw6_761_G1 Pcopy = P;
    Pcopy.to_affine_coordinates();

    bw6_761_ate_G1_precomp result;
    result.PX = Pcopy.X;
    result.PY = Pcopy.Y;

    leave_block("Call to bw6_761_ate_precompute_G1");
    return result;
}

static bw6_761_ate_G2_precomp_iteration bw6_761_ate_precompute_G2_internal(
    const bw6_761_G2 &Q, const bigint<bw6_761_Fq::num_limbs> &loop_count)
{
    enter_block("Call to bw6_761_ate_precompute_G2");

    bw6_761_G2 Qcopy(Q);
    Qcopy.to_affine_coordinates();

    bw6_761_ate_G2_precomp_iteration result;
    result.QX = Qcopy.X;
    result.QY = Qcopy.Y;

    bw6_761_G2 R;
    R.X = Qcopy.X;
    R.Y = Qcopy.Y;
    R.Z = bw6_761_Fq::one();

    bool found_nonzero = false;
    bw6_761_ate_ell_coeffs c;

    std::vector<long> NAF = find_wnaf(1, loop_count);
    for (long i = NAF.size() - 1; i >= 0; --i) {
        if (!found_nonzero) {
            // This skips the MSB itself
            found_nonzero |= (NAF[i] != 0);
            continue;
        }

        doubling_step_for_miller_loop(R, c);
        result.coeffs.push_back(c);

        if (NAF[i] != 0) {
            if (NAF[i] > 0) {
                mixed_addition_step_for_miller_loop(Qcopy, R, c);
            } else {
                mixed_addition_step_for_miller_loop(-Qcopy, R, c);
            }
            result.coeffs.push_back(c);
        }
    }

    leave_block("Call to bw6_761_ate_precompute_G2");
    return result;
}

bw6_761_ate_G2_precomp bw6_761_ate_precompute_G2(const bw6_761_G2 &Q)
{
    return {
        bw6_761_ate_precompute_G2_internal(Q, bw6_761_ate_loop_count1),
        bw6_761_ate_precompute_G2_internal(Q, bw6_761_ate_loop_count2),
    };
}

// https://gitlab.inria.fr/zk-curves/bw6-761/-/blob/master/sage/pairing.py#L344
bw6_761_Fq6 bw6_761_ate_miller_loop(
    const bw6_761_ate_G1_precomp &prec_P, const bw6_761_ate_G2_precomp &prec_Q)
{
    enter_block("Call to bw6_761_ate_miller_loop");

    const bw6_761_ate_G2_precomp_iteration &prec_Q_1 = prec_Q.precomp_1;
    const bw6_761_ate_G2_precomp_iteration &prec_Q_2 = prec_Q.precomp_2;

    // f_{u+1,Q}(P)
    bw6_761_Fq6 f_1 = bw6_761_Fq6::one();

    bool found_nonzero_1 = false;
    size_t idx_1 = 0;

    const bigint<bw6_761_Fq::num_limbs> &loop_count_1 = bw6_761_ate_loop_count1;
    bw6_761_ate_ell_coeffs c_1;

    // Get the Non-Adjacent Form of the loop count
    // loop_count_1 = u+1
    // This allows to cover steps 2 to 11 Algorithm 5:
    // https://eprint.iacr.org/2020/351.pdf
    std::vector<long> NAF_1 = find_wnaf(1, loop_count_1);
    for (long i = NAF_1.size() - 1; i >= 0; --i) {
        if (!found_nonzero_1) {
            // This skips the MSB itself
            found_nonzero_1 |= (NAF_1[i] != 0);
            continue;
        }

        // The code below gets executed for all bits (EXCEPT the MSB itself) of
        // bw6_761_param_p (skipping leading zeros) in MSB to LSB
        // order
        c_1 = prec_Q_1.coeffs[idx_1];
        ++idx_1;
        f_1 = f_1.squared();
        f_1 = f_1.mul_by_045(
            c_1.ell_0, prec_P.PY * c_1.ell_VW, prec_P.PX * c_1.ell_VV);

        if (NAF_1[i] != 0) {
            c_1 = prec_Q_1.coeffs[idx_1];
            ++idx_1;
            f_1 = f_1.mul_by_045(
                c_1.ell_0, prec_P.PY * c_1.ell_VW, prec_P.PX * c_1.ell_VV);
        }
    }

    // f_{u^3-u^2-u,Q}(P)
    bw6_761_Fq6 f_2 = bw6_761_Fq6::one();

    bool found_nonzero_2 = false;
    size_t idx_2 = 0;

    const bigint<bw6_761_Fq::num_limbs> &loop_count_2 = bw6_761_ate_loop_count2;
    bw6_761_ate_ell_coeffs c_2;

    std::vector<long> NAF_2 = find_wnaf(1, loop_count_2);
    for (long i = NAF_2.size() - 1; i >= 0; --i) {
        if (!found_nonzero_2) {
            // This skips the MSB itself
            found_nonzero_2 |= (NAF_2[i] != 0);
            continue;
        }

        // The code below gets executed for all bits (EXCEPT the MSB itself) of
        // bw6_761_param_p (skipping leading zeros) in MSB to LSB
        // order
        c_2 = prec_Q_2.coeffs[idx_2++];
        f_2 = f_2.squared();
        f_2 = f_2.mul_by_045(
            c_2.ell_0, prec_P.PY * c_2.ell_VW, prec_P.PX * c_2.ell_VV);

        if (NAF_2[i] != 0) {
            c_2 = prec_Q_2.coeffs[idx_2++];
            f_2 = f_2.mul_by_045(
                c_2.ell_0, prec_P.PY * c_2.ell_VW, prec_P.PX * c_2.ell_VV);
        }
    }

    leave_block("Call to bw6_761_ate_miller_loop");

    f_2 = f_2.Frobenius_map(1);

    return f_1 * f_2;
}

bw6_761_Fq6 bw6_761_ate_double_miller_loop(
    const bw6_761_ate_G1_precomp &prec_P1,
    const bw6_761_ate_G2_precomp &prec_Q1,
    const bw6_761_ate_G1_precomp &prec_P2,
    const bw6_761_ate_G2_precomp &prec_Q2)
{
    enter_block("Call to bw6_761_ate_double_miller_loop");

    const bw6_761_ate_G2_precomp_iteration &prec_Q1_1 = prec_Q1.precomp_1;
    const bw6_761_ate_G2_precomp_iteration &prec_Q2_1 = prec_Q1.precomp_2;
    const bw6_761_ate_G2_precomp_iteration &prec_Q1_2 = prec_Q2.precomp_1;
    const bw6_761_ate_G2_precomp_iteration &prec_Q2_2 = prec_Q2.precomp_2;

    // f_{u+1,Q}(P)
    bw6_761_Fq6 f_1 = bw6_761_Fq6::one();

    bool found_nonzero_1 = false;
    size_t idx_1 = 0;

    bw6_761_ate_ell_coeffs c_1_1;
    bw6_761_ate_ell_coeffs c_1_2;

    const bigint<bw6_761_Fq::num_limbs> &loop_count_1 = bw6_761_ate_loop_count1;
    // Get the Non-Adjacent Form of the 1st loop count
    std::vector<long> NAF_1 = find_wnaf(1, loop_count_1);
    for (long i = NAF_1.size() - 1; i >= 0; --i) {
        if (!found_nonzero_1) {
            // This skips the MSB itself
            found_nonzero_1 |= (NAF_1[i] != 0);
            continue;
        }

        // The code below gets executed for all bits (EXCEPT the MSB itself) of
        // bw6_761_param_p (skipping leading zeros) in MSB to LSB
        // order
        c_1_1 = prec_Q1_1.coeffs[idx_1];
        c_1_2 = prec_Q1_2.coeffs[idx_1];
        ++idx_1;

        f_1 = f_1.squared();
        f_1 = f_1.mul_by_045(
            c_1_1.ell_0, prec_P1.PY * c_1_1.ell_VW, prec_P1.PX * c_1_1.ell_VV);
        f_1 = f_1.mul_by_045(
            c_1_2.ell_0, prec_P2.PY * c_1_2.ell_VW, prec_P2.PX * c_1_2.ell_VV);

        if (NAF_1[i] != 0) {
            c_1_1 = prec_Q1_1.coeffs[idx_1];
            c_1_2 = prec_Q1_2.coeffs[idx_1];
            ++idx_1;

            f_1 = f_1.mul_by_045(
                c_1_1.ell_0,
                prec_P1.PY * c_1_1.ell_VW,
                prec_P1.PX * c_1_1.ell_VV);
            f_1 = f_1.mul_by_045(
                c_1_2.ell_0,
                prec_P2.PY * c_1_2.ell_VW,
                prec_P2.PX * c_1_2.ell_VV);
        }
    }

    // f_{u^3-u^2-u,Q}(P)
    bw6_761_Fq6 f_2 = bw6_761_Fq6::one();

    bool found_nonzero_2 = false;
    size_t idx_2 = 0;

    bw6_761_ate_ell_coeffs c_2_1;
    bw6_761_ate_ell_coeffs c_2_2;

    const bigint<bw6_761_Fq::num_limbs> &loop_count_2 = bw6_761_ate_loop_count2;
    // Get the Non-Adjacent Form of the 1st loop count
    std::vector<long> NAF_2 = find_wnaf(1, loop_count_2);
    for (long i = NAF_2.size() - 1; i >= 0; --i) {
        if (!found_nonzero_2) {
            // This skips the MSB itself
            found_nonzero_2 |= (NAF_2[i] != 0);
            continue;
        }

        // The code below gets executed for all bits (EXCEPT the MSB itself) of
        // bw6_761_param_p (skipping leading zeros) in MSB to LSB
        // order
        c_2_1 = prec_Q2_1.coeffs[idx_2];
        c_2_2 = prec_Q2_2.coeffs[idx_2];
        ++idx_2;

        f_2 = f_2.squared();

        f_2 = f_2.mul_by_045(
            c_2_1.ell_0, prec_P1.PY * c_2_1.ell_VW, prec_P1.PX * c_2_1.ell_VV);
        f_2 = f_2.mul_by_045(
            c_2_2.ell_0, prec_P2.PY * c_2_2.ell_VW, prec_P2.PX * c_2_2.ell_VV);

        if (NAF_2[i] != 0) {
            c_2_1 = prec_Q2_1.coeffs[idx_2];
            c_2_2 = prec_Q2_2.coeffs[idx_2];
            ++idx_2;

            f_2 = f_2.mul_by_045(
                c_2_1.ell_0,
                prec_P1.PY * c_2_1.ell_VW,
                prec_P1.PX * c_2_1.ell_VV);
            f_2 = f_2.mul_by_045(
                c_2_2.ell_0,
                prec_P2.PY * c_2_2.ell_VW,
                prec_P2.PX * c_2_2.ell_VV);
        }
    }

    leave_block("Call to bw6_761_ate_double_miller_loop");

    f_2 = f_2.Frobenius_map(1);

    return f_1 * f_2;
}

bw6_761_Fq6 bw6_761_ate_pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q)
{
    enter_block("Call to bw6_761_ate_pairing");
    bw6_761_ate_G1_precomp prec_P = bw6_761_ate_precompute_G1(P);
    bw6_761_ate_G2_precomp prec_Q = bw6_761_ate_precompute_G2(Q);
    bw6_761_Fq6 result = bw6_761_ate_miller_loop(prec_P, prec_Q);
    leave_block("Call to bw6_761_ate_pairing");
    return result;
}

bw6_761_GT bw6_761_ate_reduced_pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q)
{
    enter_block("Call to bw6_761_ate_reduced_pairing");
    const bw6_761_Fq6 f = bw6_761_ate_pairing(P, Q);
    const bw6_761_GT result = bw6_761_final_exponentiation(f);
    leave_block("Call to bw6_761_ate_reduced_pairing");
    return result;
}

// Choice of pairing

bw6_761_G1_precomp bw6_761_precompute_G1(const bw6_761_G1 &P)
{
    return bw6_761_ate_precompute_G1(P);
}

bw6_761_G2_precomp bw6_761_precompute_G2(const bw6_761_G2 &Q)
{
    return bw6_761_ate_precompute_G2(Q);
}

bw6_761_Fq6 bw6_761_miller_loop(
    const bw6_761_G1_precomp &prec_P, const bw6_761_G2_precomp &prec_Q)
{
    return bw6_761_ate_miller_loop(prec_P, prec_Q);
}

bw6_761_Fq6 bw6_761_double_miller_loop(
    const bw6_761_ate_G1_precomp &prec_P1,
    const bw6_761_ate_G2_precomp &prec_Q1,
    const bw6_761_ate_G1_precomp &prec_P2,
    const bw6_761_ate_G2_precomp &prec_Q2)
{
    return bw6_761_ate_double_miller_loop(prec_P1, prec_Q1, prec_P2, prec_Q2);
}

bw6_761_Fq6 bw6_761_pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q)
{
    return bw6_761_ate_pairing(P, Q);
}

bw6_761_GT bw6_761_reduced_pairing(const bw6_761_G1 &P, const bw6_761_G2 &Q)
{
    return bw6_761_ate_reduced_pairing(P, Q);
}

} // namespace libff
