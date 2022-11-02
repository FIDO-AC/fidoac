#ifndef LIBSNARK_SHA256_FIDO_AC_PROOF_HPP
#define LIBSNARK_SHA256_FIDO_AC_PROOF_HPP

#include <libsnark/common/default_types/r1cs_ppzksnark_pp.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/r1cs_ppzksnark.hpp>

using namespace libsnark;

typedef libff::Fr<libff::default_ec_pp> FieldT;

/**
 * Struct containing the proof and the protoboards primary input.
 * Both of which is needed to verify it.
 * Note that the primary input contains following data:
 * year_ref || valid || year_now || digest (in bits)
 * So the refernce year the age_verification_gadget compared against, the result of said comparison, the current year
 * used to determine the birthyear`s century and the calculated digest of the DG1.
 */
class fido_ac_proof{
public:
    r1cs_ppzksnark_proof<default_r1cs_ppzksnark_pp> proof;
    r1cs_ppzksnark_primary_input<default_r1cs_ppzksnark_pp> primary_input;

    fido_ac_proof() {}

    fido_ac_proof(
            const r1cs_ppzksnark_proof<default_r1cs_ppzksnark_pp> proof,
            const r1cs_ppzksnark_primary_input<default_r1cs_ppzksnark_pp> primary_input
    ) : proof(proof), primary_input(primary_input) {}

    fido_ac_proof(
            const r1cs_ppzksnark_primary_input<default_r1cs_ppzksnark_pp> primary_input
    ) : primary_input(primary_input) {}

    const r1cs_ppzksnark_proof<default_r1cs_ppzksnark_pp> get_proof(){
        return this->proof;
    }

    const r1cs_ppzksnark_primary_input<default_r1cs_ppzksnark_pp> get_primary_input(){
        return this->primary_input;
    }
};

#endif //LIBSNARK_SHA256_FIDO_AC_PROOF_HPP
