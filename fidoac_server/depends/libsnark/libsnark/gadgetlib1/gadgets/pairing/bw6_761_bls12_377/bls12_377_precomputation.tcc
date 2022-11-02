/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_PRECOMPUTATION_TCC_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_PRECOMPUTATION_TCC_

namespace libsnark
{

// Iterate through bits of loop_count, skipping the highest order bit.

class bls12_377_miller_loop_bits
{
public:
    inline bls12_377_miller_loop_bits()
    {
        // TODO: should not need to do this dynamically
        ssize_t start_i = libff::bls12_377_ate_loop_count.max_bits();
        while (!libff::bls12_377_ate_loop_count.test_bit(start_i--)) {
        }
        _i = start_i + 1;
    }

    inline bool next()
    {
        if (_i > 0) {
            --_i;
            return true;
        }

        return false;
    }

    inline bool current() const
    {
        return libff::bls12_377_ate_loop_count.test_bit(_i);
    }

    inline size_t index() const { return (size_t)_i; }

    inline bool last() const { return 0 == index(); }

private:
    ssize_t _i;
};

// bls12_377_G1_precomputation methods

template<typename ppT>
bls12_377_G1_precomputation<ppT>::bls12_377_G1_precomputation() : _Px(), _Py()
{
}

template<typename ppT>
bls12_377_G1_precomputation<ppT>::bls12_377_G1_precomputation(
    protoboard<FieldT> &pb,
    const libff::G1<other_curve<ppT>> &P_val,
    const std::string & /* annotation_prefix */)
    : _Px(new pb_linear_combination<FieldT>())
    , _Py(new pb_linear_combination<FieldT>())
{
    libff::G1<other_curve<ppT>> P_affine = P_val;
    P_affine.to_affine_coordinates();
    _Px->assign(pb, P_affine.X);
    _Py->assign(pb, P_affine.Y);
    _Px->evaluate(pb);
    _Py->evaluate(pb);
}

// bls12_377_G2_proj methods

template<typename ppT>
bls12_377_G2_proj<ppT>::bls12_377_G2_proj(
    protoboard<libff::Fr<ppT>> &pb, const std::string &annotation_prefix)
    : X(pb, FMT(annotation_prefix, " X"))
    , Y(pb, FMT(annotation_prefix, " Y"))
    , Z(pb, FMT(annotation_prefix, " Z"))
{
}

template<typename ppT>
bls12_377_G2_proj<ppT>::bls12_377_G2_proj(
    const Fqe_variable<ppT> &X_var,
    const Fqe_variable<ppT> &Y_var,
    const Fqe_variable<ppT> &Z_var)
    : X(X_var), Y(Y_var), Z(Z_var)
{
}

template<typename ppT>
void bls12_377_G2_proj<ppT>::generate_r1cs_witness(
    const libff::bls12_377_G2 &element)
{
    X.generate_r1cs_witness(element.X);
    Y.generate_r1cs_witness(element.Y);
    Z.generate_r1cs_witness(element.Z);
}

// bls12_377_ate_ell_coeffs methods

template<typename ppT>
bls12_377_ate_ell_coeffs<ppT>::bls12_377_ate_ell_coeffs(
    protoboard<FqT> &pb, const std::string &annotation_prefix)
    : ell_0(pb, FMT(annotation_prefix, " ell_0"))
    , ell_vw(pb, FMT(annotation_prefix, " ell_vw"))
    , ell_vv(pb, FMT(annotation_prefix, " ell_vv"))
{
}

template<typename ppT>
bls12_377_ate_ell_coeffs<ppT>::bls12_377_ate_ell_coeffs(
    protoboard<FqT> &pb,
    const libff::Fqe<other_curve<ppT>> ell_0_val,
    const libff::Fqe<other_curve<ppT>> ell_vw_val,
    const libff::Fqe<other_curve<ppT>> ell_vv_val,
    const std::string &annotation_prefix)
    : ell_0(pb, ell_0_val, FMT(annotation_prefix, " ell_0"))
    , ell_vw(pb, ell_vw_val, FMT(annotation_prefix, " ell_vw"))
    , ell_vv(pb, ell_vv_val, FMT(annotation_prefix, " ell_vv"))
{
}

// bls12_377_G2_precomputation methods

template<typename ppT>
bls12_377_G2_precomputation<ppT>::bls12_377_G2_precomputation()
{
}

template<typename ppT>
bls12_377_G2_precomputation<ppT>::bls12_377_G2_precomputation(
    protoboard<FieldT> &pb,
    const libff::G2<other_curve<ppT>> &Q_val,
    const std::string &annotation_prefix)
{
    const libff::G2_precomp<other_curve<ppT>> Q_prec =
        other_curve<ppT>::precompute_G2(Q_val);
    const size_t num_coeffs = Q_prec.coeffs.size();
    _coeffs.reserve(num_coeffs);
    for (size_t i = 0; i < num_coeffs; ++i) {
        const libff::bls12_377_ate_ell_coeffs &c = Q_prec.coeffs[i];
        _coeffs.emplace_back(new bls12_377_ate_ell_coeffs<ppT>(
            pb,
            c.ell_0,
            c.ell_VW,
            c.ell_VV,
            FMT(annotation_prefix, " coeffs[%zu]", i)));
    }

    assert(num_coeffs == _coeffs.size());
}

// bls12_377_G1_precompute_gadget methods

template<typename ppT>
bls12_377_G1_precompute_gadget<ppT>::bls12_377_G1_precompute_gadget(
    protoboard<libff::Fr<ppT>> &pb,
    const G1_variable<ppT> &P,
    bls12_377_G1_precomputation<ppT> &P_prec,
    const std::string &annotation_prefix)
    : gadget<libff::Fr<ppT>>(pb, annotation_prefix)
    , _Px(new pb_linear_combination<libff::Fr<ppT>>())
    , _Py(new pb_linear_combination<libff::Fr<ppT>>())
{
    // Ensure that we don't overwrite an existing precomputation.
    assert(!P_prec._Px);
    assert(!P_prec._Py);
    _Px->assign(pb, P.X);
    _Py->assign(pb, P.Y);
    P_prec._Px = _Px;
    P_prec._Py = _Py;
}

template<typename ppT>
void bls12_377_G1_precompute_gadget<ppT>::generate_r1cs_constraints()
{
}

template<typename ppT>
void bls12_377_G1_precompute_gadget<ppT>::generate_r1cs_witness()
{
    _Px->evaluate(this->pb);
    _Py->evaluate(this->pb);
}

// bls12_377_ate_dbl_gadget methods

template<typename ppT>
bls12_377_ate_dbl_gadget<ppT>::bls12_377_ate_dbl_gadget(
    protoboard<FqT> &pb,
    const bls12_377_G2_proj<ppT> &R,
    const bls12_377_G2_proj<ppT> &out_R,
    const bls12_377_ate_ell_coeffs<ppT> &coeffs,
    const std::string &annotation_prefix)
    : gadget<libff::Fr<ppT>>(pb, annotation_prefix)
    , _in_R(R)
    , _out_R(out_R)
    , _out_coeffs(coeffs)

