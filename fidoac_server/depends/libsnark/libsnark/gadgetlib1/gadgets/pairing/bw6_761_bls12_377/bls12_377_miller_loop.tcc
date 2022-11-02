/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MILLER_LOOP_TCC_
#define LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MILLER_LOOP_TCC_

#include "libsnark/gadgetlib1/gadgets/basic_gadgets.hpp"
#include "libsnark/gadgetlib1/gadgets/pairing/bw6_761_bls12_377/bls12_377_miller_loop.hpp"

namespace libsnark
{

// bls12_377_ate_compute_f_ell_P methods

template<typename ppT>
bls12_377_ate_compute_f_ell_P<ppT>::bls12_377_ate_compute_f_ell_P(
    protoboard<FieldT> &pb,
    const pb_linear_combination<FieldT> &Px,
    const pb_linear_combination<FieldT> &Py,
    const bls12_377_ate_ell_coeffs<ppT> &ell_coeffs,
    const Fp12_2over3over2_variable<FqkT> &f,
    const Fp12_2over3over2_variable<FqkT> &f_out,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _compute_ell_vv_times_Px(
          pb,
          ell_coeffs.ell_vv,
          Px,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " ell_vv_times_Px")),
          FMT(annotation_prefix, " _compute_ell_vv_times_Px"))
    , _compute_ell_vw_times_Py(
          pb,
          ell_coeffs.ell_vw,
          Py,
          Fqe_variable<ppT>(pb, FMT(annotation_prefix, " ell_vw_times_Py")),
          FMT(annotation_prefix, " _compute_ell_vw_times_Py"))
    , _compute_f_mul_ell_P(
          pb,
          f,
          ell_coeffs.ell_0,
          _compute_ell_vv_times_Px.result,
          _compute_ell_vw_times_Py.result,
          f_out,
          FMT(annotation_prefix, " _compute_f_mul_ell_P"))
{
}

template<typename ppT>
const Fp12_2over3over2_variable<libff::Fqk<other_curve<ppT>>>
    &bls12_377_ate_compute_f_ell_P<ppT>::result() const
{
    return _compute_f_mul_ell_P.result();
}

template<typename ppT>
void bls12_377_ate_compute_f_ell_P<ppT>::generate_r1cs_constraints()
{
    _compute_ell_vv_times_Px.generate_r1cs_constraints();
    _compute_ell_vw_times_Py.generate_r1cs_constraints();
    _compute_f_mul_ell_P.generate_r1cs_constraints();
}

template<typename ppT>
void bls12_377_ate_compute_f_ell_P<ppT>::generate_r1cs_witness()
{
    _compute_ell_vv_times_Px.generate_r1cs_witness();
    _compute_ell_vw_times_Py.generate_r1cs_witness();
    _compute_f_mul_ell_P.generate_r1cs_witness();
}

// bls12_377_miller_loop_gadget methods

template<typename ppT>
bls12_377_miller_loop_gadget<ppT>::bls12_377_miller_loop_gadget(
    protoboard<FieldT> &pb,
    const bls12_377_G1_precomputation<ppT> &prec_P,
    const bls12_377_G2_precomputation<ppT> &prec_Q,
    const Fqk_variable<ppT> &result,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _f0(pb, FqkT::one(), FMT(annotation_prefix, " f0"))
{
    size_t coeff_idx = 0;
    const Fp12_2over3over2_variable<FqkT> *f = &_f0;

    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        // f <- f^2
        _f_squared.push_back(
            std::shared_ptr<Fp12_2over3over2_square_gadget<FqkT>>(
                new Fp12_2over3over2_square_gadget<FqkT>(
                    pb,
                    *f,
                    Fp12_2over3over2_variable<FqkT>(
                        pb, FMT(annotation_prefix, " f^2")),
                    FMT(annotation_prefix, " _f_squared[%zu]", bits.index()))));
        f = &_f_squared.back()->result();

        // f <- f^2 * ell(P)
        _f_ell_P.push_back(std::shared_ptr<bls12_377_ate_compute_f_ell_P<ppT>>(
            new bls12_377_ate_compute_f_ell_P<ppT>(
                pb,
                *prec_P._Px,
                *prec_P._Py,
                *prec_Q._coeffs[coeff_idx++],
                *f,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " f^2*ell(P)")),
                FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size()))));
        f = &_f_ell_P.back()->result();

        if (bits.current()) {
            // f <- f * ell(P)
            if (bits.last()) {
                _f_ell_P.push_back(
                    std::shared_ptr<bls12_377_ate_compute_f_ell_P<ppT>>(
                        new bls12_377_ate_compute_f_ell_P<ppT>(
                            pb,
                            *prec_P._Px,
                            *prec_P._Py,
                            *prec_Q._coeffs[coeff_idx++],
                            *f,
                            result,
                            FMT(annotation_prefix,
                                " _f_ell_P[%zu]",
                                _f_ell_P.size()))));
            } else {
                _f_ell_P.push_back(
                    std::shared_ptr<bls12_377_ate_compute_f_ell_P<ppT>>(
                        new bls12_377_ate_compute_f_ell_P<ppT>(
                            pb,
                            *prec_P._Px,
                            *prec_P._Py,
                            *prec_Q._coeffs[coeff_idx++],
                            *f,
                            Fp12_2over3over2_variable<FqkT>(
                                pb, FMT(annotation_prefix, " f*ell(P)")),
                            FMT(annotation_prefix,
                                " _f_ell_P[%zu]",
                                _f_ell_P.size()))));
            }
            f = &_f_ell_P.back()->result();
        }
    }
}

