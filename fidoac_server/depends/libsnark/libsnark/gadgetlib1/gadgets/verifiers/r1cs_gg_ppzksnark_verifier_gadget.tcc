/** @file
 *****************************************************************************
 * @author     This file is part of libsnark, developed by Clearmatics Ltd
 *             (originally developed by SCIPR Lab) and contributors
 *             (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#ifndef LIBSNARK_GADGETLIB1_GADGETS_VERFIERS_R1CS_GG_PPZKSNARK_VERIFIER_GADGET_TCC_
#define LIBSNARK_GADGETLIB1_GADGETS_VERFIERS_R1CS_GG_PPZKSNARK_VERIFIER_GADGET_TCC_

#include "libsnark/gadgetlib1/constraint_profiling.hpp"
#include "libsnark/gadgetlib1/gadgets/verifiers/r1cs_gg_ppzksnark_verifier_gadget.hpp"

namespace libsnark
{

template<typename ppT>
r1cs_gg_ppzksnark_proof_variable<ppT>::r1cs_gg_ppzksnark_proof_variable(
    protoboard<FieldT> &pb, const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
{
#ifndef NDEBUG
    // g_A, g_C
    const size_t num_G1 = 2;
    // g_B
    const size_t num_G2 = 1;
#endif

    _g_A.reset(new G1_variable<ppT>(pb, FMT(annotation_prefix, " g_A")));
    _g_B.reset(new G2_variable<ppT>(pb, FMT(annotation_prefix, " g_B")));
    _g_C.reset(new G1_variable<ppT>(pb, FMT(annotation_prefix, " g_C")));

    _all_G1_vars = {_g_A, _g_C};
    _all_G2_vars = {_g_B};

    _all_G1_checkers.resize(_all_G1_vars.size());

    for (size_t i = 0; i < _all_G1_vars.size(); ++i) {
        _all_G1_checkers[i].reset(new G1_checker<ppT>(
            pb,
            *_all_G1_vars[i],
            FMT(annotation_prefix, " all_G1_checkers_%zu", i)));
    }

    _G2_checker.reset(
        new G2_checker<ppT>(pb, *_g_B, FMT(annotation_prefix, " G2_checker")));

    assert(_all_G1_vars.size() == num_G1);
    assert(_all_G2_vars.size() == num_G2);
}

template<typename ppT>
void r1cs_gg_ppzksnark_proof_variable<ppT>::generate_r1cs_constraints()
{
    for (auto &G1_checker : _all_G1_checkers) {
        G1_checker->generate_r1cs_constraints();
    }

    _G2_checker->generate_r1cs_constraints();
}

template<typename ppT>
void r1cs_gg_ppzksnark_proof_variable<ppT>::generate_r1cs_witness(
    const r1cs_gg_ppzksnark_proof<other_curve<ppT>> &proof)
{
    std::vector<libff::G1<other_curve<ppT>>> G1_elems;
    std::vector<libff::G2<other_curve<ppT>>> G2_elems;

    G1_elems = {proof.g_A, proof.g_C};
    G2_elems = {proof.g_B};

    assert(G1_elems.size() == _all_G1_vars.size());
    assert(G2_elems.size() == _all_G2_vars.size());

    for (size_t i = 0; i < G1_elems.size(); ++i) {
        _all_G1_vars[i]->generate_r1cs_witness(G1_elems[i]);
    }

    for (size_t i = 0; i < G2_elems.size(); ++i) {
        _all_G2_vars[i]->generate_r1cs_witness(G2_elems[i]);
    }

    for (auto &G1_checker : _all_G1_checkers) {
        G1_checker->generate_r1cs_witness();
    }

    _G2_checker->generate_r1cs_witness();
}

template<typename ppT> size_t r1cs_gg_ppzksnark_proof_variable<ppT>::size()
{
    const size_t num_G1 = 2;
    const size_t num_G2 = 1;
    return (
        num_G1 * G1_variable<ppT>::num_field_elems +
        num_G2 * G2_variable<ppT>::num_field_elems);
}

template<typename ppT>
r1cs_gg_ppzksnark_verification_key_variable<ppT>::
    r1cs_gg_ppzksnark_verification_key_variable(
        protoboard<libff::Fr<ppT>> &pb,
        const size_t num_primary_inputs,
        const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _alpha_g1(pb, FMT(annotation_prefix, " alpha_g1"))
    , _beta_g2(pb, FMT(annotation_prefix, " beta_g2"))
    , _delta_g2(pb, FMT(annotation_prefix, " delta_g2"))
    , _encoded_ABC_base(
          new G1_variable<ppT>(pb, FMT(annotation_prefix, " encoded_ABC_base")))
    , _num_primary_inputs(num_primary_inputs)
{
    // Populate _all_vars with alpha, beta, gamma and ABC_base variables.
    _all_vars.insert(
        _all_vars.end(), _alpha_g1.all_vars.begin(), _alpha_g1.all_vars.end());
    _all_vars.insert(
        _all_vars.end(), _beta_g2.all_vars.begin(), _beta_g2.all_vars.end());
    _all_vars.insert(
        _all_vars.end(), _delta_g2.all_vars.begin(), _delta_g2.all_vars.end());
    _all_vars.insert(
        _all_vars.end(),
        _encoded_ABC_base->all_vars.begin(),
        _encoded_ABC_base->all_vars.end());

    // Allocate variables for ABC_g1 elements, and populate _all_vars with each
    // variable.
    _ABC_g1.reserve(_num_primary_inputs);
    for (size_t i = 0; i < _num_primary_inputs; ++i) {
        _ABC_g1.emplace_back(new G1_variable<ppT>(
            pb, FMT(annotation_prefix, " ABC_g1[%zu]", i)));
        const G1_variable<ppT> &ivar = *(_ABC_g1.back());
        _all_vars.insert(
            _all_vars.end(), ivar.all_vars.begin(), ivar.all_vars.end());
    }
}

template<typename ppT>
void r1cs_gg_ppzksnark_verification_key_variable<
    ppT>::generate_r1cs_constraints()
{
}

template<typename ppT>
void r1cs_gg_ppzksnark_verification_key_variable<ppT>::generate_r1cs_witness(
    const r1cs_gg_ppzksnark_verification_key<other_curve<ppT>> &vk)
{
    assert(vk.ABC_g1.rest.size() == _num_primary_inputs);
    _alpha_g1.generate_r1cs_witness(vk.alpha_g1);
    _beta_g2.generate_r1cs_witness(vk.beta_g2);
    _delta_g2.generate_r1cs_witness(vk.delta_g2);
    _encoded_ABC_base->generate_r1cs_witness(vk.ABC_g1.first);
    for (size_t i = 0; i < _num_primary_inputs; ++i) {
        assert(vk.ABC_g1.rest.indices[i] == i);
        _ABC_g1[i]->generate_r1cs_witness(vk.ABC_g1.rest.values[i]);
    }
}

template<typename ppT>
size_t r1cs_gg_ppzksnark_verification_key_variable<ppT>::num_primary_inputs()
    const
{
    return _num_primary_inputs;
}

template<typename ppT>
const pb_linear_combination_array<libff::Fr<ppT>>
    &r1cs_gg_ppzksnark_verification_key_variable<ppT>::get_all_vars() const
{
    return _all_vars;
}

template<typename ppT>
std::vector<libff::Fr<ppT>> r1cs_gg_ppzksnark_verification_key_variable<ppT>::
    get_verification_key_scalars(
        const r1cs_gg_ppzksnark_verification_key<other_curve<ppT>> &r1cs_vk)
{
    // TODO: It would be much more efficient to simply iterate through the
    // field elements of r1cs_vk, replicating the order in the constructor. For
    // now, to avoid replicating that order (which also depends on the G1 and
    // G2 variable gadgets), we instantiate this gadget and extract the values
    // of _all_vars.

    const size_t num_primary_inputs = r1cs_vk.ABC_g1.rest.indices.size();

    protoboard<FieldT> pb;
    r1cs_gg_ppzksnark_verification_key_variable<ppT> vk(
        pb, num_primary_inputs, "vk");
    vk.generate_r1cs_witness(r1cs_vk);
    const pb_linear_combination_array<FieldT> &vk_vars = vk.get_all_vars();

    std::vector<FieldT> scalar_values;
    scalar_values.reserve(vk_vars.size());
    for (const pb_linear_combination<FieldT> &lc : vk_vars) {
        scalar_values.push_back(pb.lc_val(lc));
    }

    return scalar_values;
}

template<typename ppT>
r1cs_gg_ppzksnark_preprocessed_verification_key_variable<
    ppT>::r1cs_gg_ppzksnark_preprocessed_verification_key_variable()
{
    // will be allocated outside
}

template<typename ppT>
r1cs_gg_ppzksnark_preprocessed_verification_key_variable<ppT>::
    r1cs_gg_ppzksnark_preprocessed_verification_key_variable(
        protoboard<FieldT> &pb,
        const r1cs_gg_ppzksnark_verification_key<other_curve<ppT>> &r1cs_vk,
        const std::string &annotation_prefix)
{
    _encoded_ABC_base.reset(new G1_variable<ppT>(
        pb, r1cs_vk.ABC_g1.first, FMT(annotation_prefix, " encoded_ABC_base")));
    _ABC_g1.resize(r1cs_vk.ABC_g1.rest.indices.size());
    for (size_t i = 0; i < r1cs_vk.ABC_g1.rest.indices.size(); ++i) {
        assert(r1cs_vk.ABC_g1.rest.indices[i] == i);
        _ABC_g1[i].reset(new G1_variable<ppT>(
            pb,
            r1cs_vk.ABC_g1.rest.values[i],
            FMT(annotation_prefix, " ABC_g1[%zu]", i)));
    }

    _vk_alpha_g1_precomp.reset(new G1_precomputation<ppT>(
        pb, r1cs_vk.alpha_g1, FMT(annotation_prefix, " vk_alpha_g1_precomp")));

    _vk_generator_g2_precomp.reset(new G2_precomputation<ppT>(
        pb,
        libff::G2<other_curve<ppT>>::one(),
        FMT(annotation_prefix, " vk_generator_g2_precomp")));
    _vk_beta_g2_precomp.reset(new G2_precomputation<ppT>(
        pb, r1cs_vk.beta_g2, FMT(annotation_prefix, " vk_beta_g2_precomp")));
    _vk_delta_g2_precomp.reset(new G2_precomputation<ppT>(
        pb, r1cs_vk.delta_g2, FMT(annotation_prefix, " vk_delta_g2_precomp")));
}

template<typename ppT>
r1cs_gg_ppzksnark_verifier_process_vk_gadget<ppT>::
    r1cs_gg_ppzksnark_verifier_process_vk_gadget(
        protoboard<FieldT> &pb,
        const r1cs_gg_ppzksnark_verification_key_variable<ppT> &vk,
        r1cs_gg_ppzksnark_preprocessed_verification_key_variable<ppT> &pvk,
        const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix), _vk(vk), _pvk(pvk)
{
    _pvk._encoded_ABC_base = vk._encoded_ABC_base;
    _pvk._ABC_g1 = vk._ABC_g1;

    _pvk._vk_alpha_g1_precomp.reset(new G1_precomputation<ppT>());

    _pvk._vk_generator_g2_precomp.reset(new G2_precomputation<ppT>());
    _pvk._vk_beta_g2_precomp.reset(new G2_precomputation<ppT>());
    _pvk._vk_delta_g2_precomp.reset(new G2_precomputation<ppT>());

    _compute_vk_alpha_g1_precomp.reset(new precompute_G1_gadget<ppT>(
        pb,
        vk._alpha_g1,
        *pvk._vk_alpha_g1_precomp,
        FMT(annotation_prefix, " compute_vk_alpha_g1_precomp")));

    _pvk._vk_generator_g2_precomp.reset(new G2_precomputation<ppT>(
        pb,
        libff::G2<other_curve<ppT>>::one(),
        FMT(annotation_prefix, " vk_generator_g2_precomp")));
    _compute_vk_beta_g2_precomp.reset(new precompute_G2_gadget<ppT>(
        pb,
        vk._beta_g2,
        *pvk._vk_beta_g2_precomp,
        FMT(annotation_prefix, " compute_vk_beta_g2_precomp")));
    _compute_vk_delta_g2_precomp.reset(new precompute_G2_gadget<ppT>(
        pb,
        vk._delta_g2,
        *pvk._vk_delta_g2_precomp,
        FMT(annotation_prefix, " compute_vk_delta_g2_precomp")));
}

template<typename ppT>
void r1cs_gg_ppzksnark_verifier_process_vk_gadget<
    ppT>::generate_r1cs_constraints()
{
    _compute_vk_alpha_g1_precomp->generate_r1cs_constraints();

    _compute_vk_beta_g2_precomp->generate_r1cs_constraints();
    _compute_vk_delta_g2_precomp->generate_r1cs_constraints();
}

template<typename ppT>
void r1cs_gg_ppzksnark_verifier_process_vk_gadget<ppT>::generate_r1cs_witness()
{
    _compute_vk_alpha_g1_precomp->generate_r1cs_witness();

    _compute_vk_beta_g2_precomp->generate_r1cs_witness();
    _compute_vk_delta_g2_precomp->generate_r1cs_witness();
}

template<typename ppT>
r1cs_gg_ppzksnark_online_verifier_gadget<ppT>::
    r1cs_gg_ppzksnark_online_verifier_gadget(
        protoboard<FieldT> &pb,
        const r1cs_gg_ppzksnark_preprocessed_verification_key_variable<ppT>
            &pvk,
        const pb_variable_array<FieldT> &input,
        const size_t elt_size,
        const r1cs_gg_ppzksnark_proof_variable<ppT> &proof,
        const pb_variable<FieldT> &result_QAP_valid,
        const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
    , _pvk(pvk)
    , _input(input)
    , _elt_size(elt_size)
    , _proof(proof)
    , _result(result_QAP_valid)
    , _input_len(input.size())
{
    // 1. Accumulate input and store base in acc
    // See:
    // https://github.com/clearmatics/libsnark/blob/master/libsnark/zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/r1cs_gg_ppzksnark.tcc#L568-L571
    _acc.reset(new G1_variable<ppT>(pb, FMT(annotation_prefix, " acc")));
    std::vector<G1_variable<ppT>> IC_terms;
    for (size_t i = 0; i < _pvk._ABC_g1.size(); ++i) {
        IC_terms.emplace_back(*(_pvk._ABC_g1[i]));
    }
    _accumulate_input.reset(new G1_multiscalar_mul_gadget<ppT>(
        pb,
        *(_pvk._encoded_ABC_base),
        _input,
        _elt_size,
        IC_terms,
        *_acc,
        FMT(annotation_prefix, " accumulate_input")));

    // 2. Do the precomputations on the inputs of the pairings
    // See:
    // https://github.com/clearmatics/libsnark/blob/master/libsnark/zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/r1cs_gg_ppzksnark.tcc#L588-L591
    //
    // 2.1 Allocate the results of the precomputations
    _proof_g_A_precomp.reset(new G1_precomputation<ppT>());
    _proof_g_B_precomp.reset(new G2_precomputation<ppT>());
    _proof_g_C_precomp.reset(new G1_precomputation<ppT>());
    _acc_precomp.reset(new G1_precomputation<ppT>());
    // 2.2 Do the precomputations
    _compute_proof_g_A_precomp.reset(new precompute_G1_gadget<ppT>(
        pb,
        *(proof._g_A),
        *_proof_g_A_precomp,
        FMT(annotation_prefix, " compute_proof_g_A_precomp")));
    _compute_proof_g_B_precomp.reset(new precompute_G2_gadget<ppT>(
        pb,
        *(proof._g_B),
        *_proof_g_B_precomp,
        FMT(annotation_prefix, " compute_proof_g_B_precomp")));
    _compute_proof_g_C_precomp.reset(new precompute_G1_gadget<ppT>(
        pb,
        *(proof._g_C),
        *_proof_g_C_precomp,
        FMT(annotation_prefix, " compute_proof_g_C_precomp")));
    _compute_acc_precomp.reset(new precompute_G1_gadget<ppT>(
        pb,
        *_acc,
        *_acc_precomp,
        FMT(annotation_prefix, " compute_acc_precomp")));

    // 3. Carry out the pairing checks to check QAP equation
    _check_QAP_valid.reset(new check_e_equals_eee_gadget<ppT>(
        pb,
        // LHS
        *_proof_g_A_precomp,
        *_proof_g_B_precomp,
        // RHS
        *(pvk._vk_alpha_g1_precomp),
        *(pvk._vk_beta_g2_precomp),
        *(_acc_precomp),
        *(pvk._vk_generator_g2_precomp),
        *(_proof_g_C_precomp),
        *(pvk._vk_delta_g2_precomp),
        // Result of pairing check (allocated outside of this circuit)
        _result,
        FMT(annotation_prefix, " check_QAP_valid")));
}

template<typename ppT>
void r1cs_gg_ppzksnark_online_verifier_gadget<ppT>::generate_r1cs_constraints()
{
    // For the macros below
    using namespace libsnark;

    PROFILE_CONSTRAINTS(this->pb, "accumulate verifier input")
    {
        libff::print_indent();
        printf(
            "* Number of bits as an input to verifier gadget: %zu\n",
            _input.size());
        _accumulate_input->generate_r1cs_constraints();
    }

    PROFILE_CONSTRAINTS(this->pb, "rest of the verifier")
    {
        _compute_proof_g_A_precomp->generate_r1cs_constraints();
        _compute_proof_g_B_precomp->generate_r1cs_constraints();
        _compute_proof_g_C_precomp->generate_r1cs_constraints();
        _compute_acc_precomp->generate_r1cs_constraints();

        _check_QAP_valid->generate_r1cs_constraints();
    }
}

template<typename ppT>
void r1cs_gg_ppzksnark_online_verifier_gadget<ppT>::generate_r1cs_witness()
{
    _accumulate_input->generate_r1cs_witness();

    _compute_proof_g_A_precomp->generate_r1cs_witness();
    _compute_proof_g_B_precomp->generate_r1cs_witness();
    _compute_proof_g_C_precomp->generate_r1cs_witness();
    _compute_acc_precomp->generate_r1cs_witness();

    _check_QAP_valid->generate_r1cs_witness();
}

template<typename ppT>
r1cs_gg_ppzksnark_verifier_gadget<ppT>::r1cs_gg_ppzksnark_verifier_gadget(
    protoboard<FieldT> &pb,
    const r1cs_gg_ppzksnark_verification_key_variable<ppT> &vk,
    const pb_variable_array<FieldT> &input,
    const size_t elt_size,
    const r1cs_gg_ppzksnark_proof_variable<ppT> &proof,
    const pb_variable<FieldT> &result,
    const std::string &annotation_prefix)
    : gadget<FieldT>(pb, annotation_prefix)
{
    _pvk.reset(
        new r1cs_gg_ppzksnark_preprocessed_verification_key_variable<ppT>());
    _compute_pvk.reset(new r1cs_gg_ppzksnark_verifier_process_vk_gadget<ppT>(
        pb, vk, *_pvk, FMT(annotation_prefix, " compute_pvk")));
    _online_verifier.reset(new r1cs_gg_ppzksnark_online_verifier_gadget<ppT>(
        pb,
        *_pvk,
        input,
        elt_size,
        proof,
        result,
        FMT(annotation_prefix, " online_verifier")));
}

template<typename ppT>
void r1cs_gg_ppzksnark_verifier_gadget<ppT>::generate_r1cs_constraints()
{
    // For the macros below
    using namespace libsnark;

    PROFILE_CONSTRAINTS(this->pb, "precompute pvk")
    {
        _compute_pvk->generate_r1cs_constraints();
    }

    PROFILE_CONSTRAINTS(this->pb, "online verifier")
    {
        _online_verifier->generate_r1cs_constraints();
    }
}

template<typename ppT>
void r1cs_gg_ppzksnark_verifier_gadget<ppT>::generate_r1cs_witness()
{
    _compute_pvk->generate_r1cs_witness();
    _online_verifier->generate_r1cs_witness();
}

} // namespace libsnark

#endif // LIBSNARK_GADGETLIB1_GADGETS_VERFIERS_R1CS_GG_PPZKSNARK_VERIFIER_GADGET_TCC_
