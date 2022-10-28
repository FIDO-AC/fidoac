#ifndef LIBSNARK_SHA256_FIDO_AC_GADGET_HPP
#define LIBSNARK_SHA256_FIDO_AC_GADGET_HPP

#include "sha256_93B_gadget.hpp"
#include "age_verification_gadget.hpp"

using namespace libsnark;
using namespace libff;
using namespace std;

typedef libff::Fr<libff::default_ec_pp> FieldT;

class fido_ac_gadget : gadget<FieldT> {
private:
    shared_ptr<pb_variable<FieldT>> year_ref;
    shared_ptr<pb_variable<FieldT>> valid;
    shared_ptr<digest_variable<FieldT>> hash_digest;

    size_t max_input_length;
    shared_ptr<pb_variable<FieldT>> year_now;

    vector<pb_variable_array<FieldT>> data_blocks;
    shared_ptr<pb_variable<FieldT>> ZERO;
    shared_ptr<sha256_93B_gadget> sha256_gd;
    shared_ptr<age_verification_gadget> age_verification_gd;


public:
    fido_ac_gadget(
            protoboard<FieldT> &pb,
            shared_ptr<pb_variable<FieldT>> year_ref,
            shared_ptr<pb_variable<FieldT>> valid,
            shared_ptr<digest_variable<FieldT>> hash_digest,
            shared_ptr<pb_variable<FieldT>> year_now,
            size_t max_input_length,
            const std::string &annotation_prefix = "")
            : gadget<FieldT>(pb, annotation_prefix),
            year_ref(year_ref),
            valid(valid),
            hash_digest(hash_digest),
            year_now(year_now),
            max_input_length(max_input_length)
    {
        // allocate auxiliary input
        ZERO.reset(new pb_variable<FieldT>());
        ZERO->allocate(pb, "ZERO");
    }

    void feed_dg1(unsigned char *dg1, size_t dg1_length);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();

};

#endif //LIBSNARK_SHA256_FIDO_AC_GADGET_HPP
