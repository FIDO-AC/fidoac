#ifndef LIBSNARK_SHA256_FIDO_AC_PROVER_HPP
#define LIBSNARK_SHA256_FIDO_AC_PROVER_HPP

#include <libsnark-sha256/gadgets/fido_ac_gadget.hpp>
#include "fido_ac_proof.hpp"

using namespace libsnark;

typedef libff::Fr<libff::default_ec_pp> FieldT;

const fido_ac_proof prove(
        unsigned char * in,
        size_t in_length,
        int reference_year,
        int current_year,
        const r1cs_ppzksnark_proving_key<default_r1cs_ppzksnark_pp>& pk);

#endif //LIBSNARK_SHA256_FIDO_AC_PROVER_HPP
