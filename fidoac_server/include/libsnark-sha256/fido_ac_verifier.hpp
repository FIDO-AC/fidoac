#ifndef LIBSNARK_SHA256_FIDO_AC_VERIFIER_HPP
#define LIBSNARK_SHA256_FIDO_AC_VERIFIER_HPP

#include <libsnark/common/default_types/r1cs_ppzksnark_pp.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/r1cs_ppzksnark.hpp>
#include "fido_ac_proof.hpp"

using namespace libsnark;

bool verify(fido_ac_proof proof,
            const r1cs_ppzksnark_verification_key<default_r1cs_ppzksnark_pp> &vk);

#endif //LIBSNARK_SHA256_FIDO_AC_VERIFIER_HPP
