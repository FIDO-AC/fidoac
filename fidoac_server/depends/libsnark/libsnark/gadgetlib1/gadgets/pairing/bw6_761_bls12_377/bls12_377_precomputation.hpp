/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_PRECOMPUTATION_HPP_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_PRECOMPUTATION_HPP_

namespace libsnark
{

template<typename ppT> class bls12_377_G1_precomputation
{
public:
    using FieldT = libff::Fr<ppT>;

    std::shared_ptr<pb_linear_combination<FieldT>> _Px;
    std::shared_ptr<pb_linear_combination<FieldT>> _Py;

    // Pointers _Px and _Py are assigned in the constructor of the
    // precompute_G1_gadget. Until that is called, no reference should be made
    // to these members.
    bls12_377_G1_precomputation();

    // Construct a populated G1_precomputation from a value. All terms are
    // created as constants, requiring no new gates in the circuit.
    bls12_377_G1_precomputation(
        protoboard<FieldT> &pb,
        const libff::G1<other_curve<ppT>> &P_val,
        const std::string &annotation_prefix);
};

/// Holds an element of G2 in homogeneous projective form. Used for
/// intermediate values of R in the miller loop.
template<typename ppT> class bls12_377_G2_proj
{
public:
    Fqe_variable<ppT> X;
    Fqe_variable<ppT> Y;
    Fqe_variable<ppT> Z;

    bls12_377_G2_proj(
        protoboard<libff::Fr<ppT>> &pb, const std::string &annotation_prefix);

    bls12_377_G2_proj(
        const Fqe_variable<ppT> &X_var,
        const Fqe_variable<ppT> &Y_var,
        const Fqe_variable<ppT> &Z_var);

    void generate_r1cs_witness(const libff::bls12_377_G2 &element);
};

/// Not a gadget - holds the variables for the Fq2 coefficients of the tangent
/// line at some R, used during the doubling step.
template<typename ppT> class bls12_377_ate_ell_coeffs
{
public:
    using FqT = libff::Fq<other_curve<ppT>>;

    Fqe_variable<ppT> ell_0;
    Fqe_variable<ppT> ell_vw;
    Fqe_variable<ppT> ell_vv;

    bls12_377_ate_ell_coeffs(
        protoboard<FqT> &pb, const std::string &annotation_prefix);

    // Create from constants
    bls12_377_ate_ell_coeffs(
        protoboard<FqT> &pb,
        const libff::Fqe<other_curve<ppT>> ell_0_val,
        const libff::Fqe<other_curve<ppT>> ell_vw_val,
        const libff::Fqe<other_curve<ppT>> ell_vv_val,
        const std::string &annotation_prefix);
};

template<typename ppT> class bls12_377_G2_precomputation
{
public:
    using FieldT = libff::Fr<ppT>;

    std::vector<std::shared_ptr<bls12_377_ate_ell_coeffs<ppT>>> _coeffs;

    bls12_377_G2_precomputation();

    // Construct a populated G2_precomputation from a value. All terms are
    // created as constants, requiring no new gates in the circuit.
    bls12_377_G2_precomputation(
        protoboard<FieldT> &pb,
        const libff::G2<other_curve<ppT>> &Q_val,
        const std::string &annotation_prefix);
};

template<typename ppT>
class bls12_377_G1_precompute_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FieldT = libff::Fr<ppT>;

    std::shared_ptr<pb_linear_combination<FieldT>> _Px;
    std::shared_ptr<pb_linear_combination<FieldT>> _Py;

    bls12_377_G1_precompute_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const G1_variable<ppT> &P,
        bls12_377_G1_precomputation<ppT> &P_prec,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Gadget that relates some "current" bls12_377_G2_proj value in_R with the
/// result of the doubling step, that is some bls12_377_G2_proj out_R and the
/// bls12_377_ate_ell_coeffs holding the coefficients of the tangent at in_R.
/// Note that the output variables are allocated by this gadget.
template<typename ppT>
class bls12_377_ate_dbl_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fq<other_curve<ppT>> FqT;
    typedef libff::Fqe<other_curve<ppT>> FqeT;

    bls12_377_G2_proj<ppT> _in_R;
    bls12_377_G2_proj<ppT> _out_R;
    bls12_377_ate_ell_coeffs<ppT> _out_coeffs;

    // TODO: Many of these intermediate Fqe_variables are only for clarity and
    // replicate the references held by other gadgets (e.g. `A` refers to the
    // same variable as `check_A.result`. Do an optimization pass and remove
    // some of the redundancy.

    // A = R.X * R.Y / 2
    Fqe_mul_gadget<ppT> _compute_A;

    // B = R.Y^2
    Fqe_sqr_gadget<ppT> _compute_B;

    // C = R.Z^2
    Fqe_sqr_gadget<ppT> _compute_C;

    // D = 3 * C
    // E = b' * D
    // F = 3 * E
    // G = (B + F) / 2

    // ell_vw = -H
    //   where
    //     H = (Y + 2) ^ 2 - (B + C)
    // ell_vw = (B+C) - (Y+2)^2
    // <=> (Y+2)^2 [H] = ell_vw - B - C
    Fqe_sqr_gadget<ppT> _compute_Y_plus_Z_squared;