template<typename ppT>
const Fp12_2over3over2_variable<libff::Fqk<other_curve<ppT>>>
    &bls12_377_miller_loop_gadget<ppT>::result() const
{
    return _f_ell_P.back()->result();
}

template<typename ppT>
void bls12_377_miller_loop_gadget<ppT>::generate_r1cs_constraints()
{
    // TODO: everything is allocated, so constraint generation does not need
    // to be done in this order. For now, keep a consistent loop.

    size_t sqr_idx = 0;
    size_t f_ell_P_idx = 0;
    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        _f_squared[sqr_idx++]->generate_r1cs_constraints();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        if (bits.current()) {
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        }
    }

    assert(sqr_idx == _f_squared.size());
    assert(f_ell_P_idx == _f_ell_P.size());
}

template<typename ppT>
void bls12_377_miller_loop_gadget<ppT>::generate_r1cs_witness()
{
    size_t sqr_idx = 0;
    size_t f_ell_P_idx = 0;
    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        _f_squared[sqr_idx++]->generate_r1cs_witness();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        if (bits.current()) {
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        }
    }

    assert(sqr_idx == _f_squared.size());
    assert(f_ell_P_idx == _f_ell_P.size());
}

template<typename ppT>
bls12_377_e_over_e_miller_loop_gadget<ppT>::
    bls12_377_e_over_e_miller_loop_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const bls12_377_G1_precomputation<ppT> &P1_prec,
        const bls12_377_G2_precomputation<ppT> &Q1_prec,
        const bls12_377_G1_precomputation<ppT> &P2_prec,
        const bls12_377_G2_precomputation<ppT> &Q2_prec,
        const Fp12_2over3over2_variable<FqkT> &result,
        const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _f0(pb, FqkT::one(), FMT(annotation_prefix, " f0"))
    , _minus_P2_Y()
{
    _minus_P2_Y.assign(pb, -(*P2_prec._Py));
    size_t coeff_idx = 0;
    const Fp12_2over3over2_variable<FqkT> *f = &_f0;

    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        // f <- f^2
        _f_squared.emplace_back(new Fp12_2over3over2_square_gadget<FqkT>(
            pb,
            *f,
            Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, "f^2")),
            FMT(annotation_prefix, " _f_squared[%zu]", _f_squared.size())));
        f = &_f_squared.back()->result();

        // f <- f^2 * ell_Q1(P1)
        _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
            pb,
            *P1_prec._Px,
            *P1_prec._Py,
            *Q1_prec._coeffs[coeff_idx],
            *f,
            Fp12_2over3over2_variable<FqkT>(
                pb, FMT(annotation_prefix, " f^2*ell_Q1(P1)")),
            FMT(annotation_prefix, " _f_ell_P1[%zu]", _f_ell_P.size())));
        f = &_f_ell_P.back()->result();

        // f <- f^2 * ell_Q2(P2)
        _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
            pb,
            *P2_prec._Px,
            _minus_P2_Y,
            *Q2_prec._coeffs[coeff_idx],
            *f,
            Fp12_2over3over2_variable<FqkT>(
                pb, FMT(annotation_prefix, " f^2*ell_Q2(P2)")),
            FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
        f = &_f_ell_P.back()->result();

        assert(0 == _f_ell_P.size() % 2);

        ++coeff_idx;

        if (bits.current()) {
            // f <- f * ell_Q1(P1)
            _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
                pb,
                *P1_prec._Px,
                *P1_prec._Py,
                *Q1_prec._coeffs[coeff_idx],
                *f,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " f*ell_Q1(P2)")),
                FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
            f = &_f_ell_P.back()->result();

            // f <- f * ell_Q2(P2)
            if (bits.last()) {
                _f_ell_P.emplace_back(
                    std::shared_ptr<bls12_377_ate_compute_f_ell_P<ppT>>(
                        new bls12_377_ate_compute_f_ell_P<ppT>(
                            pb,
                            *P2_prec._Px,
                            _minus_P2_Y,
                            *Q2_prec._coeffs[coeff_idx],
                            *f,
                            result,
                            FMT(annotation_prefix,
                                " _f_ell_P[%zu]",
                                _f_ell_P.size()))));
            } else {
                _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
                    pb,
                    *P2_prec._Px,
                    _minus_P2_Y,
                    *Q2_prec._coeffs[coeff_idx],
                    *f,
                    Fp12_2over3over2_variable<FqkT>(
                        pb, FMT(annotation_prefix, " f*ell_Q2(P2)")),
                    FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
            }
            f = &_f_ell_P.back()->result();

            assert(0 == _f_ell_P.size() % 2);

            ++coeff_idx;
        }
    }
}