    // A = Rx * Ry / 2
    , _compute_A(
          pb,
          _in_R.X,
          _in_R.Y * FqT(2).inverse(),
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " A")),
          FMT(annotation_prefix, " _compute_A"))

    // B = Ry^2
    , _compute_B(
          pb,
          _in_R.Y,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " B")),
          FMT(annotation_prefix, " _compute_B"))

    // ell_0 = xi * I
    //   where
    //     C = Rz^2
    //     D = 3 * C
    //     E = b' * D
    //     I = (E - B)
    // ell_0 = xi * I
    //       = xi * (E - B)
    //       = xi * (3*b'*C  - B)
    //       = xi * (3*b'*Rz^2 - Ry^2)
    // <=> Rz^2 [C] = (ell_0 + xi.Ry^2) / 3*b'*xi
    //              = (3*b')^{-1}(ell_0*xi^{-1} + B)
    , _compute_C(
          pb,
          _in_R.Z,
          (_out_coeffs.ell_0 * libff::bls12_377_twist.inverse() +
           _compute_B.result) *
              (FqT(3) * libff::bls12_377_twist_coeff_b).inverse(),
          FMT(annotation_prefix, " _compute_C"))

    // D = 3 * C
    // E = b' * D
    // F = 3 * E

    // H = (Y + Z) ^ 2 - (B + C)
    // ell_vw = -H
    //        = (B+C) - (Y+2)^2
    // <=> (Y+2)^2 [H] = B + C - ell_vw
    , _compute_Y_plus_Z_squared(
          pb,
          _in_R.Y + _in_R.Z,
          _compute_B.result + _compute_C.result - _out_coeffs.ell_vw,
          FMT(annotation_prefix, " _compute_Y_plus_Z_squared"))

    // I = (E - B)
    // J = Rx^2
    // ell_vv = 3 * J
    //        = 3 * Rx^2
    // <=> Rx^2 [J] = ell_vv * 3^{-1}
    , _compute_J(
          pb,
          _in_R.X,
          _out_coeffs.ell_vv * FqT(3).inverse(),
          FMT(annotation_prefix, " _compute_J"))

    // outRx = A * (B-F)
    , _check_out_Rx(
          pb,
          _compute_A.result,
          _compute_B.result - _compute_C.result *
                                  libff::bls12_377_twist_coeff_b *
                                  FqT(9), // B-F
          _out_R.X,
          FMT(annotation_prefix, " _check_out_Rx"))

    // outRy = G^2 - 3E^2
    //   where  G = (B + F) / 2
    // <=> G^2 = outRy + 3 * E^2
    , _compute_E_squared(
          pb,
          _compute_C.result * libff::bls12_377_twist_coeff_b * FqT(3), // E
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, "E_squared")),
          FMT(annotation_prefix, " _compute_E_squared"))
    , _compute_G_squared(
          pb,
          (_compute_B.result +
           _compute_C.result * libff::bls12_377_twist_coeff_b * FqT(9)) *
              FqT(2).inverse(), // G = (B+F)/2
          _out_R.Y + _compute_E_squared.result + _compute_E_squared.result +
              _compute_E_squared.result,
          FMT(annotation_prefix, " _compute_G_squared"))

    // outRz = B * H
    //   where
    //     H = (Y + Z)^2 - (B + C)
    , _check_out_Rz(
          pb,
          _compute_B.result,
          _compute_Y_plus_Z_squared.result - _compute_B.result -
              _compute_C.result,
          _out_R.Z,
          FMT(annotation_prefix, " _check_out_Rz"))
{
}

