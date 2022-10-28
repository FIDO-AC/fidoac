#ifndef LIBSNARK_SHA256_FIDO_AC_TRUSTED_SETUP_HPP
#define LIBSNARK_SHA256_FIDO_AC_TRUSTED_SETUP_HPP

#include <libsnark-sha256/gadgets/fido_ac_gadget.hpp>

typedef libff::Fr<libff::default_ec_pp> FieldT;

const r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp> trusted_setup(
        size_t dg1_length,
        int reference_year,
        int current_year);

#endif //LIBSNARK_SHA256_FIDO_AC_TRUSTED_SETUP_HPP
