#include <libff/algebra/curves/bw6_761/bw6_761_g1.hpp>

namespace libff
{

#ifdef PROFILE_OP_COUNTS
long long bw6_761_G1::add_cnt = 0;
long long bw6_761_G1::dbl_cnt = 0;
#endif

std::vector<size_t> bw6_761_G1::wnaf_window_table;
std::vector<size_t> bw6_761_G1::fixed_base_exp_window_table;
bw6_761_G1 bw6_761_G1::G1_zero;
bw6_761_G1 bw6_761_G1::G1_one;
bw6_761_Fq bw6_761_G1::coeff_a;
bw6_761_Fq bw6_761_G1::coeff_b;
bigint<bw6_761_G1::h_limbs> bw6_761_G1::h;

bw6_761_G1::bw6_761_G1()
{
    this->X = G1_zero.X;
    this->Y = G1_zero.Y;
    this->Z = G1_zero.Z;
}

void bw6_761_G1::print() const
{
    if (this->is_zero()) {
        printf("O\n");
    } else {
        bw6_761_G1 copy(*this);
        copy.to_affine_coordinates();
        gmp_printf(
            "(%Nd , %Nd)\n",
            copy.X.as_bigint().data,
            bw6_761_Fq::num_limbs,
            copy.Y.as_bigint().data,
            bw6_761_Fq::num_limbs);
    }
}

void bw6_761_G1::print_coordinates() const
{
    if (this->is_zero()) {
        printf("O\n");
    } else {
        gmp_printf(
            "(%Nd : %Nd : %Nd)\n",
            this->X.as_bigint().data,
            bw6_761_Fq::num_limbs,
            this->Y.as_bigint().data,
            bw6_761_Fq::num_limbs,
            this->Z.as_bigint().data,
            bw6_761_Fq::num_limbs);
    }
}

void bw6_761_G1::to_affine_coordinates()
{
    if (this->is_zero()) {
        this->X = bw6_761_Fq::zero();
        this->Y = bw6_761_Fq::one();
        this->Z = bw6_761_Fq::zero();
    } else {
        const bw6_761_Fq Z_inv = Z.inverse();
        this->X = this->X * Z_inv;
        this->Y = this->Y * Z_inv;
        this->Z = bw6_761_Fq::one();
    }
}

void bw6_761_G1::to_special() { this->to_affine_coordinates(); }

bool bw6_761_G1::is_special() const
{
    return (this->is_zero() || this->Z == bw6_761_Fq::one());
}

bool bw6_761_G1::is_zero() const
{
    return (this->X.is_zero() && this->Z.is_zero());
}

bool bw6_761_G1::operator==(const bw6_761_G1 &other) const
{
    if (this->is_zero()) {
        return other.is_zero();
    }

    if (other.is_zero()) {
        return false;
    }

    // Using Projective coordinates so:
    //   (X1:Y1:Z1) = (X2:Y2:Z2) <=>
    //   X1/Z1 = X2/Z2 AND Y1/Z1 = Y2/Z2 <=>
    //   X1*Z2 = X2*Z1 AND Y1*Z2 = Y2*Z1
    if (((this->X * other.Z) != (other.X * this->Z)) ||
        ((this->Y * other.Z) != (other.Y * this->Z))) {
        return false;
    }

    return true;
}

bool bw6_761_G1::operator!=(const bw6_761_G1 &other) const
{
    return !(operator==(other));
}

bw6_761_G1 bw6_761_G1::operator+(const bw6_761_G1 &other) const
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

    // X1Z2 = X1*Z2
    const bw6_761_Fq X1Z2 = (this->X) * (other.Z);
    // X2Z1 = X2*Z1
    const bw6_761_Fq X2Z1 = (this->Z) * (other.X);

    // Y1Z2 = Y1*Z2
    const bw6_761_Fq Y1Z2 = (this->Y) * (other.Z);
    // Y2Z1 = Y2*Z1
    const bw6_761_Fq Y2Z1 = (this->Z) * (other.Y);