template<typename ppT>
void bls12_377_ate_dbl_gadget<ppT>::generate_r1cs_constraints()
{
    _compute_A.generate_r1cs_constraints();
    _compute_B.generate_r1cs_constraints();
    _compute_C.generate_r1cs_constraints();
    _compute_Y_plus_Z_squared.generate_r1cs_constraints();
    _compute_J.generate_r1cs_constraints();
    _check_out_Rx.generate_r1cs_constraints();
    _compute_E_squared.generate_r1cs_constraints();
    _compute_G_squared.generate_r1cs_constraints();
    _check_out_Rz.generate_r1cs_constraints();
}

template<typename ppT>
void bls12_377_ate_dbl_gadget<ppT>::generate_r1cs_witness()
{
    const FqeT Rx = _in_R.X.get_element();
    const FqeT Ry = _in_R.Y.get_element();
    const FqeT Rz = _in_R.Z.get_element();

    // A = Rx * Ry / 2
    _compute_A.B.evaluate();
    _compute_A.generate_r1cs_witness();

    // B = Ry^2
    _compute_B.generate_r1cs_witness();

    // ell_0 = xi * I
    //   where
    //     C = Rz^2
    //     D = 3 * C
    //     E = b' * D
    //     I = (E - B)
    // <=> Rz^2 [C] = (ell_0.xi^{-1} + Ry^2) * (3*b')^{-1}
    const FqeT B = _compute_B.result.get_element();
    const FqeT C = Rz * Rz;
    const FqeT D = FqT(3) * C;
    const FqeT E = libff::bls12_377_twist_coeff_b * D;
    const FqeT I = E - B;
    _out_coeffs.ell_0.generate_r1cs_witness(libff::bls12_377_twist * I);
    _compute_C.result.evaluate();
    _compute_C.generate_r1cs_witness();
    assert(C == _compute_C.result.get_element());
    assert(
        (_out_coeffs.ell_0.get_element() * libff::bls12_377_twist.inverse() +
         B) *
            (FqT(3) * libff::bls12_377_twist_coeff_b).inverse() ==
        _compute_C.result.get_element());

    // G = (B + F) / 2
    // ell_vw = -H
    //   where
    //     H = (Y + 2) ^ 2 - (B + C)
    // ell_vw = (B+C) - (Y+2)^2
    // <=> (Y+2)^2 [H] = ell_vw - B - C
    const FqeT Ry_plus_Rz_squared = (Ry + Rz) * (Ry + Rz);
    _out_coeffs.ell_vw.generate_r1cs_witness(B + C - Ry_plus_Rz_squared);
    _compute_Y_plus_Z_squared.A.evaluate();
    _compute_Y_plus_Z_squared.result.evaluate();
    _compute_Y_plus_Z_squared.generate_r1cs_witness();
    assert(
        _compute_Y_plus_Z_squared.result.get_element() == Ry_plus_Rz_squared);

    // I = E - B

    // ell_vv = 3 * J
    // J = Rx^2
    const FqeT J = Rx * Rx;
    _out_coeffs.ell_vv.generate_r1cs_witness(FqT(3) * J);
    _compute_J.result.evaluate();
    _compute_J.generate_r1cs_witness();

    // outRx = A * (B - F)
    _check_out_Rx.B.evaluate();
    _check_out_Rx.result.evaluate();
    _check_out_Rx.generate_r1cs_witness();

    // outRy = G^2 - 3E^2
    //   where  G = (B + F) / 2
    // <=> G^2 = outRy + 3 * E^2
    _compute_E_squared.A.evaluate();
    _compute_E_squared.generate_r1cs_witness();
    const FqeT E_squared = _compute_E_squared.result.get_element();
    const FqeT F = FqT(3) * E;
    const FqeT G = FqT(2).inverse() * (B + F);
    const FqeT G_squared = G * G;
    _out_R.Y.generate_r1cs_witness(G_squared - FqT(3) * E_squared);
    _compute_G_squared.A.evaluate();
    _compute_G_squared.result.evaluate();
    _compute_G_squared.generate_r1cs_witness();

    // out_Rz = B * H (assigned by check_outRz)
    _check_out_Rz.B.evaluate();
    assert(B == _check_out_Rz.A.get_element());
    assert(Ry_plus_Rz_squared - B - C == _check_out_Rz.B.get_element());
    _check_out_Rz.generate_r1cs_witness();
    assert(
        B * (Ry_plus_Rz_squared - B - C) == _check_out_Rz.result.get_element());
}

