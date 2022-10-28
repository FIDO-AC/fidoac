/** @file
 *****************************************************************************
 Implementation of arithmetic in the finite field F[(p^3)^2].
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef FP6_2OVER3_TCC_
#define FP6_2OVER3_TCC_
#include <libff/algebra/fields/field_utils.hpp>
#include <libff/algebra/scalar_multiplication/wnaf.hpp>

namespace libff
{

template<mp_size_t n, const bigint<n> &modulus>
Fp3_model<n, modulus> Fp6_2over3_model<n, modulus>::mul_by_non_residue(
    const Fp3_model<n, modulus> &elem)
{
    return Fp3_model<n, modulus>(
        non_residue * elem.coeffs[2], elem.coeffs[0], elem.coeffs[1]);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::zero()
{
    return Fp6_2over3_model<n, modulus>(my_Fp3::zero(), my_Fp3::zero());
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::one()
{
    return Fp6_2over3_model<n, modulus>(my_Fp3::one(), my_Fp3::zero());
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::random_element()
{
    Fp6_2over3_model<n, modulus> r;
    r.coeffs[0] = my_Fp3::random_element();
    r.coeffs[1] = my_Fp3::random_element();

    return r;
}

template<mp_size_t n, const bigint<n> &modulus>
bool Fp6_2over3_model<n, modulus>::operator==(
    const Fp6_2over3_model<n, modulus> &other) const
{
    return (
        this->coeffs[0] == other.coeffs[0] &&
        this->coeffs[1] == other.coeffs[1]);
}

template<mp_size_t n, const bigint<n> &modulus>
bool Fp6_2over3_model<n, modulus>::operator!=(
    const Fp6_2over3_model<n, modulus> &other) const
{
    return !(operator==(other));
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::operator+(
    const Fp6_2over3_model<n, modulus> &other) const
{
    return Fp6_2over3_model<n, modulus>(
        this->coeffs[0] + other.coeffs[0], this->coeffs[1] + other.coeffs[1]);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::operator-(
    const Fp6_2over3_model<n, modulus> &other) const
{
    return Fp6_2over3_model<n, modulus>(
        this->coeffs[0] - other.coeffs[0], this->coeffs[1] - other.coeffs[1]);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> operator*(
    const Fp_model<n, modulus> &lhs, const Fp6_2over3_model<n, modulus> &rhs)
{
    return Fp6_2over3_model<n, modulus>(
        lhs * rhs.coeffs[0], lhs * rhs.coeffs[1]);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::operator*(
    const Fp6_2over3_model<n, modulus> &other) const
{
    // Devegili OhEig Scott Dahab --- Multiplication and Squaring on
    // Pairing-Friendly Fields.pdf; Section 3 (Karatsuba)

    const my_Fp3 &B = other.coeffs[1], &A = other.coeffs[0],
                 &b = this->coeffs[1], &a = this->coeffs[0];
    const my_Fp3 aA = a * A;
    const my_Fp3 bB = b * B;
    const my_Fp3 beta_bB = Fp6_2over3_model<n, modulus>::mul_by_non_residue(bB);

    return Fp6_2over3_model<n, modulus>(
        aA + beta_bB, (a + b) * (A + B) - aA - bB);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::mul_by_045(
    const Fp_model<n, modulus> &ell_0,
    const Fp_model<n, modulus> &ell_VW,
    const Fp_model<n, modulus> &ell_VV) const
{
    // OLD
    //   Fp6_2over3_model<n,modulus> a(
    //     my_Fp3(ell_VW, my_Fp::zero(), my_Fp::zero()),
    //     my_Fp3(my_Fp::zero(),
    //     ell_0,
    //     ell_VV));
    //   return (*this) * a;

    my_Fp z0 = this->coeffs[0].coeffs[0];
    my_Fp z1 = this->coeffs[0].coeffs[1];
    my_Fp z2 = this->coeffs[0].coeffs[2];
    my_Fp z3 = this->coeffs[1].coeffs[0];
    my_Fp z4 = this->coeffs[1].coeffs[1];
    my_Fp z5 = this->coeffs[1].coeffs[2];

    my_Fp x0 = ell_VW;
    my_Fp x4 = ell_0;
    my_Fp x5 = ell_VV;

    my_Fp t0, t1, t2, t3, t4, t5;
    my_Fp tmp1, tmp2;

    tmp1 = my_Fp3::non_residue * x4;
    tmp2 = my_Fp3::non_residue * x5;

    t0 = x0 * z0 + tmp1 * z4 + tmp2 * z3;
    t1 = x0 * z1 + tmp1 * z5 + tmp2 * z4;
    t2 = x0 * z2 + x4 * z3 + tmp2 * z5;
    t3 = x0 * z3 + tmp1 * z2 + tmp2 * z1;
    t4 = x0 * z4 + x4 * z0 + tmp2 * z2;
    t5 = x0 * z5 + x4 * z1 + x5 * z0;

    return Fp6_2over3_model<n, modulus>(my_Fp3(t0, t1, t2), my_Fp3(t3, t4, t5));
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::mul_by_2345(
    const Fp6_2over3_model<n, modulus> &other) const
{
    // Devegili OhEig Scott Dahab --- Multiplication and Squaring on
    // Pairing-Friendly Fields.pdf; Section 3 (Karatsuba)
    assert(other.coeffs[0].coeffs[0].is_zero());
    assert(other.coeffs[0].coeffs[1].is_zero());

    const my_Fp3 &B = other.coeffs[1], &A = other.coeffs[0],
                 &b = this->coeffs[1], &a = this->coeffs[0];
    const my_Fp3 aA = my_Fp3(
        a.coeffs[1] * A.coeffs[2] * non_residue,
        a.coeffs[2] * A.coeffs[2] * non_residue,
        a.coeffs[0] * A.coeffs[2]);
    const my_Fp3 bB = b * B;
    const my_Fp3 beta_bB = Fp6_2over3_model<n, modulus>::mul_by_non_residue(bB);

    return Fp6_2over3_model<n, modulus>(
        aA + beta_bB, (a + b) * (A + B) - aA - bB);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::operator-() const
{
    return Fp6_2over3_model<n, modulus>(-this->coeffs[0], -this->coeffs[1]);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::squared() const
{
    // Devegili OhEig Scott Dahab --- Multiplication and Squaring on
    // Pairing-Friendly Fields.pdf; Section 3 (Complex)
    const my_Fp3 &b = this->coeffs[1], &a = this->coeffs[0];
    const my_Fp3 ab = a * b;

    return Fp6_2over3_model<n, modulus>(
        (a + b) * (a + Fp6_2over3_model<n, modulus>::mul_by_non_residue(b)) -
            ab - Fp6_2over3_model<n, modulus>::mul_by_non_residue(ab),
        ab + ab);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::inverse() const
{
    // From "High-Speed Software Implementation of the Optimal Ate Pairing over
    // Barreto-Naehrig Curves"; Algorithm 8

    const my_Fp3 &b = this->coeffs[1], &a = this->coeffs[0];
    const my_Fp3 t1 = b.squared();
    const my_Fp3 t0 =
        a.squared() - Fp6_2over3_model<n, modulus>::mul_by_non_residue(t1);
    const my_Fp3 new_t1 = t0.inverse();

    return Fp6_2over3_model<n, modulus>(a * new_t1, -(b * new_t1));
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::Frobenius_map(
    unsigned long power) const
{
    return Fp6_2over3_model<n, modulus>(
        coeffs[0].Frobenius_map(power),
        Frobenius_coeffs_c1[power % 6] * coeffs[1].Frobenius_map(power));
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::unitary_inverse()
    const
{
    return Fp6_2over3_model<n, modulus>(this->coeffs[0], -this->coeffs[1]);
}

template<mp_size_t n, const bigint<n> &modulus>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::cyclotomic_squared()
    const
{
    my_Fp2 a = my_Fp2(coeffs[0].coeffs[0], coeffs[1].coeffs[1]);
    // my_Fp a_a = c0.coeffs[0]; // a = Fp2([c0[0],c1[1]])
    // my_Fp a_b = c1.coeffs[1];

    my_Fp2 b = my_Fp2(coeffs[1].coeffs[0], coeffs[0].coeffs[2]);
    // my_Fp b_a = c1.coeffs[0]; // b = Fp2([c1[0],c0[2]])
    // my_Fp b_b = c0.coeffs[2];

    my_Fp2 c = my_Fp2(coeffs[0].coeffs[1], coeffs[1].coeffs[2]);
    // my_Fp c_a = c0.coeffs[1]; // c = Fp2([c0[1],c1[2]])
    // my_Fp c_b = c1.coeffs[2];

    my_Fp2 asq = a.squared();
    my_Fp2 bsq = b.squared();
    my_Fp2 csq = c.squared();

    // A = vector(3*a^2 - 2*Fp2([vector(a)[0],-vector(a)[1]]))
    // my_Fp A_a = my_Fp(3l) * asq_a - my_Fp(2l) * a_a;
    my_Fp A_a = asq.coeffs[0] - a.coeffs[0];
    A_a = A_a + A_a + asq.coeffs[0];
    // my_Fp A_b = my_Fp(3l) * asq_b + my_Fp(2l) * a_b;
    my_Fp A_b = asq.coeffs[1] + a.coeffs[1];
    A_b = A_b + A_b + asq.coeffs[1];

    // B = vector(3*Fp2([non_residue*c2[1],c2[0]]) +
    // 2*Fp2([vector(b)[0],-vector(b)[1]])) my_Fp B_a = my_Fp(3l) *
    // my_Fp3::non_residue * csq_b + my_Fp(2l) * b_a;
    my_Fp B_tmp = my_Fp3::non_residue * csq.coeffs[1];
    my_Fp B_a = B_tmp + b.coeffs[0];
    B_a = B_a + B_a + B_tmp;

    // my_Fp B_b = my_Fp(3l) * csq_a - my_Fp(2l) * b_b;
    my_Fp B_b = csq.coeffs[0] - b.coeffs[1];
    B_b = B_b + B_b + csq.coeffs[0];

    // C = vector(3*b^2 - 2*Fp2([vector(c)[0],-vector(c)[1]]))
    // my_Fp C_a = my_Fp(3l) * bsq_a - my_Fp(2l) * c_a;
    my_Fp C_a = bsq.coeffs[0] - c.coeffs[0];
    C_a = C_a + C_a + bsq.coeffs[0];
    // my_Fp C_b = my_Fp(3l) * bsq_b + my_Fp(2l) * c_b;
    my_Fp C_b = bsq.coeffs[1] + c.coeffs[1];
    C_b = C_b + C_b + bsq.coeffs[1];

    // e0 = Fp3([A[0],C[0],B[1]])
    // e1 = Fp3([B[0],A[1],C[1]])
    // fin = Fp6e([e0,e1])
    // return fin

    return Fp6_2over3_model<n, modulus>(
        my_Fp3(A_a, C_a, B_b), my_Fp3(B_a, A_b, C_b));
}

template<mp_size_t n, const bigint<n> &modulus>
template<mp_size_t m>
Fp6_2over3_model<n, modulus> Fp6_2over3_model<n, modulus>::cyclotomic_exp(
    const bigint<m> &exponent) const
{
    Fp6_2over3_model<n, modulus> res = Fp6_2over3_model<n, modulus>::one();
    Fp6_2over3_model<n, modulus> this_inverse = this->unitary_inverse();

    bool found_nonzero = false;
    std::vector<long> NAF = find_wnaf(1, exponent);

    for (long i = static_cast<long>(NAF.size() - 1); i >= 0; --i) {
        if (found_nonzero) {
            res = res.cyclotomic_squared();
        }

        if (NAF[i] != 0) {
            found_nonzero = true;

            if (NAF[i] > 0) {
                res = res * (*this);
            } else {
                res = res * this_inverse;
            }
        }
    }

    return res;
}

template<mp_size_t n, const bigint<n> &modulus>
std::ostream &operator<<(
    std::ostream &out, const Fp6_2over3_model<n, modulus> &el)
{
    out << el.coeffs[0] << OUTPUT_SEPARATOR << el.coeffs[1];
    return out;
}

template<mp_size_t n, const bigint<n> &modulus>
std::istream &operator>>(std::istream &in, Fp6_2over3_model<n, modulus> &el)
{
    in >> el.coeffs[0] >> el.coeffs[1];
    return in;
}

template<mp_size_t n, const bigint<n> &modulus, mp_size_t m>
Fp6_2over3_model<n, modulus> operator^(
    const Fp6_2over3_model<n, modulus> &self, const bigint<m> &exponent)
{
    return power<Fp6_2over3_model<n, modulus>, m>(self, exponent);
}

template<
    mp_size_t n,
    const bigint<n> &modulus,
    mp_size_t m,
    const bigint<m> &exp_modulus>
Fp6_2over3_model<n, modulus> operator^(
    const Fp6_2over3_model<n, modulus> &self,
    const Fp_model<m, exp_modulus> &exponent)
{
    return self ^ (exponent.as_bigint());
}

} // namespace libff

#endif // FP6_2OVER3_TCC_
