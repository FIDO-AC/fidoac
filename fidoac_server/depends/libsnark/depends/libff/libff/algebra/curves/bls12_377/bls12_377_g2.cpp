/** @file
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <libff/algebra/curves/bls12_377/bls12_377_g2.hpp>

namespace libff
{

#ifdef PROFILE_OP_COUNTS
long long bls12_377_G2::add_cnt = 0;
long long bls12_377_G2::dbl_cnt = 0;
#endif

std::vector<size_t> bls12_377_G2::wnaf_window_table;
std::vector<size_t> bls12_377_G2::fixed_base_exp_window_table;
bls12_377_G2 bls12_377_G2::G2_zero;
bls12_377_G2 bls12_377_G2::G2_one;
bls12_377_Fq2 bls12_377_G2::coeff_a;
bls12_377_Fq2 bls12_377_G2::coeff_b;
bigint<bls12_377_G2::h_limbs> bls12_377_G2::h;

bls12_377_G2::bls12_377_G2()
{
    this->X = G2_zero.X;
    this->Y = G2_zero.Y;
    this->Z = G2_zero.Z;
}

bls12_377_Fq2 bls12_377_G2::mul_by_b(const bls12_377_Fq2 &elt)
{
    return bls12_377_Fq2(
        bls12_377_twist_mul_by_b_c0 * elt.coeffs[0],
        bls12_377_twist_mul_by_b_c1 * elt.coeffs[1]);
}

void bls12_377_G2::print() const
{
    if (this->is_zero()) {
        printf("O\n");
    } else {
        bls12_377_G2 copy(*this);
        copy.to_affine_coordinates();
        gmp_printf(
            "(%Nd*z + %Nd , %Nd*z + %Nd)\n",
            copy.X.coeffs[1].as_bigint().data,
            bls12_377_Fq::num_limbs,
            copy.X.coeffs[0].as_bigint().data,
            bls12_377_Fq::num_limbs,
            copy.Y.coeffs[1].as_bigint().data,
            bls12_377_Fq::num_limbs,
            copy.Y.coeffs[0].as_bigint().data,
            bls12_377_Fq::num_limbs);
    }
}

void bls12_377_G2::print_coordinates() const
{
    if (this->is_zero()) {
        printf("O\n");
    } else {
        gmp_printf(
            "(%Nd*z + %Nd : %Nd*z + %Nd : %Nd*z + %Nd)\n",
            this->X.coeffs[1].as_bigint().data,
            bls12_377_Fq::num_limbs,
            this->X.coeffs[0].as_bigint().data,
            bls12_377_Fq::num_limbs,
            this->Y.coeffs[1].as_bigint().data,
            bls12_377_Fq::num_limbs,
            this->Y.coeffs[0].as_bigint().data,
            bls12_377_Fq::num_limbs,
            this->Z.coeffs[1].as_bigint().data,
            bls12_377_Fq::num_limbs,
            this->Z.coeffs[0].as_bigint().data,
            bls12_377_Fq::num_limbs);
    }
}

void bls12_377_G2::to_affine_coordinates()
{
    if (this->is_zero()) {
        this->X = bls12_377_Fq2::zero();
        this->Y = bls12_377_Fq2::one();
        this->Z = bls12_377_Fq2::zero();
    } else {
        bls12_377_Fq2 Z_inv = Z.inverse();
        bls12_377_Fq2 Z2_inv = Z_inv.squared();
        bls12_377_Fq2 Z3_inv = Z2_inv * Z_inv;
        this->X = this->X * Z2_inv;
        this->Y = this->Y * Z3_inv;
        this->Z = bls12_377_Fq2::one();
    }
}

void bls12_377_G2::to_special() { this->to_affine_coordinates(); }

bool bls12_377_G2::is_special() const
{
    return (this->is_zero() || this->Z == bls12_377_Fq2::one());
}

bool bls12_377_G2::is_zero() const { return (this->Z.is_zero()); }

bool bls12_377_G2::operator==(const bls12_377_G2 &other) const
{
    if (this->is_zero()) {
        return other.is_zero();
    }

    if (other.is_zero()) {
        return false;
    }

    // now neither is O

    // Using Jacobian coordinates so:
    //   (X1:Y1:Z1) = (X2:Y2:Z2) <=>
    //   X1/Z1^2 == X2/Z2^2 AND Y1/Z1^3 == Y2/Z2^3 <=>
    //   X1 * Z2^2 == X2 * Z1^2 AND Y1 * Z2^3 == Y2 * Z1^3
    bls12_377_Fq2 Z1_squared = (this->Z).squared();
    bls12_377_Fq2 Z2_squared = (other.Z).squared();
    bls12_377_Fq2 Z1_cubed = (this->Z) * Z1_squared;
    bls12_377_Fq2 Z2_cubed = (other.Z) * Z2_squared;
    if (((this->X * Z2_squared) != (other.X * Z1_squared)) ||
        ((this->Y * Z2_cubed) != (other.Y * Z1_cubed))) {
        return false;
    }

    return true;
}

bool bls12_377_G2::operator!=(const bls12_377_G2 &other) const
{
    return !(operator==(other));
}

bls12_377_G2 bls12_377_G2::operator+(const bls12_377_G2 &other) const
{
    // Handle special cases having to do with O
    if (this->is_zero()) {
        return other;
    }

    if (other.is_zero()) {
        return *this;
    }

    // No need to handle points of order 2,4
    // (they cannot exist in a prime-order subgroup)

    // Z1Z1 = Z1*Z1
    bls12_377_Fq2 Z1Z1 = (this->Z).squared();
    // Z2Z2 = Z2*Z2
    bls12_377_Fq2 Z2Z2 = (other.Z).squared();

    // U1 = X1*Z2Z2
    bls12_377_Fq2 U1 = this->X * Z2Z2;
    // U2 = X2*Z1Z1
    bls12_377_Fq2 U2 = other.X * Z1Z1;

    // S1 = Y1*Z2*Z2Z2
    bls12_377_Fq2 S1 = (this->Y) * ((other.Z) * Z2Z2);
    // S2 = Y2*Z1*Z1Z1
    bls12_377_Fq2 S2 = (other.Y) * ((this->Z) * Z1Z1);

    // Check if the 2 points are equal, in which can we do a point doubling
    // (i.e. P + P)
    if (U1 == U2 && S1 == S2) {
        return this->dbl();
    }

    // Point addition (i.e. P + Q, P =/= Q)
    // https://www.hyperelliptic.org/EFD/g1p/data/shortw/jacobian-0/addition/add-2007-bl
    // H = U2-U1
    bls12_377_Fq2 H = U2 - U1;
    // I = (2*H)^2
    bls12_377_Fq2 I = (H + H).squared();
    // J = H*I
    bls12_377_Fq2 J = H * I;
    // r = 2*(S2-S1)
    bls12_377_Fq2 S2_minus_S1 = S2 - S1;
    bls12_377_Fq2 r = S2_minus_S1 + S2_minus_S1;
    // V = U1*I
    bls12_377_Fq2 V = U1 * I;
    // X3 = r^2-J-2*V
    bls12_377_Fq2 X3 = r.squared() - J - (V + V);
    bls12_377_Fq2 S1_J = S1 * J;
    // Y3 = r*(V-X3)-2*S1*J
    bls12_377_Fq2 Y3 = r * (V - X3) - (S1_J + S1_J);
    // Z3 = ((Z1+Z2)^2-Z1Z1-Z2Z2) * H
    bls12_377_Fq2 Z3 = ((this->Z + other.Z).squared() - Z1Z1 - Z2Z2) * H;

    return bls12_377_G2(X3, Y3, Z3);
}

bls12_377_G2 bls12_377_G2::operator-() const
{
    return bls12_377_G2(this->X, -(this->Y), this->Z);
}

bls12_377_G2 bls12_377_G2::operator-(const bls12_377_G2 &other) const
{
    return (*this) + (-other);
}

bls12_377_G2 bls12_377_G2::add(const bls12_377_G2 &other) const
{
    // handle special cases having to do with O
    if (this->is_zero()) {
        return other;
    }

    if (other.is_zero()) {
        return *this;
    }

    // No need to handle points of order 2,4
    // (they cannot exist in a prime-order subgroup)

    // Handle double case
    if (this->operator==(other)) {
        return this->dbl();
    }

#ifdef PROFILE_OP_COUNTS
    this->add_cnt++;
#endif
    // NOTE: does not handle O and pts of order 2,4
    // https://www.hyperelliptic.org/EFD/g1p/data/shortw/jacobian-0/addition/add-2007-bl
    // Z1Z1 = Z1*Z1
    bls12_377_Fq2 Z1Z1 = (this->Z).squared();
    // Z2Z2 = Z2*Z2
    bls12_377_Fq2 Z2Z2 = (other.Z).squared();
    // U1 = X1*Z2Z2
    bls12_377_Fq2 U1 = this->X * Z2Z2;
    // U2 = X2*Z1Z1
    bls12_377_Fq2 U2 = other.X * Z1Z1;
    // S1 = Y1*Z2*Z2Z2
    bls12_377_Fq2 S1 = (this->Y) * ((other.Z) * Z2Z2);
    // S2 = Y2*Z1*Z1Z1
    bls12_377_Fq2 S2 = (other.Y) * ((this->Z) * Z1Z1);
    // H = U2-U1
    bls12_377_Fq2 H = U2 - U1;
    // I = (2*H)^2
    bls12_377_Fq2 I = (H + H).squared();
    // J = H*I
    bls12_377_Fq2 J = H * I;
    // r = 2*(S2-S1)
    bls12_377_Fq2 S2_minus_S1 = S2 - S1;
    bls12_377_Fq2 r = S2_minus_S1 + S2_minus_S1;
    // V = U1*I
    bls12_377_Fq2 V = U1 * I;
    // X3 = r^2-J-2*V
    bls12_377_Fq2 X3 = r.squared() - J - (V + V);
    bls12_377_Fq2 S1_J = S1 * J;
    // Y3 = r*(V-X3)-2*S1*J
    bls12_377_Fq2 Y3 = r * (V - X3) - (S1_J + S1_J);
    // Z3 = ((Z1+Z2)^2-Z1Z1-Z2Z2) * H
    bls12_377_Fq2 Z3 = ((this->Z + other.Z).squared() - Z1Z1 - Z2Z2) * H;

    return bls12_377_G2(X3, Y3, Z3);
}

// This function assumes that:
// *this is of the form (X1/Z1, Y1/Z1), and that
// other is of the form (X2, Y2), i.e. Z2=1
bls12_377_G2 bls12_377_G2::mixed_add(const bls12_377_G2 &other) const
{
#ifdef DEBUG
    assert(other.is_special());
#endif

    if (this->is_zero()) {
        return other;
    }

    if (other.is_zero()) {
        return *this;
    }

    // No need to handle points of order 2,4
    // (they cannot exist in a prime-order subgroup)

    // Z1Z1 = Z1*Z1
    const bls12_377_Fq2 Z1Z1 = (this->Z).squared();
    // U2 = X2*Z1Z1
    const bls12_377_Fq2 U2 = other.X * Z1Z1;
    // S2 = Y2 * Z1 * Z1Z1
    const bls12_377_Fq2 S2 = (other.Y) * ((this->Z) * Z1Z1);

    // (X1/Z1^2) == X2 => X1 == X2*Z1^2
    // (Y1/Z1^3) == Y2 => Y1 == Y2*Z1^3
    if (this->X == U2 && this->Y == S2) {
        return this->dbl();
    }

#ifdef PROFILE_OP_COUNTS
    this->add_cnt++;
#endif

    // NOTE: does not handle O and pts of order 2,4
    // https://www.hyperelliptic.org/EFD/g1p/data/shortw/jacobian-0/addition/madd-2007-bl
    // H = U2-X1
    bls12_377_Fq2 H = U2 - (this->X);
    // HH = H^2
    bls12_377_Fq2 HH = H.squared();
    // I = 4*HH
    bls12_377_Fq2 I = HH + HH;
    I = I + I;
    // J = H*I
    bls12_377_Fq2 J = H * I;
    // r = 2*(S2-Y1)
    bls12_377_Fq2 r = S2 - (this->Y);
    r = r + r;
    // V = X1*I
    bls12_377_Fq2 V = (this->X) * I;
    // X3 = r^2-J-2*V
    bls12_377_Fq2 X3 = r.squared() - J - V - V;
    // Y3 = r*(V-X3)-2*Y1*J
    bls12_377_Fq2 Y3 = (this->Y) * J;
    Y3 = r * (V - X3) - Y3 - Y3;
    // Z3 = (Z1+H)^2-Z1Z1-HH
    bls12_377_Fq2 Z3 = ((this->Z) + H).squared() - Z1Z1 - HH;

    return bls12_377_G2(X3, Y3, Z3);
}

bls12_377_G2 bls12_377_G2::dbl() const
{
#ifdef PROFILE_OP_COUNTS
    this->dbl_cnt++;
#endif
    // Handle point at infinity
    if (this->is_zero()) {
        return (*this);
    }

    // NOTE: does not handle O and pts of order 2,4
    // https://www.hyperelliptic.org/EFD/g1p/data/shortw/jacobian-0/doubling/dbl-2009-l
    // A = X1^2
    bls12_377_Fq2 A = (this->X).squared();
    // B = Y1^2
    bls12_377_Fq2 B = (this->Y).squared();
    // C = B^2
    bls12_377_Fq2 C = B.squared();
    // D = 2 * ((X1 + B)^2 - A - C)
    bls12_377_Fq2 D = (this->X + B).squared() - A - C;
    D = D + D;
    // E = 3 * A
    bls12_377_Fq2 E = A + A + A;
    // F = E^2
    bls12_377_Fq2 F = E.squared();
    // X3 = F - 2 D
    bls12_377_Fq2 X3 = F - (D + D);
    // Y3 = E * (D - X3) - 8 * C
    bls12_377_Fq2 eightC = C + C;
    eightC = eightC + eightC;
    eightC = eightC + eightC;
    bls12_377_Fq2 Y3 = E * (D - X3) - eightC;
    // Z3 = 2 * Y1 * Z1
    bls12_377_Fq2 Y1Z1 = (this->Y) * (this->Z);
    bls12_377_Fq2 Z3 = Y1Z1 + Y1Z1;

    return bls12_377_G2(X3, Y3, Z3);
}

bls12_377_G2 bls12_377_G2::mul_by_q() const
{
    return bls12_377_G2(
        bls12_377_twist_mul_by_q_X * (this->X).Frobenius_map(1),
        bls12_377_twist_mul_by_q_Y * (this->Y).Frobenius_map(1),
        (this->Z).Frobenius_map(1));
}

bls12_377_G2 bls12_377_G2::untwist_frobenius_twist() const
{
    bls12_377_G2 g = *this;
    g.to_affine_coordinates();

    // Note, the algebra works out such that the first component of the
    // untwisted point only ever occupies Fq6, and so we use this type to avoid
    // the extra multiplications involved in Fq12 operations.

    // TODO: There are further optimizations we can make here, because we know
    // that many components will be zero and unused. For now, we use generic
    // Fp6 and Fp12 operations for conveneience.

    // Untwist
    const bls12_377_Fq6 x_fq6(
        g.X, bls12_377_Fq2::zero(), bls12_377_Fq2::zero());
    const bls12_377_Fq12 y_fq12(
        bls12_377_Fq6(g.Y, bls12_377_Fq2::zero(), bls12_377_Fq2::zero()),
        bls12_377_Fq6::zero());
    const bls12_377_Fq6 untwist_x =
        x_fq6 * bls12_377_g2_untwist_frobenius_twist_v.coeffs[0];
    const bls12_377_Fq12 untwist_y =
        y_fq12 * bls12_377_g2_untwist_frobenius_twist_w_3;
    // Frobenius
    const bls12_377_Fq6 frob_untwist_x = untwist_x.Frobenius_map(1);
    const bls12_377_Fq12 frob_untwist_y = untwist_y.Frobenius_map(1);
    // Twist
    const bls12_377_Fq6 twist_frob_untwist_x =
        frob_untwist_x *
        bls12_377_g2_untwist_frobenius_twist_v_inverse.coeffs[0];
    const bls12_377_Fq12 twist_frob_untwist_y =
        frob_untwist_y * bls12_377_g2_untwist_frobenius_twist_w_3_inverse;

    assert(twist_frob_untwist_x.coeffs[2] == bls12_377_Fq2::zero());
    assert(twist_frob_untwist_x.coeffs[1] == bls12_377_Fq2::zero());
    assert(twist_frob_untwist_y.coeffs[1] == bls12_377_Fq6::zero());
    assert(twist_frob_untwist_y.coeffs[0].coeffs[2] == bls12_377_Fq2::zero());
    assert(twist_frob_untwist_y.coeffs[0].coeffs[1] == bls12_377_Fq2::zero());

    return bls12_377_G2(
        twist_frob_untwist_x.coeffs[0],
        twist_frob_untwist_y.coeffs[0].coeffs[0],
        bls12_377_Fq2::one());
}

bls12_377_G2 bls12_377_G2::mul_by_cofactor() const
{
    // See bls12_377.sage.
    // [h2]P = [h2_0]P + [h2_1]([t] psi_p - psi_2_p)
    // where:
    //   h2_0 = 293634935485640680722085584138834120318524213360527933441
    //   h2_1 = 30631250834960419227450344600217059328
    //   t = 9586122913090633730
    const bls12_377_G2 psi_p = untwist_frobenius_twist();
    const bls12_377_G2 psi_2_p = psi_p.untwist_frobenius_twist();
    const bls12_377_G2 t_psi_mins_psi_2 =
        bls12_377_trace_of_frobenius * psi_p - psi_2_p;
    const bls12_377_G2 result =
        bls12_377_g2_mul_by_cofactor_h2_0 * (*this) +
        bls12_377_g2_mul_by_cofactor_h2_1 * t_psi_mins_psi_2;
    return result;
}

bool bls12_377_G2::is_well_formed() const
{
    if (this->is_zero()) {
        return true;
    }

    // The curve equation is
    // E': y^2 = x^3 + ax + b', where a=0 and b'= b*xi
    // We are using Jacobian coordinates. As such, the equation becomes:
    // y^2/z^6 = x^3/z^6 + b'
    // = y^2 = x^3  + b' z^6
    bls12_377_Fq2 X2 = this->X.squared();
    bls12_377_Fq2 Y2 = this->Y.squared();
    bls12_377_Fq2 Z2 = this->Z.squared();
    bls12_377_Fq2 X3 = this->X * X2;
    bls12_377_Fq2 Z3 = this->Z * Z2;
    bls12_377_Fq2 Z6 = Z3.squared();
    return (Y2 == X3 + bls12_377_twist_coeff_b * Z6);
}

bool bls12_377_G2::is_in_safe_subgroup() const
{
    // Check that [h1.r]P == 0, where
    //   [h1.r]P as P + [t](\psi(P) - P) - \psi^2(P)
    // (See bls12_377.sage).

    const bls12_377_G2 psi_p = untwist_frobenius_twist();
    const bls12_377_G2 psi_2_p = psi_p.untwist_frobenius_twist();
    const bls12_377_G2 psi_p_minus_p = psi_p - *this;
    const bls12_377_G2 h1_r_p =
        *this + bls12_377_trace_of_frobenius * psi_p_minus_p - psi_2_p;
    return zero() == h1_r_p;
}

const bls12_377_G2 &bls12_377_G2::zero() { return G2_zero; }

const bls12_377_G2 &bls12_377_G2::one() { return G2_one; }

bls12_377_G2 bls12_377_G2::random_element()
{
    return (bls12_377_Fr::random_element().as_bigint()) * G2_one;
}

void bls12_377_G2::write_uncompressed(std::ostream &out) const
{
    bls12_377_G2 copy(*this);
    copy.to_affine_coordinates();
    out << (copy.is_zero() ? 1 : 0) << OUTPUT_SEPARATOR;
    out << copy.X << OUTPUT_SEPARATOR << copy.Y;
}

void bls12_377_G2::write_compressed(std::ostream &out) const
{
    bls12_377_G2 copy(*this);
    copy.to_affine_coordinates();
    out << (copy.is_zero() ? 1 : 0) << OUTPUT_SEPARATOR;
    /* storing LSB of Y */
    out << copy.X << OUTPUT_SEPARATOR
        << (copy.Y.coeffs[0].as_bigint().data[0] & 1);
}