    // Check if the 2 points are equal, in which can we do a point doubling
    // (i.e. P + P)
    // https://www.hyperelliptic.org/EFD/g1p/data/shortw/projective/doubling/dbl-2007-bl
    if (X1Z2 == X2Z1 && Y1Z2 == Y2Z1) {
        // XX = X1^2
        const bw6_761_Fq XX = (this->X).squared();
        // w = a*ZZ + 3*XX = 3*XX (since a=0)
        const bw6_761_Fq w = XX + XX + XX;
        const bw6_761_Fq Y1Z1 = (this->Y) * (this->Z);
        // s = 2*Y1*Z1
        const bw6_761_Fq s = Y1Z1 + Y1Z1;
        // ss = s^2
        const bw6_761_Fq ss = s.squared();
        // sss = s*ss
        const bw6_761_Fq sss = s * ss;
        // R = Y1*s
        const bw6_761_Fq R = (this->Y) * s;
        // RR = R^2
        const bw6_761_Fq RR = R.squared();
        // B = (X1+R)^2 - XX - RR
        const bw6_761_Fq B = ((this->X) + R).squared() - XX - RR;
        // h = w^2 - 2*B
        const bw6_761_Fq h = w.squared() - (B + B);
        // X3 = h*s
        const bw6_761_Fq X3 = h * s;
        // Y3 = w*(B-h) - 2*RR
        const bw6_761_Fq Y3 = w * (B - h) - (RR + RR);
        // Z3 = sss
        const bw6_761_Fq Z3 = sss;

        return bw6_761_G1(X3, Y3, Z3);
    }

    // Point addition (i.e. P + Q, P =/= Q)
    // https://www.hyperelliptic.org/EFD/g1p/data/shortw/projective/addition/add-1998-cmo-2
    // Z1Z2 = Z1*Z2
    const bw6_761_Fq Z1Z2 = (this->Z) * (other.Z);
    // u = Y2*Z1-Y1Z2
    const bw6_761_Fq u = Y2Z1 - Y1Z2;
    // uu = u^2
    const bw6_761_Fq uu = u.squared();
    // v = X2*Z1-X1Z2
    const bw6_761_Fq v = X2Z1 - X1Z2;
    // vv = v^2
    const bw6_761_Fq vv = v.squared();
    // vvv = v*vv
    const bw6_761_Fq vvv = v * vv;
    // R = vv*X1Z2
    const bw6_761_Fq R = vv * X1Z2;
    // A = uu*Z1Z2 - vvv - 2*R
    const bw6_761_Fq A = uu * Z1Z2 - (vvv + R + R);
    // X3 = v*A
    const bw6_761_Fq X3 = v * A;
    // Y3 = u*(R-A) - vvv*Y1Z2
    const bw6_761_Fq Y3 = u * (R - A) - vvv * Y1Z2;
    // Z3 = vvv*Z1Z2
    const bw6_761_Fq Z3 = vvv * Z1Z2;

    return bw6_761_G1(X3, Y3, Z3);
}

bw6_761_G1 bw6_761_G1::operator-() const
{
    return bw6_761_G1(this->X, -(this->Y), this->Z);
}

bw6_761_G1 bw6_761_G1::operator-(const bw6_761_G1 &other) const
{
    return (*this) + (-other);
}

bw6_761_G1 bw6_761_G1::add(const bw6_761_G1 &other) const
{
    if (this->is_zero()) {
        return other;
    }

    if (other.is_zero()) {
        return (*this);
    }

    // No need to handle points of order 2,4
    // (they cannot exist in a prime-order subgroup)

    // Point doubling (i.e. P + P)
    if (this->operator==(other)) {
        return this->dbl();
    }

#ifdef PROFILE_OP_COUNTS
    this->add_cnt++;
#endif
    // NOTE: does not handle O and pts of order 2,4
    // Point addition (i.e. P + Q, P =/= Q)
    // http://www.hyperelliptic.org/EFD/g1p/auto-shortw-projective.html#addition-add-1998-cmo-2
    // Y1Z2 = Y1*Z2
    const bw6_761_Fq Y1Z2 = (this->Y) * (other.Z);
    // X1Z2 = X1*Z2
    const bw6_761_Fq X1Z2 = (this->X) * (other.Z);
    // Z1Z2 = Z1*Z2
    const bw6_761_Fq Z1Z2 = (this->Z) * (other.Z);
    // u = Y2*Z1-Y1Z2
    const bw6_761_Fq u = (other.Y) * (this->Z) - Y1Z2;
    // uu = u^2
    const bw6_761_Fq uu = u.squared();
    // v = X2*Z1-X1Z2
    const bw6_761_Fq v = (other.X) * (this->Z) - X1Z2;
    // vv = v^2
    const bw6_761_Fq vv = v.squared();
    // vvv = v*vv
    const bw6_761_Fq vvv = v * vv;
    // R = vv*X1Z2
    const bw6_761_Fq R = vv * X1Z2;
    // A = uu*Z1Z2 - vvv - 2*R
    const bw6_761_Fq A = uu * Z1Z2 - (vvv + R + R);
    // X3 = v*A
    const bw6_761_Fq X3 = v * A;
    // Y3 = u*(R-A) - vvv*Y1Z2
    const bw6_761_Fq Y3 = u * (R - A) - vvv * Y1Z2;
    // Z3 = vvv*Z1Z2
    const bw6_761_Fq Z3 = vvv * Z1Z2;

    return bw6_761_G1(X3, Y3, Z3);
}