// bls12_377_ate_add_gadget methods

template<typename ppT>
bls12_377_ate_add_gadget<ppT>::bls12_377_ate_add_gadget(
    protoboard<libff::Fr<ppT>> &pb,
    const Fqe_variable<ppT> &Q_X,
    const Fqe_variable<ppT> &Q_Y,
    const bls12_377_G2_proj<ppT> &in_R,
    const bls12_377_G2_proj<ppT> &out_R,
    const bls12_377_ate_ell_coeffs<ppT> &out_coeffs,
    const std::string &annotation_prefix)
    : gadget<libff::Fr<ppT>>(pb, annotation_prefix)
    , _Q_X(Q_X)
    , _Q_Y(Q_Y)
    , _in_R(in_R)
    , _out_R(out_R)
    , _out_coeffs(out_coeffs)
    // ell_vv = -theta
    //   where
    //     theta = Ry - A
    //     A = Qy * Rz;
    // <=> A = Qy * Rz = ell_vv + Ry
    , _compute_A(
          pb,
          _Q_Y,
          _in_R.Z,
          _out_coeffs.ell_vv + _in_R.Y,
          FMT(annotation_prefix, " _compute_A"))
    // ell_vw = lambda
    //   where
    //     lambda = Rx - B
    //     B = Qx * Rz
    // <=> B = Qx * Rz = Rx - ell_vw
    , _compute_B(
          pb,
          _Q_X,
          _in_R.Z,
          _in_R.X - _out_coeffs.ell_vw,
          FMT(annotation_prefix, " _compute_B"))
    // theta = Ry - A;
    // , theta(in_R.Y + (A * -libff::Fr<ppT>::one()))
    // lambda = Rx - B;
    // , lambda(in_R.X + (B * -libff::Fr<ppT>::one()))
    // C = theta.squared() = ell_vv^2
    , _compute_C(
          pb,
          _out_coeffs.ell_vv,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " C")),
          FMT(annotation_prefix, " _compute_C"))
    // D = lambda.squared() = ell_vw^2
    , _compute_D(
          pb,
          _out_coeffs.ell_vw,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " D")),
          FMT(annotation_prefix, " _compute_D"))
    // E = lambda * D = D * ell_vw;
    , _compute_E(
          pb,
          _compute_D.result,
          _out_coeffs.ell_vw,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " E")),
          FMT(annotation_prefix, " _compute_E"))
    // F = Rz * C;
    , _compute_F(
          pb,
          _in_R.Z,
          _compute_C.result,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " F")),
          FMT(annotation_prefix, " _compute_F"))
    // G = Rx * D;
    , _compute_G(
          pb,
          _in_R.X,
          _compute_D.result,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " G")),
          FMT(annotation_prefix, " _compute_G"))
    // H = E + F - (G + G);
    , _H(_compute_E.result + _compute_F.result - _compute_G.result -
         _compute_G.result)
    // I = Ry * E;
    , _compute_I(
          pb,
          _in_R.Y,
          _compute_E.result,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " I")),
          FMT(annotation_prefix, " _compute_I"))
    // out_coeffs.ell_0 = xi * J
    //   where J = theta * Qx - lambda * Qy
    // <=> lambda * Qy = theta * Qx - ell_0 * xi^{-1}
    , _compute_theta_times_Qx(
          pb,
          -_out_coeffs.ell_vv,
          _Q_X,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " theta_times_Qx")),
          FMT(annotation_prefix, " _compute_theta_times_Qx"))
    , _compute_lambda_times_Qy(
          pb,
          _out_coeffs.ell_vw,
          _Q_Y,
          _compute_theta_times_Qx.result -
              (_out_coeffs.ell_0 * libff::bls12_377_twist.inverse()),
          FMT(annotation_prefix, " _compute_lambda_times_Qy"))
    // out_Rx = lambda * H = ell_vw * H
    , _check_out_Rx(
          pb,
          _out_coeffs.ell_vw,
          _H,
          _out_R.X,
          FMT(annotation_prefix, " _check_out_Rx"))
    // out_Ry = theta * (G - H) - I = -ell_vv * (G-H) - I
    // <=> ell_vv * (H-G) = out_Ry + I
    , _check_out_Ry(
          pb,
          _out_coeffs.ell_vv,
          _H - _compute_G.result,
          _out_R.Y + _compute_I.result,
          FMT(annotation_prefix, " _check_out_Ry"))
    // out_Rz = Z1 * E;
    , _check_out_Rz(
          pb,
          _in_R.Z,
          _compute_E.result,
          _out_R.Z,
          FMT(annotation_prefix, " _check_out_Rz"))
{
}

