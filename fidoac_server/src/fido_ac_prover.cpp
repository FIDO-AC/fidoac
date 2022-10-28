#include "libsnark-sha256/fido_ac_prover.hpp"

/**
 * \brief Proves that the input DG1 hashes to a specific digest and that the year from the date of birth is >= a given
 * reference year.
 *
 * @param in The raw DG1 read from the passport.
 * @param in_length The length of DG1 - should always be 93!
 * @param reference_year The year to compare the birthdate year against. MUST BE THE SAME AS IN TRUSTED SETUP.
 * @param current_year The LAST TWO DIGITS of the current year. This is used to decide if a birth year in the DG1 is
 * from the 20th or 21th century as only the last two digits are encoded. MUST BE THE SAME AS IN TRUSTED SETUP.
 * @param pk The proving key from the keypair/crs
 * @return A struct containing the proof and the primary input.
 */
const fido_ac_proof prove(
        unsigned char * in,
        size_t in_length,
        int reference_year,
        int current_year,
        const r1cs_ppzksnark_proving_key<default_r1cs_ppzksnark_pp>& pk){

    assert(in_length == 93);

    default_ec_pp::init_public_params();

    // prepare input and out variables
    shared_ptr<protoboard<FieldT>> pb;
    pb.reset(new protoboard<FieldT>());

    shared_ptr<pb_variable<FieldT>> year_ref, valid, year_now;
    shared_ptr<digest_variable<FieldT>> hash_digest;

    // allocate primary input
    year_ref.reset(new pb_variable<FieldT>());
    year_ref->allocate(*pb, "year_ref");
    pb->val(*year_ref) = reference_year;

    valid.reset(new pb_variable<FieldT>());
    valid->allocate(*pb, "valid");

    year_now.reset(new pb_variable<FieldT>());
    year_now->allocate(*pb, "year_now");
    pb->val(*year_now) = current_year;

    hash_digest.reset(new digest_variable<FieldT>(*pb, SHA256_digest_size, "hash_digest"));

    shared_ptr<fido_ac_gadget> fido_ac;
    fido_ac.reset(new fido_ac_gadget(
            *pb,
            year_ref,
            valid,
            hash_digest,
            year_now,
            in_length,
            "fido_ac_gadget"
    ));

    fido_ac->feed_dg1(in, in_length);

    fido_ac->generate_r1cs_constraints();
    fido_ac->generate_r1cs_witness();

    // primary input is year_ref, year_now, valid and the digest
    pb->set_input_sizes(SHA256_digest_size + 3);

    const r1cs_ppzksnark_proof<default_r1cs_ppzksnark_pp> proof = r1cs_ppzksnark_prover<default_r1cs_ppzksnark_pp>(
            pk, pb->primary_input(), pb->auxiliary_input());

    return fido_ac_proof(proof, pb->primary_input());

}