// This function assumes that:
// *this is of the form (X1/Z1, Y1/Z1), and that
// other is of the form (X2, Y2), i.e. Z2=1
bw6_761_G1 bw6_761_G1::mixed_add(const bw6_761_G1 &other) const
{
#ifdef DEBUG
    assert(other.is_special());
#endif

    if (this->is_zero()) {
        return other;
    }

    if (other.is_zero()) {
        return (*this);
    }

    // X2Z1 = X2*Z1
    const bw6_761_Fq X2Z1 = (this->Z) * (other.X);
    // Y2Z1 = Y2*Z1
    const bw6_761_Fq Y2Z1 = (this->Z) * (other.Y);

    // (X1/Z1) == X2 => X1 == X2Z1
    // (Y1/Z1) == Y2 => Y1 == Y2Z1
    if (this->X == X2Z1 && this->Y == Y2Z1) {
        return this->dbl();
    }

#ifdef PROFILE_OP_COUNTS
    this->add_cnt++;
#endif

    // Mixed point addition
    // https://www.hyperelliptic.org/EFD/g1p/data/shortw/projective/addition/madd-1998-cmo
    // u = Y2*Z1-Y1
    bw6_761_Fq u = Y2Z1 - this->Y;
    // uu = u2
    bw6_761_Fq uu = u.squared();
    // v = X2*Z1-X1
    bw6_761_Fq v = X2Z1 - this->X;
    // vv = v2
    bw6_761_Fq vv = v.squared();
    // vvv = v*vv
    bw6_761_Fq vvv = v * vv;
    // R = vv*X1
    bw6_761_Fq R = vv * this->X;
    // A = uu*Z1-vvv-2*R
    bw6_761_Fq A = uu * this->Z - vvv - R - R;
    // X3 = v*A
    bw6_761_Fq X3 = v * A;
    // Y3 = u*(R-A)-vvv*Y1
    bw6_761_Fq Y3 = u * (R - A) - vvv * this->Y;
    // Z3 = vvv*Z1
    bw6_761_Fq Z3 = vvv * this->Z;

    return bw6_761_G1(X3, Y3, Z3);
}

bw6_761_G1 bw6_761_G1::dbl() const
{
#ifdef PROFILE_OP_COUNTS
    this->dbl_cnt++;
#endif
    if (this->is_zero()) {
        return (*this);
    }

    // NOTE: does not handle O and pts of order 2,4
    // (they cannot exist in a prime-order subgroup)

    // Point doubling (i.e. P + P)
    // https://www.hyperelliptic.org/EFD/g1p/data/shortw/projective/doubling/dbl-2007-bl
    // XX = X1^2
    const bw6_761_Fq XX = (this->X).squared();
    // w = a*ZZ + 3*XX = 3*XX (since a=0)
    const bw6_761_Fq w = XX + XX + XX;
    const bw6_761_Fq Y1Z1 = (this->Y) * (this->Z);
    // s = 2*Y1*Z1
    const bw6_761_Fq s = Y1Z1 + Y1Z1;
    // ss = s^2
    const bw6_761_Fq ss = s.squared();
    // sss = s*ss
    const bw6_761_Fq sss = s * ss;
    // R = Y1*s
    const bw6_761_Fq R = (this->Y) * s;
    // RR = R^2
    const bw6_761_Fq RR = R.squared();
    // B = (X1+R)^2 - XX - RR
    const bw6_761_Fq B = ((this->X) + R).squared() - XX - RR;
    // h = w^2 - 2*B
    const bw6_761_Fq h = w.squared() - (B + B);
    // X3 = h*s
    const bw6_761_Fq X3 = h * s;
    // Y3 = w*(B-h) - 2*RR
    const bw6_761_Fq Y3 = w * (B - h) - (RR + RR);
    // Z3 = sss
    const bw6_761_Fq Z3 = sss;
    return bw6_761_G1(X3, Y3, Z3);
}

bw6_761_G1 bw6_761_G1::mul_by_cofactor() const
{
    return bw6_761_G1::h * (*this);
}

