#include "libsnark-sha256/fido_ac_trusted_setup.hpp"

// generate key pair from constraint system
const r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp> trusted_setup(
        const r1cs_constraint_system<FieldT> constraint_system){
    return r1cs_ppzksnark_generator<default_r1cs_ppzksnark_pp>(constraint_system);
}

/**
 * \brief Creates a keypair/crs, that can be used for proving and verifying respectively, assuming the same parameters
 * are used.
 *
 * @param dg1_length The length of DG1 - should always be 93!
 * @param reference_year The year to compare the birthdate year against. MUST BE THE SAME AS IN TRUSTED SETUP.
 * @param current_year The LAST TWO DIGITS of the current year. This is used to decide if a birth year in the DG1 is
 * @return A keypair/crs.
 */
// get constraint system size by creating mock input of set size
const r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp> trusted_setup(
        size_t dg1_length,
        int reference_year,
        int current_year){

    assert(dg1_length == 93);

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

    // prepare mock input
    unsigned char * mock_input = new unsigned char[dg1_length];
    memset(mock_input, 0x00, dg1_length);
    // set year digits in mock input to something reasonable
    mock_input[0x3e] = 0 + 48;
    mock_input[0x3f] = 1 + 48;

    shared_ptr<fido_ac_gadget> fido_ac;
    fido_ac.reset(new fido_ac_gadget(
            *pb,
            year_ref,
            valid,
            hash_digest,
            year_now,
            dg1_length,
            "fido_ac_gadget"
            ));

    fido_ac->feed_dg1(mock_input, dg1_length, mock_input, 16);

    fido_ac->generate_r1cs_constraints();
    // TODO remove gen witness
    fido_ac->generate_r1cs_witness();

    // primary input is year_ref, year_now, valid and the digest
    pb->set_input_sizes(SHA256_digest_size + 3);

    assert(pb->is_satisfied());

    delete[] mock_input;

    const r1cs_constraint_system<FieldT> constraint_system = pb->get_constraint_system();
    return trusted_setup(constraint_system);
}
