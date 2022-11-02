/**
 *****************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *****************************************************************************/

#include <exception>
#include <libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp>
#include <libff/algebra/curves/bls12_377/bls12_377_pp.hpp>
#include <libff/common/profiling.hpp>

using namespace libff;

/// Number of random elements to generate (since this process is expensive)
static const size_t ADD_NUM_DIFFERENT_ELEMENTS = 1024;

/// Number of element to operate on in a single iteration.
static const size_t ADD_NUM_ELEMENTS = 1024 * 1024;

template<typename GroupT> bool profile_group_add()
{
    std::vector<GroupT> elements;
    {
        elements.reserve(ADD_NUM_ELEMENTS);
        size_t i = 0;
        for (; i < ADD_NUM_DIFFERENT_ELEMENTS; ++i) {
            elements.push_back(GroupT::random_element());
        }
        for (; i < ADD_NUM_ELEMENTS; ++i) {
            elements.push_back(elements[i % ADD_NUM_DIFFERENT_ELEMENTS]);
        }
    }

    std::cout << "    num elements: " << std::to_string(ADD_NUM_ELEMENTS)
              << "\n";

    size_t num_elements = 0;
    GroupT accum = GroupT::zero();
    enter_block("group add operation profiling");
    for (const GroupT &el : elements) {
        accum = accum.add(el);
        num_elements++;
    }
    leave_block("group add operation profiling");

    if (num_elements != ADD_NUM_ELEMENTS) {
        throw std::runtime_error("invalid number of elements seen");
    }

    return true;
}

template<typename GroupT> bool profile_group_mixed_add()
{
    std::vector<GroupT> elements;
    {
        elements.reserve(ADD_NUM_ELEMENTS);
        size_t i = 0;
        for (; i < ADD_NUM_DIFFERENT_ELEMENTS; ++i) {
            GroupT e = GroupT::random_element();
            e.to_affine_coordinates();
            elements.push_back(e);
        }
        for (; i < ADD_NUM_ELEMENTS; ++i) {
            elements.push_back(elements[i % ADD_NUM_DIFFERENT_ELEMENTS]);
        }
    }

    std::cout << "    num elements: " << std::to_string(ADD_NUM_ELEMENTS)
              << "\n";

    size_t num_elements = 0;
    GroupT accum = GroupT::one();
    enter_block("group mixed add operation profiling");
    for (const GroupT &el : elements) {
        accum = accum.mixed_add(el);
        num_elements++;
    }
    leave_block("group mixed add operation profiling");

    if (num_elements != ADD_NUM_ELEMENTS) {
        throw std::runtime_error("invalid number of elements seen");
    }

    return true;
}

template<typename GroupT> bool profile_group_membership()
{
    static const size_t NUM_ELEMENTS = 1000;

    // Measure the time taken to check membership of 1000 elements. (Note all
    // elements are in fact members of the group - we are not testing
    // correctness here).

    std::vector<GroupT> elements;
    elements.reserve(NUM_ELEMENTS);
    for (size_t i = 0; i < NUM_ELEMENTS; ++i) {
        elements.push_back(GroupT::random_element());
    }

    enter_block("group membership profiling");
    for (const GroupT &el : elements) {
        if (!el.is_in_safe_subgroup()) {
            return false;
        }
    }
    leave_block("group membership profiling");

    return true;
}

template<typename ppT> bool profile_pairing_e_over_e()
{
    constexpr size_t NUM_ITERATIONS = ADD_NUM_DIFFERENT_ELEMENTS;

    // Mock BLS signature verification

    const Fr<ppT> sk = Fr<ppT>::random_element();
    G2<ppT> pk = sk * G2<ppT>::one();
    pk.to_affine_coordinates();

    // Messages to sign
    std::vector<Fr<ppT>> messages;
    messages.reserve(NUM_ITERATIONS);
    for (size_t i = 0; i < NUM_ITERATIONS; ++i) {
        messages.push_back(Fr<ppT>::random_element());
    }

    // Signatures. Every other one is invalid.
    std::vector<G1<ppT>> signatures;
    signatures.reserve(NUM_ITERATIONS);
    for (size_t i = 0; i < NUM_ITERATIONS; ++i) {
        if (i & 1) {
            signatures.push_back(sk * messages[i] * G1<ppT>::one());
        } else {
            signatures.push_back(messages[i] * G1<ppT>::one());
        }
        signatures.back().to_affine_coordinates();
    }

    // Precompute the parts we expect to be known up-front.
    const G2_precomp<ppT> id_precomp = ppT::precompute_G2(-G2<ppT>::one());
    const G2_precomp<ppT> pk_precomp = ppT::precompute_G2(pk);

    enter_block("pairing_e_over_e");

    // NOTE: the pairing implementations call enter_block and leave_block in
    // several places, generating a lot of output, which slows down the
    // performance a lot. Hence we suppress the profiling here, and re-enable
    // it before the end of the block.

    libff::inhibit_profiling_info = true;
    libff::inhibit_profiling_counters = true;

    for (size_t i = 0; i < NUM_ITERATIONS; ++i) {
        const G1_precomp<ppT> sig_prec = ppT::precompute_G1(signatures[i]);
        const G1_precomp<ppT> H_precomp =
            ppT::precompute_G1(messages[i] * G1<ppT>::one());

        const Fqk<ppT> result =
            ppT::final_exponentiation(ppT::double_miller_loop(
                sig_prec, id_precomp, H_precomp, pk_precomp));

        if (i & 1) {
            if (result != Fqk<ppT>::one()) {
                throw std::runtime_error(
                    "signature verification failed " + std::to_string(i));
            }
        } else {
            if (result == Fqk<ppT>::one()) {
                throw std::runtime_error(
                    "signature verification passed (should fail)");
            }
        }
    }

    libff::inhibit_profiling_info = false;
    libff::inhibit_profiling_counters = false;

    leave_block("pairing_e_over_e");

    return true;
}

int main(void)
{
    std::cout << "alt_bn128_pp\n";
    alt_bn128_pp::init_public_params();

    std::cout << "  profile_group_add<alt_bn128_G1>:\n";
    if (!profile_group_add<alt_bn128_G1>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_group_mixed_add<alt_bn128_G1>:\n";
    if (!profile_group_mixed_add<alt_bn128_G1>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_group_add<alt_bn128_G2>:\n";
    if (!profile_group_add<alt_bn128_G2>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_group_mixed_add<alt_bn128_G2>:\n";
    if (!profile_group_mixed_add<alt_bn128_G2>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_affine_e_over_e<alt_bn128>:\n";
    profile_pairing_e_over_e<alt_bn128_pp>();

    std::cout << "bls12_377_pp\n";
    bls12_377_pp::init_public_params();

    std::cout << "  profile_group_add<bls12_377_G1>:\n";
    if (!profile_group_add<bls12_377_G1>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_group_mixed_add<bls12_377_G1>:\n";
    if (!profile_group_mixed_add<bls12_377_G1>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_group_add<bls12_377_G2>:\n";
    if (!profile_group_add<bls12_377_G2>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_group_membership<bls12_377_G1>:\n";
    if (!profile_group_membership<bls12_377_G1>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_group_membership<bls12_377_G2>:\n";
    if (!profile_group_membership<bls12_377_G2>()) {
        throw std::runtime_error("failed");
    }

    std::cout << "  profile_affine_e_over_e<bls12_377>:\n";
    profile_pairing_e_over_e<bls12_377_pp>();

    return 0;
}