void bls12_377_G2::read_uncompressed(std::istream &in, bls12_377_G2 &g)
{
    char is_zero;
    bls12_377_Fq2 tX, tY;
    in >> is_zero >> tX >> tY;
    is_zero -= '0';

    if (!is_zero) {
        g.X = tX;
        g.Y = tY;
        g.Z = bls12_377_Fq2::one();
    } else {
        g = bls12_377_G2::zero();
    }
}

void bls12_377_G2::read_compressed(std::istream &in, bls12_377_G2 &g)
{
    char is_zero;
    bls12_377_Fq2 tX, tY;
    // this reads is_zero;
    in.read((char *)&is_zero, 1);
    is_zero -= '0';
    consume_OUTPUT_SEPARATOR(in);

    unsigned char Y_lsb;
    in >> tX;
    consume_OUTPUT_SEPARATOR(in);
    in.read((char *)&Y_lsb, 1);
    Y_lsb -= '0';

    // y = +/- sqrt(x^3 + b)
    if (!is_zero) {
        bls12_377_Fq2 tX2 = tX.squared();
        bls12_377_Fq2 tY2 = tX2 * tX + bls12_377_twist_coeff_b;
        tY = tY2.sqrt();

        if ((tY.coeffs[0].as_bigint().data[0] & 1) != Y_lsb) {
            tY = -tY;
        }

        g.X = tX;
        g.Y = tY;
        g.Z = bls12_377_Fq2::one();
    } else {
        g = bls12_377_G2::zero();
    }
}

