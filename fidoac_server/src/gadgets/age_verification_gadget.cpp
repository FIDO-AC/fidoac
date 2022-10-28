#include "libsnark-sha256/gadgets/age_verification_gadget.hpp"

void age_verification_gadget::feed_dg1(protoboard<FieldT> &pb, unsigned char *dg1, size_t dg1_length, vector<pb_variable_array<FieldT>> &out) {
    fill_input_var_array_from_char(pb, dg1, dg1_length, out);

}

void age_verification_gadget::generate_r1cs_constraints() {
    // constants
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(ONE, 1900, nineteenhundred));
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(ONE, 2000, twothousand));
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(ONE, 0, zero));

    // invert year_now for comparison later
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_now - (year_now * 2), 1, year_now_negative));

    // new vars pointing to input offset in dg1
    size_t DECADE_OFFSET = 0x3e * CHAR_BIT;
    for(int i = 0; i < CHAR_BIT * 2; i++){
        this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(dg1.at(0)[DECADE_OFFSET + i], 1, year_ut_var_arr[i]));
    }

    // multipack bits
    multi_gadget.reset(new multipacking_gadget<FieldT>(
            this->pb,
            pb_linear_combination_array<FieldT>(year_ut_var_arr),
            pb_linear_combination_array<FieldT>(year_ut_lc_var_arr),
            CHAR_BIT,
            "multipacker"
            ));
    multi_gadget->generate_r1cs_constraints(true);


    // subtract ascii offset
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_ut_lc_var_arr[0] - 48, 1, year_ut_packed_var_arr[0]));
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_ut_lc_var_arr[1] - 48, 1, year_ut_packed_var_arr[1]));

    // pack ints
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_ut_packed_var_arr[0] * 10 + year_ut_packed_var_arr[1], 1, year_ut));

    // compare number to detect if pre 2000 or post 2000 since only e.g. 95 or 02 is in the dg
    // comparison gadget seems to only be able to compare negative to positive values or negative to negative reliably
    // TODO what is parameter n exactly?
    // TODO set to 2, add assertion that values cannot exceed size
    // invert sign of years
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_ut - year_ut * 2, 1, year_ut_negative));
    comparison_gadget_century.reset(new comparison_gadget<FieldT>(
            this->pb,
            16,
            pb_linear_combination<FieldT>(year_now_negative),
            pb_linear_combination<FieldT>(year_ut_negative),
            comparison_gadget_century_less,
            comparison_gadget_century_less_or_eq,
            "comparison_gadget_century"
            ));
    comparison_gadget_century->generate_r1cs_constraints();
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(comparison_gadget_century_less_or_eq - 1, - 1, comparison_gadget_century_more));

    // add 1900 or 2000 depending on result of comparison
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(nineteenhundred, comparison_gadget_century_more, nineteenhundred_mult));
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(twothousand, comparison_gadget_century_less_or_eq, twothousand_mult));
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_ut + (nineteenhundred_mult + twothousand_mult), 1, year_ut_full));

    // invert values
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_ut_full - (year_ut_full * 2), 1, year_ut_full_negative));
    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_ref - (year_ref * 2), 1, year_ref_negative));

    // compare years
    comparison_gadget_year.reset(new comparison_gadget<FieldT>(
            this->pb,
            16,
            pb_linear_combination<FieldT>(year_ref_negative),
            pb_linear_combination<FieldT>(year_ut_full_negative),
            valid,
            comparison_gadget_year_less_or_eq,
            "comparison_gadget_year"
            ));
    comparison_gadget_year->generate_r1cs_constraints();
}

void age_verification_gadget::generate_r1cs_witness() {
    // invert year_now for comparison later
    this->pb.val(year_now_negative) = this->pb.val(year_now) - this->pb.val(year_now) * 2;

    this->pb.add_r1cs_constraint(r1cs_constraint<FieldT>(year_now - (year_now * 2), 1, year_now_negative));
    // new vars pointing to input offset in dg1
    size_t DECADE_OFFSET = 0x3e * CHAR_BIT;
    for(int i = 0; i < CHAR_BIT * 2; i++){
        this->pb.val(year_ut_var_arr[i]) = this->pb.val(dg1.at(0)[DECADE_OFFSET + i]);
    }

    // multipack bits
    multi_gadget->generate_r1cs_witness_from_bits();

    // subtract ascii offset
    this->pb.val(year_ut_packed_var_arr[0]) = year_ut_lc_var_arr.get_vals(this->pb)[0] - 48;
    this->pb.val(year_ut_packed_var_arr[1]) = year_ut_lc_var_arr.get_vals(this->pb)[1] - 48;

    // pack ints
    this->pb.val(year_ut) = (this->pb.val(year_ut_packed_var_arr[0]) * 10) + this->pb.val(year_ut_packed_var_arr[1]);

    // compare number to detect if pre 2000 or post 2000 since only e.g. 95 or 02 is in the dg
    // invert sign of years
    this->pb.val(year_ut_negative) = this->pb.val(year_ut) - this->pb.val(year_ut) * 2;
    comparison_gadget_century->generate_r1cs_witness();
    this->pb.val(comparison_gadget_century_more) = (this->pb.val(comparison_gadget_century_less_or_eq) - 1) * - 1;

    // add 1900 or 2000 depending on result of comparison
    this->pb.val(nineteenhundred_mult) = this->pb.val(nineteenhundred) * this->pb.val(comparison_gadget_century_more);
    this->pb.val(twothousand_mult) = this->pb.val(twothousand) * this->pb.val(comparison_gadget_century_less_or_eq);
    this->pb.val(year_ut_full) = this->pb.val(year_ut) + (this->pb.val(nineteenhundred_mult) + this->pb.val(twothousand_mult));
    assert(this->pb.val(year_now).as_ulong() + 2000 - 100
           <= this->pb.val(year_ut_full).as_ulong() <
           this->pb.val(year_now).as_ulong() + 2000);

    // invert values
    this->pb.val(year_ut_full_negative) = this->pb.val(year_ut_full) - (this->pb.val(year_ut_full) * 2);
    this->pb.val(year_ref_negative) = this->pb.val(year_ref) - (this->pb.val(year_ref) * 2);

    // compare year
    comparison_gadget_year->generate_r1cs_witness();
}

