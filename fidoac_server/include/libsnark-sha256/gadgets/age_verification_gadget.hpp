#ifndef LIBSNARK_SHA256_AGE_VERIFICATION_GADGET_HPP
#define LIBSNARK_SHA256_AGE_VERIFICATION_GADGET_HPP

#include <libsnark/gadgetlib1/gadget.hpp>
#include <libsnark/gadgetlib1/protoboard.hpp>
#include <libsnark/gadgetlib1/gadgets/basic_gadgets.hpp>
#include <libsnark/common/data_structures/merkle_tree.hpp>
#include <libsnark/gadgetlib1/gadgets/hashes/hash_io.hpp>
#include <libsnark/gadgetlib1/gadgets/hashes/sha256/sha256_components.hpp>
#include <libsnark/gadgetlib1/gadgets/hashes/sha256/sha256_gadget.hpp>
#include <libsnark/common/default_types/r1cs_ppzksnark_pp.hpp>
#include <libsnark/zk_proof_systems/ppzksnark/r1cs_ppzksnark/r1cs_ppzksnark.hpp>
#include <libsnark/gadgetlib1/pb_variable.hpp>

#include <libff/common/default_types/ec_pp.hpp>

using namespace libsnark;
using namespace libff;
using namespace std;

typedef libff::Fr<libff::default_ec_pp> FieldT;

class age_verification_gadget : gadget<FieldT> {
private:
    pb_variable<FieldT> valid;
    pb_variable<FieldT> year_ref;
    pb_variable<FieldT> year_ut;
    pb_variable<FieldT> year_ut_full;
    vector<pb_variable_array<FieldT>> dg1;
    pb_variable<FieldT> year_now;
    pb_variable<FieldT> year_now_negative;
    pb_variable_array<FieldT> year_ut_var_arr;
    pb_variable_array<FieldT> year_ut_lc_var_arr;
    pb_variable_array<FieldT> year_ut_packed_var_arr;
    shared_ptr<multipacking_gadget<FieldT>> multi_gadget;
    shared_ptr<comparison_gadget<FieldT>> comparison_gadget_century;
    pb_variable<FieldT> year_ut_negative;
    pb_variable<FieldT> year_ut_full_negative;
    pb_variable<FieldT> comparison_gadget_century_less;
    pb_variable<FieldT> comparison_gadget_century_less_or_eq;
    pb_variable<FieldT> comparison_gadget_century_more;
    pb_variable<FieldT> nineteenhundred;
    pb_variable<FieldT> nineteenhundred_mult;
    pb_variable<FieldT> twothousand;
    pb_variable<FieldT> twothousand_mult;
    pb_variable<FieldT> zero;
    pb_variable<FieldT> year_ref_negative;
    shared_ptr<comparison_gadget<FieldT>> comparison_gadget_year;
    pb_linear_combination<FieldT> year_ut_lc;
    pb_linear_combination<FieldT> year_ref_lc;
    pb_variable<FieldT> comparison_gadget_year_less;
    pb_variable<FieldT> comparison_gadget_year_less_or_eq;

    static void char_block_to_libff_bit_vector(unsigned char *in, size_t in_length, libff::bit_vector &out);
    static void char_to_libff_bit_vector(protoboard<FieldT> &pb, unsigned char *in, size_t in_length, int block_count, vector<libff::bit_vector> &out);
    static void fill_input_var_array_from_char(protoboard<FieldT> &pb, unsigned char *in, size_t in_length,
                                        vector<pb_variable_array<FieldT>> &input_blocks);



public:
    age_verification_gadget(
            protoboard<FieldT> &pb,
            vector<pb_variable_array<FieldT>> &dg1,
            pb_variable<FieldT> &year_now,
            pb_variable<FieldT> &year_ref,
            pb_variable<FieldT> &valid,
            const std::string &annotation_prefix = "")
            : gadget<FieldT>(pb, annotation_prefix),
            dg1(dg1),
            year_now(year_now),
            year_ref(year_ref),
            valid(valid)
            {
        // tested input ranges for comparison gadgets - see test_comparison_gadget.cpp
        assert(22 <= pb.val(year_now).as_ulong() < 32);
        assert(pb.val(year_now).as_ulong() + 2000 - 100
        <= pb.val(year_ref).as_ulong() <
        pb.val(year_now).as_ulong() + 2000);

        // allocate auxiliary input
        year_ut.allocate(pb, "year_ut");
        year_now_negative.allocate(pb, "year_now_negative");
        year_ut_var_arr.allocate(pb, CHAR_BIT * 2, "year_ut_var_arr");
        year_ut_lc_var_arr.allocate(pb, 2, "year_ut_lc_var_arr");
        year_ut_packed_var_arr.allocate(pb, 2, "yeat_ut_packed_var_arr");
        year_ut_negative.allocate(pb, "year_ut_negative");
        year_ut_full_negative.allocate(pb, "year_ut_full_negative");
        year_ut_full.allocate(pb, "year_ut_full");
        comparison_gadget_century_less.allocate(pb, "comparison_gadget_century_less");
        comparison_gadget_century_less_or_eq.allocate(pb, "comparison_gadget_century_less_or_eq");
        comparison_gadget_century_more.allocate(pb, "comparison_gadget_century_more");
        nineteenhundred.allocate(pb, "nineteenhundred");
        pb.val(nineteenhundred) = 1900;
        nineteenhundred_mult.allocate(pb, "nineteenhundred_mult");
        twothousand.allocate(pb, "twothousand");
        pb.val(twothousand) = 2000;
        twothousand_mult.allocate(pb, "twothousand_mult");
        zero.allocate(pb, "zero");
        pb.val(zero) = 0;
        year_ref_negative.allocate(pb, "year_ref_negative");
        comparison_gadget_year_less.allocate(pb, "comparison_gadget_year_less");
        comparison_gadget_year_less_or_eq.allocate(pb, "comparison_gadget_year_less_or_eq");
    };
    static void feed_dg1(protoboard<FieldT> &pb, unsigned char *dg1, size_t dg1_length, vector<pb_variable_array<FieldT>> &out);
    void generate_r1cs_constraints();
    void generate_r1cs_witness();
    void print_vars();
};

#endif //LIBSNARK_SHA256_AGE_VERIFICATION_GADGET_HPP
