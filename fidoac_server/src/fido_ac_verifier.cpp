#include "../include/libsnark-sha256/fido_ac_verifier.hpp"

/**
 * \brief Verifies the proof against the primary input.
 * @param proof The proof from the fido_ac_prover.
 * @param vk The verification key from the keypair/crs
 * @return 1 iff the proof is valid.
 */
bool verify(fido_ac_proof proof,
            const r1cs_ppzksnark_verification_key<default_r1cs_ppzksnark_pp> &vk){
    return r1cs_ppzksnark_verifier_strong_IC<default_r1cs_ppzksnark_pp>(vk, proof.get_primary_input(), proof.get_proof());
}
