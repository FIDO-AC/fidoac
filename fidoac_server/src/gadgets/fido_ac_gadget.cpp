#include "libsnark-sha256/gadgets/fido_ac_gadget.hpp"

void fido_ac_gadget::feed_dg1(unsigned char *dg1, size_t dg1_length, unsigned char* client_nonce, size_t client_nonce_length) {

    data_blocks = sha256_93B_gadget::feed_dg1(this->pb, dg1, dg1_length, *ZERO);
    client_nonce_blocks = sha256_93B_gadget::feed_client_nonce(this->pb, client_nonce, client_nonce_length, *ZERO);

    sha256_gd.reset(new sha256_93B_gadget(
            this->pb,
            data_blocks.at(0),
            data_blocks.at(1),
            *ZERO,
            hash_digest,
            client_nonce_blocks.at(0),
            "sha256_93B_gadget"
            ));

    age_verification_gd.reset(new age_verification_gadget(
            this->pb,
            data_blocks,
            *year_now,
            *year_ref,
            *valid,
            "age_verification_gadget"
            ));
}

void fido_ac_gadget::generate_r1cs_constraints() {
    generate_r1cs_equals_const_constraint(pb, pb_linear_combination<FieldT>(*ZERO), FieldT::zero(), "ZERO");
    sha256_gd->generate_r1cs_constraints();
    age_verification_gd->generate_r1cs_constraints();
}

void fido_ac_gadget::generate_r1cs_witness() {
    pb.val(*ZERO) = 0;
    sha256_gd->generate_r1cs_witness();
    age_verification_gd->generate_r1cs_witness();
}