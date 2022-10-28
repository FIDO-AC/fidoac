/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_FINAL_EXPONENTIATION_TCC_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_FINAL_EXPONENTIATION_TCC_

#include "libsnark/gadgetlib1/gadgets/basic_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_final_exponentiation.hpp"

namespace libsnark
{

// bls12_377_final_exp_first_part_gadget methods

template<typename ppT>
bls12_377_final_exp_first_part_gadget<ppT>::
    bls12_377_final_exp_first_part_gadget(
        protoboard<FieldT> &pb,
        const Fp12_2over3over2_variable<FqkT> &in,
        const Fp12_2over3over2_variable<FqkT> &result,
        const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _result(result)
    , _compute_B(
          pb,
          in,
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " B")),
          FMT(annotation_prefix, " _B"))
    , _compute_C(
          pb,
          in.frobenius_map(6), // _A
          _compute_B.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " C")),
          FMT(annotation_prefix, " _C"))
    , _compute_D_times_C(
          pb,
          _compute_C.result().frobenius_map(2), // _D
          _compute_C.result(),
          _result,
          FMT(annotation_prefix, " _D_times_C"))
{
}

template<typename ppT>
const Fp12_2over3over2_variable<libff::Fqk<other_curve<ppT>>>
    &bls12_377_final_exp_first_part_gadget<ppT>::result() const
{
    return _result;
}

template<typename ppT>
void bls12_377_final_exp_first_part_gadget<ppT>::generate_r1cs_constraints()
{
    _compute_B.generate_r1cs_constraints();
    _compute_C.generate_r1cs_constraints();
    _compute_D_times_C.generate_r1cs_constraints();
}

template<typename ppT>
void bls12_377_final_exp_first_part_gadget<ppT>::generate_r1cs_witness()
{
    _compute_B.generate_r1cs_witness();
    _compute_C._A.evaluate();
    _compute_C.generate_r1cs_witness();
    _compute_D_times_C._A.evaluate();
    _compute_D_times_C.generate_r1cs_witness();
}

// bls12_377_exp_by_z_gadget methods

template<typename ppT>
bls12_377_exp_by_z_gadget<ppT>::bls12_377_exp_by_z_gadget(
    protoboard<FieldT> &pb,
    const Fp12_2over3over2_variable<FqkT> &in,
    const Fp12_2over3over2_variable<FqkT> &result,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix), _result(result)
{
    // There is some complexity in ensuring that the result uses _result as an
    // output variable. If bls12_377_final_exponent_is_z_neg, we perform all
    // square and multiplies into intermediate variables and then unitary
    // inverse into _result. Otherwise, care must be taken during the final
    // iteration so that _result holds the output from the final multiply.

    if (libff::bls12_377_final_exponent_is_z_neg) {
        initialize_z_neg(pb, in, annotation_prefix);
    } else {
        initialize_z_pos(pb, in, annotation_prefix);
    }
}

template<typename ppT>
void bls12_377_exp_by_z_gadget<ppT>::initialize_z_neg(
    protoboard<FieldT> &pb,
    const Fp12_2over3over2_variable<FqkT> &in,
    const std::string &annotation_prefix)
{
    const Fp12_2over3over2_variable<FqkT> *res = &in;

    // Iterate through all bits, then perform a unitary_inverse into result

    const size_t num_bits = libff::bls12_377_final_exponent_z.num_bits();
    for (size_t bit_idx = num_bits - 1; bit_idx > 0; --bit_idx) {
        // result <- result.cyclotomic_squared()
        _squares.push_back(
            std::shared_ptr<cyclotomic_square>(new cyclotomic_square(
                pb,
                *res,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " res^2")),
                FMT(annotation_prefix, " _squares[%zu]", _squares.size()))));
        res = &(_squares.back()->result());

        if (libff::bls12_377_final_exponent_z.test_bit(bit_idx - 1)) {
            // result <- result * elt
            _multiplies.push_back(std::shared_ptr<multiply>(new multiply(
                pb,
                *res,
                in,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " res*in")),
                FMT(annotation_prefix,
                    " _multiplies[%zu]",
                    _multiplies.size()))));
            res = &(_multiplies.back()->result());
        }
    }

    _inverse = std::shared_ptr<unitary_inverse>(new unitary_inverse(
        pb, *res, _result, FMT(annotation_prefix, " res.inv")));
}