template<typename ppT>
void bls12_377_e_over_e_miller_loop_gadget<ppT>::generate_r1cs_constraints()
{
    size_t sqr_idx = 0;
    size_t f_ell_P_idx = 0;
    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        _f_squared[sqr_idx++]->generate_r1cs_constraints();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        if (bits.current()) {
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        }
    }

    assert(sqr_idx == _f_squared.size());
    assert(f_ell_P_idx == _f_ell_P.size());
}

template<typename ppT>
void bls12_377_e_over_e_miller_loop_gadget<ppT>::generate_r1cs_witness()
{
    _minus_P2_Y.evaluate(this->pb);
    size_t sqr_idx = 0;
    size_t f_ell_P_idx = 0;
    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        _f_squared[sqr_idx++]->generate_r1cs_witness();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        if (bits.current()) {
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        }
    }

    assert(sqr_idx == _f_squared.size());
    assert(f_ell_P_idx == _f_ell_P.size());
}

template<typename ppT>
bls12_377_e_times_e_times_e_over_e_miller_loop_gadget<ppT>::
    bls12_377_e_times_e_times_e_over_e_miller_loop_gadget(
        protoboard<libff::Fr<ppT>> &pb,
        const bls12_377_G1_precomputation<ppT> &P1_prec,
        const bls12_377_G2_precomputation<ppT> &Q1_prec,
        const bls12_377_G1_precomputation<ppT> &P2_prec,
        const bls12_377_G2_precomputation<ppT> &Q2_prec,
        const bls12_377_G1_precomputation<ppT> &P3_prec,
        const bls12_377_G2_precomputation<ppT> &Q3_prec,
        const bls12_377_G1_precomputation<ppT> &P4_prec,
        const bls12_377_G2_precomputation<ppT> &Q4_prec,
        const Fp12_2over3over2_variable<FqkT> &result,
        const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _f0(pb, FqkT::one(), FMT(annotation_prefix, " f0"))
    , _minus_P4_Y()
{
    _minus_P4_Y.assign(pb, -(*P4_prec._Py));
    size_t coeff_idx = 0;
    const Fp12_2over3over2_variable<FqkT> *f = &_f0;

    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        // f <- f^2
        _f_squared.emplace_back(new Fp12_2over3over2_square_gadget<FqkT>(
            pb,
            *f,
            Fp12_2over3over2_variable<FqkT>(pb, FMT(annotation_prefix, "f^2")),
            FMT(annotation_prefix, " _f_squared[%zu]", _f_squared.size())));
        f = &_f_squared.back()->result();

        // f <- f^2 * ell_Q1(P1)
        _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
            pb,
            *P1_prec._Px,
            *P1_prec._Py,
            *Q1_prec._coeffs[coeff_idx],
            *f,
            Fp12_2over3over2_variable<FqkT>(
                pb, FMT(annotation_prefix, " f^2*ell_Q1(P1)")),
            FMT(annotation_prefix, " _f_ell_P1[%zu]", _f_ell_P.size())));
        f = &_f_ell_P.back()->result();

        // f <- f^2 * ell_Q2(P2)
        _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
            pb,
            *P2_prec._Px,
            *P2_prec._Py,
            *Q2_prec._coeffs[coeff_idx],
            *f,
            Fp12_2over3over2_variable<FqkT>(
                pb, FMT(annotation_prefix, " f^2*ell_Q2(P2)")),
            FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
        f = &_f_ell_P.back()->result();

        // f <- f^2 * ell_Q3(P3)
        _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
            pb,
            *P3_prec._Px,
            *P3_prec._Py,
            *Q3_prec._coeffs[coeff_idx],
            *f,
            Fp12_2over3over2_variable<FqkT>(
                pb, FMT(annotation_prefix, " f^2*ell_Q3(P3)")),
            FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
        f = &_f_ell_P.back()->result();

        // f <- f^2 * ell_Q4(P4)
        _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
            pb,
            *P4_prec._Px,
            _minus_P4_Y,
            *Q4_prec._coeffs[coeff_idx],
            *f,
            Fp12_2over3over2_variable<FqkT>(
                pb, FMT(annotation_prefix, " f^2*ell_Q4(P4)")),
            FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
        f = &_f_ell_P.back()->result();

        assert(0 == _f_ell_P.size() % 4);

        ++coeff_idx;

        if (bits.current()) {
            // f <- f * ell_Q1(P1)
            _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
                pb,
                *P1_prec._Px,
                *P1_prec._Py,
                *Q1_prec._coeffs[coeff_idx],
                *f,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " f*ell_Q1(P2)")),
                FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
            f = &_f_ell_P.back()->result();

            // f <- f * ell_Q2(P2)
            _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
                pb,
                *P2_prec._Px,
                *P2_prec._Py,
                *Q2_prec._coeffs[coeff_idx],
                *f,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " f*ell_Q2(P2)")),
                FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
            f = &_f_ell_P.back()->result();

            // f <- f * ell_Q3(P3)
            _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
                pb,
                *P3_prec._Px,
                *P3_prec._Py,
                *Q3_prec._coeffs[coeff_idx],
                *f,
                Fp12_2over3over2_variable<FqkT>(
                    pb, FMT(annotation_prefix, " f*ell_Q3(P3)")),
                FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
            f = &_f_ell_P.back()->result();

            // f <- f * ell_Q4(P4)
            if (bits.last()) {
                _f_ell_P.emplace_back(
                    std::shared_ptr<bls12_377_ate_compute_f_ell_P<ppT>>(
                        new bls12_377_ate_compute_f_ell_P<ppT>(
                            pb,
                            *P4_prec._Px,
                            _minus_P4_Y,
                            *Q4_prec._coeffs[coeff_idx],
                            *f,
                            result,
                            FMT(annotation_prefix,
                                " _f_ell_P[%zu]",
                                _f_ell_P.size()))));
            } else {
                _f_ell_P.emplace_back(new bls12_377_ate_compute_f_ell_P<ppT>(
                    pb,
                    *P4_prec._Px,
                    _minus_P4_Y,
                    *Q4_prec._coeffs[coeff_idx],
                    *f,
                    Fp12_2over3over2_variable<FqkT>(
                        pb, FMT(annotation_prefix, " f*ell_Q4(P4)")),
                    FMT(annotation_prefix, " _f_ell_P[%zu]", _f_ell_P.size())));
            }
            f = &_f_ell_P.back()->result();

            assert(0 == _f_ell_P.size() % 4);

            ++coeff_idx;
        }
    }
}