void age_verification_gadget::print_vars() {
    cout << "year_ref\n";
    this->pb.val(year_ref).print();
    //cout << "dg1\n";
    // for (int i = 0; i < dg1.at(0).size(); i++){
    //     this->pb.val(dg1.at(0)[i]).print();
    // }
    cout << "year_ut_var_arr\n";
    for (int i = 0; i < year_ut_var_arr.size(); i++){
        year_ut_var_arr.get_vals(this->pb)[i].print();
    }
    cout << "year_ut_lc_var_arr\n";
    this->pb.val(year_ut_lc_var_arr[0]).print();
    this->pb.val(year_ut_lc_var_arr[1]).print();
    cout << "year_ut_packed_var_arr\n";
    this->pb.val(year_ut_packed_var_arr[0]).print();
    this->pb.val(year_ut_packed_var_arr[1]).print();
    cout << "comparison_gadget_century_less\n";
    this->pb.val(comparison_gadget_century_less).print();
    cout << "comparison_gadget_century_less_or_eq\n";
    this->pb.val(comparison_gadget_century_less_or_eq).print();
    cout << "comparison_gadget_century_more\n";
    this->pb.val(comparison_gadget_century_more).print();
    cout << "nineteenhundred_mult\n";
    this->pb.val(nineteenhundred_mult).print();
    cout << "twothousand_mult\n";
    this->pb.val(twothousand_mult).print();
    cout << "year_ut\n";
    this->pb.val(year_ut).print();
    cout << "year_ut_full\n";
    this->pb.val(year_ut_full).print();
    cout << "comparison_gadget_year_less_or_eq\n";
    this->pb.val(comparison_gadget_year_less_or_eq).print();
    cout << "valid\n";
    this->pb.val(valid).print();
}

/* private functions */
void age_verification_gadget::char_block_to_libff_bit_vector(unsigned char *in, size_t in_length, libff::bit_vector &out){
    // iterate over input block
    for (int i = 0; i < in_length; i++){
        // iterate over byte
        for (int j = 0; j < CHAR_BIT; j++){
            // add bit to libff::bitvector
            out.push_back((in[i] >> j) & 1);
        }
    }
    // if we encounter a partial last block, fill vector with zeroes
    if (in_length < 64){
        for(int i = out.size(); i < SHA256_block_size; i++){
            out.push_back(false);
        }
    }
}

void age_verification_gadget::char_to_libff_bit_vector(protoboard<FieldT> &pb, unsigned char *in, size_t in_length, int block_count, vector<libff::bit_vector> &out){
    // iterate over blocks
    for (int i = 0; i < block_count; i++){
        // create new libff::vector to hold block content
        libff::bit_vector bv;
        // calculate block length
        int block_length;
        // if block fills the full 64 use as length, otherwise calculate partial block size
        if ((i + 1) * 64 <= in_length){
            block_length = 64;
        } else {
            block_length = in_length - i * 64;
        }
        // convert char block to libff::vector
        char_block_to_libff_bit_vector(in + i * 64, block_length, bv);
        // add vector/block to output
        out.push_back(bv);
    }
}

void age_verification_gadget::fill_input_var_array_from_char(protoboard<FieldT> &pb, unsigned char *in, size_t in_length,
                                                             vector<pb_variable_array<FieldT>> &input_blocks) {
    // create new vector of libff::bit_vectors
    vector<libff::bit_vector> block_vector;
    // convert input to libff::bit_vectors
    int block_count = (in_length * CHAR_BIT + SHA256_block_size - 1) / SHA256_block_size;
    char_to_libff_bit_vector(pb, in, in_length, block_count, block_vector);
    // iterate of input_blocks varriable array vector
    for (int i = 0; i < block_count; i++){
        // if we still have input add it
        if (i < block_vector.size()) {
            pb_variable_array<FieldT> tmp;
            tmp.allocate(pb, SHA256_block_size, "tmp" + to_string(i));
            input_blocks.push_back(tmp);
            input_blocks.at(i).fill_with_bits(pb, block_vector[i]);
        }
    }
}