template<typename ppT>
void bls12_377_exp_by_z_gadget<ppT>::initialize_z_pos(
    protoboard<FieldT> &pb,
    const Fp12_2over3over2_variable<FqkT> &in,
    const std::string &annotation_prefix)
{
    const Fp12_2over3over2_variable<FqkT> *res = &in;

    // Iterate through all bits, leaving the last one as a special case.
    const size_t num_bits = libff::bls12_377_final_exponent_z.num_bits();
    for (size_t bit_idx = num_bits - 1; bit_idx > 1; --bit_idx) {
        // result <- result.cyclotomic_squared()
        _squares.push_back(
            std::shared_ptr<cyclotomic_square>(new cyclotomic_square(
                pb,
                *res,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " res^2")),
                FMT(annotation_prefix, " _squares[%zu]", _squares.size()))));
        res = &(_squares.back()->result());

        if (libff::bls12_377_final_exponent_z.test_bit(bit_idx - 1)) {
            // result <- result * elt
            _multiplies.push_back(std::shared_ptr<multiply>(new multiply(
                pb,
                *res,
                in,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " res*in")),
                FMT(annotation_prefix,
                    " _multiplies[%zu]",
                    _multiplies.size()))));
            res = &(_multiplies.back()->result());
        }
    }

    // Write the output of the final iteration to result.
    assert(libff::bls12_377_final_exponent_z.test_bit(0));
    _squares.push_back(std::shared_ptr<cyclotomic_square>(new cyclotomic_square(
        pb,
        *res,
        Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, "res^2")),
        FMT(annotation_prefix, " _squares[%zu]", _squares.size()))));
    res = &(_squares.back()->result());

    _multiplies.push_back(std::shared_ptr<multiply>(new multiply(
        pb,
        *res,
        in,
        _result,
        FMT(annotation_prefix, " _multiplies[%zu]", _multiplies.size()))));
}

template<typename ppT>
const Fp12_2over3over2_variable<libff::Fqk<other_curve<ppT>>>
    &bls12_377_exp_by_z_gadget<ppT>::result() const
{
    return _result;
}

template<typename ppT>
void bls12_377_exp_by_z_gadget<ppT>::generate_r1cs_constraints()
{
    size_t sqr_idx = 0;
    size_t mul_idx = 0;
    const size_t num_bits = libff::bls12_377_final_exponent_z.num_bits();
    for (size_t bit_idx = num_bits - 1; bit_idx > 0; --bit_idx) {
        _squares[sqr_idx++]->generate_r1cs_constraints();
        if (libff::bls12_377_final_exponent_z.test_bit(bit_idx - 1)) {
            _multiplies[mul_idx++]->generate_r1cs_constraints();
        }
    }

    if (_inverse) {
        _inverse->generate_r1cs_constraints();
    }
}

template<typename ppT>
void bls12_377_exp_by_z_gadget<ppT>::generate_r1cs_witness()
{
    size_t sqr_idx = 0;
    size_t mul_idx = 0;
    const size_t num_bits = libff::bls12_377_final_exponent_z.num_bits();
    for (size_t bit_idx = num_bits - 1; bit_idx > 0; --bit_idx) {
        _squares[sqr_idx++]->generate_r1cs_witness();
        if (libff::bls12_377_final_exponent_z.test_bit(bit_idx - 1)) {
            _multiplies[mul_idx++]->generate_r1cs_witness();
        }
    }

    if (_inverse) {
        _inverse->generate_r1cs_witness();
    }
}

// bls12_377_final_exp_last_part_gadget methods