template<typename ppT>
void bls12_377_e_times_e_times_e_over_e_miller_loop_gadget<
    ppT>::generate_r1cs_constraints()
{
    size_t sqr_idx = 0;
    size_t f_ell_P_idx = 0;
    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        _f_squared[sqr_idx++]->generate_r1cs_constraints();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        if (bits.current()) {
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_constraints();
        }
    }

    assert(sqr_idx == _f_squared.size());
    assert(f_ell_P_idx == _f_ell_P.size());
}

template<typename ppT>
void bls12_377_e_times_e_times_e_over_e_miller_loop_gadget<
    ppT>::generate_r1cs_witness()
{
    _minus_P4_Y.evaluate(this->pb);
    size_t sqr_idx = 0;
    size_t f_ell_P_idx = 0;
    bls12_377_miller_loop_bits bits;
    while (bits.next()) {
        _f_squared[sqr_idx++]->generate_r1cs_witness();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        if (bits.current()) {
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
            _f_ell_P[f_ell_P_idx++]->generate_r1cs_witness();
        }
    }

    assert(sqr_idx == _f_squared.size());
    assert(f_ell_P_idx == _f_ell_P.size());
}

} // namespace libsnark

#endif // LIBSNARK_GADGETLIB1_GADGETS_PAIRING_BW6_761_BLS12_377_BLS12_377_MILLER_LOOP_TCC_