bool bw6_761_G1::is_well_formed() const
{
    if (this->is_zero()) {
        return true;
    }

    // The curve equation is
    // y^2 = x^3 + ax + b, where a=0 and b=-1
    // We are using Projective coordinates. As such, the equation becomes:
    // (y/z)^2 = (x/z)^3 + a (x/z) + b
    // = z y^2 = x^3  + a z^2 x + b z^3
    // = z (y^2 - b z^2) = x ( x^2 + a z^2)
    // = z (y^2 - b z^2) = x^3, since a = 0
    const bw6_761_Fq X2 = this->X.squared();
    const bw6_761_Fq Y2 = this->Y.squared();
    const bw6_761_Fq Z2 = this->Z.squared();

    return (this->Z * (Y2 - bw6_761_coeff_b * Z2) == this->X * X2);
}

bool bw6_761_G1::is_in_safe_subgroup() const
{
    return zero() == scalar_field::mod * (*this);
}

const bw6_761_G1 &bw6_761_G1::zero() { return G1_zero; }

const bw6_761_G1 &bw6_761_G1::one() { return G1_one; }

bw6_761_G1 bw6_761_G1::random_element()
{
    return (scalar_field::random_element().as_bigint()) * G1_one;
}

void bw6_761_G1::write_uncompressed(std::ostream &out) const
{
    bw6_761_G1 copy(*this);
    copy.to_affine_coordinates();
    out << (copy.is_zero() ? 1 : 0) << OUTPUT_SEPARATOR;
    out << copy.X << OUTPUT_SEPARATOR << copy.Y;
}

void bw6_761_G1::write_compressed(std::ostream &out) const
{
    bw6_761_G1 copy(*this);
    copy.to_affine_coordinates();
    out << (copy.is_zero() ? 1 : 0) << OUTPUT_SEPARATOR;
    /* storing LSB of Y */
    out << copy.X << OUTPUT_SEPARATOR << (copy.Y.as_bigint().data[0] & 1);
}

std::ostream &operator<<(std::ostream &out, const bw6_761_G1 &g)
{
#ifdef NO_PT_COMPRESSION
    g.write_uncompressed(out);
#else
    g.write_compressed(out);
#endif
    return out;
}

void bw6_761_G1::read_uncompressed(std::istream &in, bw6_761_G1 &g)
{
    char is_zero;
    bw6_761_Fq tX, tY;

    in >> is_zero >> tX >> tY;
    is_zero -= '0';

    // using projective coordinates
    if (!is_zero) {
        g.X = tX;
        g.Y = tY;
        g.Z = bw6_761_Fq::one();
    } else {
        g = bw6_761_G1::zero();
    }
}

void bw6_761_G1::read_compressed(std::istream &in, bw6_761_G1 &g)
{
    char is_zero;
    bw6_761_Fq tX, tY;

    // this reads is_zero;
    in.read((char *)&is_zero, 1);
    is_zero -= '0';
    consume_OUTPUT_SEPARATOR(in);

    unsigned char Y_lsb;
    in >> tX;
    consume_OUTPUT_SEPARATOR(in);
    in.read((char *)&Y_lsb, 1);
    Y_lsb -= '0';

    // y = +/- sqrt(x^3 + a*x + b)
    if (!is_zero) {
        // using Projective coordinates
        bw6_761_Fq tX2 = tX.squared();
        bw6_761_Fq tY2 = tX2 * tX + bw6_761_coeff_b;
        tY = tY2.sqrt();

        if ((tY.as_bigint().data[0] & 1) != Y_lsb) {
            tY = -tY;
        }

        g.X = tX;
        g.Y = tY;
        g.Z = bw6_761_Fq::one();
    } else {
        g = bw6_761_G1::zero();
    }
}

std::istream &operator>>(std::istream &in, bw6_761_G1 &g)
{
#ifdef NO_PT_COMPRESSION
    bw6_761_G1::read_uncompressed(in, g);
#else
    bw6_761_G1::read_compressed(in, g);
#endif
    return in;
}

void bw6_761_G1::batch_to_special_all_non_zeros(std::vector<bw6_761_G1> &vec)
{
    std::vector<bw6_761_Fq> Z_vec;
    Z_vec.reserve(vec.size());

    for (auto &el : vec) {
        Z_vec.emplace_back(el.Z);
    }
    batch_invert<bw6_761_Fq>(Z_vec);

    const bw6_761_Fq one = bw6_761_Fq::one();

    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i].X = vec[i].X * Z_vec[i];
        vec[i].Y = vec[i].Y * Z_vec[i];
        vec[i].Z = one;
    }
}

} // namespace libff