template<typename ppT>
void bls12_377_ate_add_gadget<ppT>::generate_r1cs_constraints()
{
    _compute_A.generate_r1cs_constraints();
    _compute_B.generate_r1cs_constraints();
    _compute_C.generate_r1cs_constraints();
    _compute_D.generate_r1cs_constraints();
    _compute_E.generate_r1cs_constraints();
    _compute_F.generate_r1cs_constraints();
    _compute_G.generate_r1cs_constraints();
    _compute_theta_times_Qx.generate_r1cs_constraints();
    _compute_lambda_times_Qy.generate_r1cs_constraints();
    _check_out_Rx.generate_r1cs_constraints();
    _check_out_Ry.generate_r1cs_constraints();
    _check_out_Rz.generate_r1cs_constraints();
}

template<typename ppT>
void bls12_377_ate_add_gadget<ppT>::generate_r1cs_witness()
{
    const FqeT Qx = _Q_X.get_element();
    const FqeT Qy = _Q_Y.get_element();
    const FqeT Rx = _in_R.X.get_element();
    const FqeT Ry = _in_R.Y.get_element();
    const FqeT Rz = _in_R.Z.get_element();

    // ell_vv = -theta
    //   where
    //     theta = Ry - A
    //     A = Qy * Rz;
    // <=> A = Qy * Rz = ell_vv + Ry
    const FqeT A = Qy * Rz;
    const FqeT theta = Ry - A;
    _out_coeffs.ell_vv.generate_r1cs_witness(-theta);
    _compute_A.result.evaluate();
    _compute_A.generate_r1cs_witness();

    // ell_vw = lambda
    //   where
    //     lambda = Rx - B
    //     B = Qx * Rz
    // <=> B = Qx * Rz = Rx - ell_vw
    const FqeT B = Qx * Rz;
    const FqeT lambda = Rx - B;
    _out_coeffs.ell_vw.generate_r1cs_witness(lambda);
    _compute_B.result.evaluate();
    _compute_B.generate_r1cs_witness();
    // C = theta.squared() = ell_vv^2
    _compute_C.generate_r1cs_witness();
    // D = lambda.squared() = ell_vw^2
    _compute_D.generate_r1cs_witness();
    // E = lambda * D = D * ell_vw;
    _compute_E.generate_r1cs_witness();
    // F = Rz * C
    _compute_F.generate_r1cs_witness();
    // G = Rx * D;
    _compute_G.generate_r1cs_witness();
    // H = E + F - (G + G);
    _H.evaluate();
    // I = Ry * E
    _compute_I.generate_r1cs_witness();
    // out_coeffs.ell_0 = xi * J
    //   where J = theta * Qx - lambda * Qy
    // <=> lambda * Qy = theta * Qx - ell_0 * xi^{-1}
    _compute_theta_times_Qx.A.evaluate();
    _compute_theta_times_Qx.generate_r1cs_witness();
    const FqeT theta_times_Qx = _compute_theta_times_Qx.result.get_element();
    const FqeT lambda_times_Qy = lambda * Qy;
    _out_coeffs.ell_0.generate_r1cs_witness(
        libff::bls12_377_twist * (theta_times_Qx - lambda_times_Qy));
    _compute_lambda_times_Qy.result.evaluate();
    _compute_lambda_times_Qy.generate_r1cs_witness();
    // out_Rx = lambda * H = ell_vw * H
    _check_out_Rx.generate_r1cs_witness();
    // out_Ry = theta * (G - H) - I = -ell_vv * (G-H) - I
    // <=> ell_vv * (H-G) = out_Ry + I
    const FqeT G = _compute_G.result.get_element();
    const FqeT H = _H.get_element();
    const FqeT I = _compute_I.result.get_element();
    _out_R.Y.generate_r1cs_witness(theta * (G - H) - I);
    _check_out_Ry.B.evaluate();
    _check_out_Ry.result.evaluate();
    _check_out_Ry.generate_r1cs_witness();
    // out_Rz = Z1 * E
    _check_out_Rz.generate_r1cs_witness();
}