template<typename ppT>
bls12_377_final_exp_last_part_gadget<ppT>::bls12_377_final_exp_last_part_gadget(
    protoboard<FieldT> &pb,
    const Fp12_2over3over2_variable<FqkT> &in,
    const Fp12_2over3over2_variable<FqkT> &result,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _result(result)
    // A = [-2]
    , _compute_in_squared(
          pb,
          in,
          Fp12_2over3over2_variable<FqkT>(
              pb, FMT(annotation_prefix, " in_squared")),
          FMT(annotation_prefix, " _compute_in_squared"))
    // B = [z]
    , _compute_B(
          pb,
          in,
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " B")),
          FMT(annotation_prefix, " _compute_B"))
    // C = [2z]
    , _compute_C(
          pb,
          _compute_B.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " C")),
          FMT(annotation_prefix, " _compute_C"))
    // D = [z-2]
    , _compute_D(
          pb,
          _compute_in_squared.result().unitary_inverse(), // _A
          _compute_B.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " D")),
          FMT(annotation_prefix, " _compute_D"))
    // E = [z^2-2z]
    , _compute_E(
          pb,
          _compute_D.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " E")),
          FMT(annotation_prefix, " _compute_E"))
    // F = [z^3-2z^2]
    , _compute_F(
          pb,
          _compute_E.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " F")),
          FMT(annotation_prefix, " _compute_F"))
    // G = [z^4-2z^3]
    , _compute_G(
          pb,
          _compute_F.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " G")),
          FMT(annotation_prefix, " _compute_G"))
    // H = [z^4-2z^3+2z]
    , _compute_H(
          pb,
          _compute_G.result(),
          _compute_C.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " H")),
          FMT(annotation_prefix, " _comptue_H"))
    // I = [z^5-2z^4+2z^2]
    , _compute_I(
          pb,
          _compute_H.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " I")),
          FMT(annotation_prefix, " _compute_I"))
    // J = [-z+2]
    // K = [z^5-2z^4+2z^2-z+2]
    , _compute_K(
          pb,
          _compute_I.result(),
          _compute_D.result().unitary_inverse(), // _J
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " K")),
          FMT(annotation_prefix, " _compute_K"))
    // L = [z^5-2z^4+2z^2-z+3] = [\lambda_0]
    , _compute_L(
          pb,
          _compute_K.result(),
          in,
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " L")),
          FMT(annotation_prefix, " _compute_L"))
    // M = [-1]
    // N = [z^2-2z+1] = [\lambda_3]
    , _compute_N(
          pb,
          _compute_E.result(),
          in,
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " N")),
          FMT(annotation_prefix, " _compute_N"))
    // O = [(z^2-2z+1) * (q^3)]
    // P = [z^4-2z^3+2z-1] = [\lambda_1]
    , _compute_P(
          pb,
          _compute_H.result(),
          in.unitary_inverse(), // _M
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " P")),
          FMT(annotation_prefix, " _compute_P"))
    // Q = [(z^4-2z^3+2z-1) * q]
    // R = [z^3-2z^2+z] = [\lambda_2]
    , _compute_R(
          pb,
          _compute_F.result(),
          _compute_B.result(),
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " R")),
          FMT(annotation_prefix, " _compute_R"))
    // S = [(z^3-2z^2+z) * (q^2)]
    // T = [(z^2-2z+1) * (q^3) + (z^3-2z^2+z) * (q^2)]
    , _compute_T(
          pb,
          _compute_N.result().frobenius_map(3), // _O
          _compute_R.result().frobenius_map(2), // _S
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " T")),
          FMT(annotation_prefix, " _compute_T"))
    // U = [(z^2-2z+1) * (q^3) + (z^3-2z^2+z) * (q^2) + (z^4-2z^3+2z-1) * q]
    , _compute_U(
          pb,
          _compute_T.result(),
          _compute_P.result().frobenius_map(1), // _Q
          Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, " U")),
          FMT(annotation_prefix, " _compute_U"))
    // result = [(z^2-2z+1) * (q^3) + (z^3-2z^2+z) * (q^2) + (z^4-2z^3+2z-1) * q
    //          + z^5-2z^4+2z^2-z+3]
    //        = [(p^4 - p^2 + 1)/r].
    , _compute_U_times_L(
          pb,
          _compute_U.result(),
          _compute_L.result(),
          _result,
          FMT(annotation_prefix, " _compute_U_times_L"))
{
}

template<typename ppT>
const Fp12_2over3over2_variable<libff::Fqk<other_curve<ppT>>>
    &bls12_377_final_exp_last_part_gadget<ppT>::result() const
{
    return _result;
}

template<typename ppT>
void bls12_377_final_exp_last_part_gadget<ppT>::generate_r1cs_constraints()
{
    _compute_in_squared.generate_r1cs_constraints();
    _compute_B.generate_r1cs_constraints();
    _compute_C.generate_r1cs_constraints();
    _compute_D.generate_r1cs_constraints();
    _compute_E.generate_r1cs_constraints();
    _compute_F.generate_r1cs_constraints();
    _compute_G.generate_r1cs_constraints();
    _compute_H.generate_r1cs_constraints();
    _compute_I.generate_r1cs_constraints();
    _compute_K.generate_r1cs_constraints();
    _compute_L.generate_r1cs_constraints();
    _compute_N.generate_r1cs_constraints();
    _compute_P.generate_r1cs_constraints();
    _compute_R.generate_r1cs_constraints();
    _compute_T.generate_r1cs_constraints();
    _compute_U.generate_r1cs_constraints();
    _compute_U_times_L.generate_r1cs_constraints();
}