std::ostream &operator<<(std::ostream &out, const bls12_377_G2 &g)
{
#ifdef NO_PT_COMPRESSION
    g.write_uncompressed(out);
#else
    g.write_compressed(out);
#endif
    return out;
}

std::istream &operator>>(std::istream &in, bls12_377_G2 &g)
{
#ifdef NO_PT_COMPRESSION
    bls12_377_G2::read_uncompressed(in, g);
#else
    bls12_377_G2::read_compressed(in, g);
#endif
    return in;
}
void bls12_377_G2::batch_to_special_all_non_zeros(
    std::vector<bls12_377_G2> &vec)
{
    std::vector<bls12_377_Fq2> Z_vec;
    Z_vec.reserve(vec.size());

    for (auto &el : vec) {
        Z_vec.emplace_back(el.Z);
    }
    batch_invert<bls12_377_Fq2>(Z_vec);

    const bls12_377_Fq2 one = bls12_377_Fq2::one();

    for (size_t i = 0; i < vec.size(); ++i) {
        bls12_377_Fq2 Z2 = Z_vec[i].squared();
        bls12_377_Fq2 Z3 = Z_vec[i] * Z2;

        vec[i].X = vec[i].X * Z2;
        vec[i].Y = vec[i].Y * Z3;
        vec[i].Z = one;
    }
}

} // namespace libff