// bls12_377_G2_precompute methods

template<typename ppT>
bls12_377_G2_precompute_gadget<ppT>::bls12_377_G2_precompute_gadget(
    protoboard<libff::Fr<ppT>> &pb,
    const G2_variable<ppT> &Q,
    bls12_377_G2_precomputation<ppT> &Q_prec,
    const std::string &annotation_prefix)
    : gadget<libff::Fr<ppT>>(pb, annotation_prefix)
    , _R0(*Q.X, *Q.Y, Fqe_variable<ppT>(pb, FqeT::one(), "Fqe(1)"))
{
    // Track the R variable at each step. Initially it is _R0;
    const bls12_377_G2_proj<ppT> *currentR = &_R0;
    size_t num_dbl = 0;
    size_t num_add = 0;
    size_t num_Rs = 0;
    std::vector<std::shared_ptr<bls12_377_G2_proj<ppT>>> R;

    // Iterate through bits of loop_count
    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        R.push_back(
            std::shared_ptr<bls12_377_G2_proj<ppT>>(new bls12_377_G2_proj<ppT>(
                pb, FMT(annotation_prefix, " R%zu", num_Rs++))));
        Q_prec._coeffs.push_back(std::shared_ptr<bls12_377_ate_ell_coeffs<ppT>>(
            new bls12_377_ate_ell_coeffs<ppT>(
                pb, FMT(annotation_prefix, " Q_prec_dbl_%zu", num_dbl++))));
        _ate_dbls.push_back(std::shared_ptr<bls12_377_ate_dbl_gadget<ppT>>(
            new bls12_377_ate_dbl_gadget<ppT>(
                pb,
                *currentR,
                *R.back(),
                *Q_prec._coeffs.back(),
                FMT(annotation_prefix, " dbls[%zu]", bits.index()))));
        currentR = &(*R.back());

        if (bits.current()) {
            R.push_back(std::shared_ptr<bls12_377_G2_proj<ppT>>(
                new bls12_377_G2_proj<ppT>(
                    pb, FMT(annotation_prefix, " R%zu", num_Rs++))));
            Q_prec._coeffs.push_back(
                std::shared_ptr<bls12_377_ate_ell_coeffs<ppT>>(
                    new bls12_377_ate_ell_coeffs<ppT>(
                        pb,
                        FMT(annotation_prefix, " Q_prec_add_%zu", num_add++))));
            _ate_adds.push_back(std::shared_ptr<bls12_377_ate_add_gadget<ppT>>(
                new bls12_377_ate_add_gadget<ppT>(
                    pb,
                    *Q.X,
                    *Q.Y,
                    *currentR,
                    *R.back(),
                    *Q_prec._coeffs.back(),
                    FMT(annotation_prefix, " adds[%zu]", bits.index()))));
            currentR = &(*R.back());
        }
    }
}

template<typename ppT>
void bls12_377_G2_precompute_gadget<ppT>::generate_r1cs_constraints()
{
    size_t dbl_idx = 0;
    size_t add_idx = 0;

    // TODO: There should be no need to loop through the bits of loop_count
    // when generating the constraints (all variables have been allocated, so
    // the order of generation is not important). For now we do
    // this to keep a
    // consistent loop in all methods.

    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        _ate_dbls[dbl_idx++]->generate_r1cs_constraints();
        if (bits.current()) {
            _ate_adds[add_idx++]->generate_r1cs_constraints();
        }
    }
}

template<typename ppT>
void bls12_377_G2_precompute_gadget<ppT>::generate_r1cs_witness()
{
    size_t dbl_idx = 0;
    size_t add_idx = 0;
    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        _ate_dbls[dbl_idx++]->generate_r1cs_witness();
        if (bits.current()) {
            _ate_adds[add_idx++]->generate_r1cs_witness();
        }
    }
}

} // namespace libsnark

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_PRECOMPUTATION_TCC_