template<typename ppT>
void bls12_377_final_exp_last_part_gadget<ppT>::generate_r1cs_witness()
{
    _compute_in_squared.generate_r1cs_witness();
    _compute_B.generate_r1cs_witness();
    _compute_C.generate_r1cs_witness();
    _compute_D._A.evaluate();
    _compute_D.generate_r1cs_witness();
    _compute_E.generate_r1cs_witness();
    _compute_F.generate_r1cs_witness();
    _compute_G.generate_r1cs_witness();
    _compute_H.generate_r1cs_witness();
    _compute_I.generate_r1cs_witness();
    _compute_K._B.evaluate();
    _compute_K.generate_r1cs_witness();
    _compute_L.generate_r1cs_witness();
    _compute_N._A.evaluate();
    _compute_N.generate_r1cs_witness();
    _compute_P._B.evaluate();
    _compute_P.generate_r1cs_witness();
    _compute_R.generate_r1cs_witness();
    _compute_T._A.evaluate();
    _compute_T._B.evaluate();
    _compute_T.generate_r1cs_witness();
    _compute_U._B.evaluate();
    _compute_U.generate_r1cs_witness();
    _compute_U_times_L.generate_r1cs_witness();
}

// bls12_377_final_exp_gadget methods

template<typename ppT>
bls12_377_final_exp_gadget<ppT>::bls12_377_final_exp_gadget(
    protoboard<libff::Fr<ppT>> &pb,
    const Fp12_2over3over2_variable<FqkT> &el,
    const pb_variable<FieldT> &result_is_one,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _compute_first_part(
          pb,
          el,
          Fqk_variable<ppT>(pb, FMT(annotation_prefix, " first_part")),
          FMT(annotation_prefix, " _compute_first_part"))
    , _compute_last_part(
          pb,
          _compute_first_part.result(),
          Fqk_variable<ppT>(pb, FMT(annotation_prefix, "last_part")),
          FMT(annotation_prefix, " _compute_last_part"))
    , _result_is_one(result_is_one)
{
}

template<typename ppT>
void bls12_377_final_exp_gadget<ppT>::generate_r1cs_constraints()
{
    _compute_first_part.generate_r1cs_constraints();
    _compute_last_part.generate_r1cs_constraints();

    // Constrain result_is_one to be 0 or 1.
    generate_boolean_r1cs_constraint<FieldT>(
        this->pb,
        _result_is_one,
        FMT(this->annotation_prefix, " result_is_one_boolean"));

    // Use the value of result_is_one to enable / disable the constraints on
    // the 12 components of the result of the final exponentiation in Fq12.
    Fqk_variable<ppT> result = _compute_last_part.result();
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, 1 - result._c0._c0.c0, 0),
        " c0.c0.c0==1");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c0._c0.c1, 0),
        " c0.c0.c1==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c0._c1.c0, 0),
        " c0.c1.c0==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c0._c1.c1, 0),
        " c0.c1.c1==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c0._c2.c0, 0),
        " c0.c2.c0==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c0._c2.c1, 0),
        " c0.c2.c1==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c1._c0.c0, 0),
        " c1.c0.c0==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c1._c0.c1, 0),
        " c1.c0.c1==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c1._c1.c0, 0),
        " c1.c1.c0==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c1._c1.c1, 0),
        " c1.c1.c1==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c1._c2.c0, 0),
        " c1.c2.c0==0");
    this->pb.add_r1cs_constraint(
        r1cs_constraint<FieldT>(_result_is_one, result._c1._c2.c1, 0),
        " c1.c2.c1==0");
}

template<typename ppT>
void bls12_377_final_exp_gadget<ppT>::generate_r1cs_witness()
{
    _compute_first_part.generate_r1cs_witness();
    _compute_last_part.generate_r1cs_witness();

    const FqkT result_val = _compute_last_part.result().get_element();
    this->pb.val(_result_is_one) =
        (result_val == FqkT::one()) ? FieldT::one() : FieldT::zero();
}

} // namespace libsnark

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_FINAL_EXPONENTIATION_TCC_