    // I = E - B

    // ell_vv = 3 * J
    //   where
    //     J = Rx^2
    // ell_vv = 3 * Rx^2
    // <=> Rx^2 [J] = ell_vv * 3^{-1}
    Fqe_sqr_gadget<ppT> _compute_J; // Rx^2 == J

    // out_R.X = A * (B - F)
    Fqe_mul_gadget<ppT> _check_out_Rx;

    // out_R.Y = G^2 - 3 * E^2
    // <=> G^2 = outRy + 3*E^2
    Fqe_sqr_gadget<ppT> _compute_E_squared;
    Fqe_sqr_gadget<ppT> _compute_G_squared;

    // out_R.Z = B * H
    Fqe_mul_gadget<ppT> _check_out_Rz;

    bls12_377_ate_dbl_gadget(
        protoboard<FqT> &pb,
        const bls12_377_G2_proj<ppT> &R,
        const bls12_377_G2_proj<ppT> &out_R,
        const bls12_377_ate_ell_coeffs<ppT> &coeffs,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

template<typename ppT>
class bls12_377_ate_add_gadget : public gadget<libff::Fr<ppT>>
{
public:
    typedef libff::Fq<other_curve<ppT>> FqT;
    typedef libff::Fqe<other_curve<ppT>> FqeT;

    Fqe_variable<ppT> _Q_X;
    Fqe_variable<ppT> _Q_Y;
    bls12_377_G2_proj<ppT> _in_R;
    bls12_377_G2_proj<ppT> _out_R;
    bls12_377_ate_ell_coeffs<ppT> _out_coeffs;

    // ell_vv = -theta
    //   where
    //     theta = R.Y - A
    //     A = Q_Y * R.Z;
    // <=> A = Q_Y * R.Z = ell_vv + Ry
    Fqe_mul_gadget<ppT> _compute_A;
    // ell_vw = lambda
    //   where
    //     lambda = R.X - B
    //     B = Q_X * R.Z
    // <=> B = Q_X * R.Z = R.X - ell_vw
    Fqe_mul_gadget<ppT> _compute_B;
    // theta = R.Y - A = -ell_vv
    // Fqe_variable<ppT> _theta
    // lambda = R.X - B = ell_vw
    // Fqe_variable<ppT> lambda
    // C = theta.squared() = ell_vv^2
    Fqe_sqr_gadget<ppT> _compute_C;
    // D = lambda.squared() = ell_vw^2
    Fqe_sqr_gadget<ppT> _compute_D;
    // E = lambda * D;
    Fqe_mul_gadget<ppT> _compute_E;
    // F = R.Z * C;
    Fqe_mul_gadget<ppT> _compute_F;
    // G = R.X * D;
    Fqe_mul_gadget<ppT> _compute_G;
    // H = E + F - (G + G);
    Fqe_variable<ppT> _H;
    // I = R.Y * E;
    Fqe_mul_gadget<ppT> _compute_I;
    // out_coeffs.ell_0 = xi * J
    //   where
    //     J = theta * Q_X - lambda * Q_Y
    // <=> lambda * Q_Y = theta * Q_X - ell_0 * xi^{-1}
    Fqe_mul_gadget<ppT> _compute_theta_times_Qx;
    Fqe_mul_gadget<ppT> _compute_lambda_times_Qy;

    // out_R.X = lambda * H = ell_vw * H
    Fqe_mul_gadget<ppT> _check_out_Rx;
    // out_R.Y = theta * (G - H) - I = -ell_vv * (G-H) - I
    // <=> ell_vv * (H-G) = out_R.Y + I
    Fqe_mul_gadget<ppT> _check_out_Ry;
    // out_R.Z = Z1 * E;
    Fqe_mul_gadget<ppT> _check_out_Rz;

    bls12_377_ate_add_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const Fqe_variable<ppT> &Q_X,
        const Fqe_variable<ppT> &Q_Y,
        const bls12_377_G2_proj<ppT> &R,
        const bls12_377_G2_proj<ppT> &out_R,
        const bls12_377_ate_ell_coeffs<ppT> &coeffs,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

/// Holds the relationship between an (affine) pairing parameter Q in G2, and
/// the precomputed double and add gadgets.
template<typename ppT>
class bls12_377_G2_precompute_gadget : public gadget<libff::Fr<ppT>>
{
public:
    using FqeT = libff::Fqe<other_curve<ppT>>;

    bls12_377_G2_proj<ppT> _R0;
    std::vector<std::shared_ptr<bls12_377_ate_dbl_gadget<ppT>>> _ate_dbls;
    std::vector<std::shared_ptr<bls12_377_ate_add_gadget<ppT>>> _ate_adds;

    bls12_377_G2_precompute_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const G2_variable<ppT> &Q,
        bls12_377_G2_precomputation<ppT> &Q_prec,
        const std::string &annotation_prefix);

    void generate_r1cs_constraints();
    void generate_r1cs_witness();
};

} // namespace libsnark

#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_precomputation.tcc"

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_PAIRING_HPP_
