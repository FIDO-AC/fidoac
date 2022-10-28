#include "libsnark-sha256/gadgets/sha256_93B_gadget.hpp"


 vector<pb_variable_array<FieldT>> sha256_93B_gadget::feed_dg1(protoboard<FieldT> &pb,
                                                               unsigned char *dg1,
                                                               size_t dg1_length,
                                                               pb_variable<FieldT> &ZERO){
    assert(dg1_length == 93);
    vector<pb_variable_array<FieldT>> out;

    out.push_back(from_bits(pb, from_char_array(dg1, 64), ZERO));
    out.push_back(from_bits(pb, from_char_array(dg1 + 64, 29), ZERO));

    return out;
}

void sha256_93B_gadget::generate_r1cs_constraints(){
    hasher1->generate_r1cs_constraints();
    hasher2->generate_r1cs_constraints();
}

void sha256_93B_gadget::generate_r1cs_witness(){
    hasher1->generate_r1cs_witness();
    hasher2->generate_r1cs_witness();
}

pb_variable_array<FieldT> sha256_93B_gadget::from_bits(protoboard<FieldT> &pb, std::vector<bool> bits, pb_variable<FieldT>& ZERO){
    pb_variable_array<FieldT> acc;
    acc.allocate(pb, bits.size(), "");
    acc.fill_with_bits(pb, bits);

    // TODO need to allocate variable array?
    // no since we use allocated variables?
    //for (size_t i = 0; i < bits.size(); i++) {
    //    bool bit = bits[i];
    //    acc.emplace_back(bit ? ONE : ZERO);
    //}

    return acc;
}

 vector<bool> sha256_93B_gadget::from_char(unsigned char in){
    vector<bool> bits;
    for (int j = CHAR_BIT - 1; j >= 0; j--){
        // add bit to libff::bitvector
        bits.push_back((in >> j) & 1);
    }
    return bits;
}

 vector<bool> sha256_93B_gadget::from_char_array(unsigned char *in, size_t in_length){
    vector<bool> bits;
    for (int i = 0; i < in_length; i++){
        vector<bool> byte = from_char(in[i]);
        bits.insert(bits.end(), byte.begin(), byte.end());
    }
    return bits;
}

unsigned char * sha256_93B_gadget::get_digest(shared_ptr<digest_variable<FieldT>> result){
    // set digest
    unsigned char * digest = new unsigned char[SHA256_digest_size/CHAR_BIT];
    bit_vector_to_char_array(result->get_digest(), digest, 32);

    // output digest
    for (int i = 0; i < 32; i++) {
        printf("%02x", digest[i]);
    }
    cout << endl;
    return digest;
}

void sha256_93B_gadget::bit_vector_to_char_array(vector<bool> in, unsigned char *out, size_t out_length){
    // iterate over output char array
    for (int i = 0; i < out_length; i++){
        out[i] = 0x00;
        out[i] |= in.at(i * CHAR_BIT + 0) << 7;
        out[i] |= in.at(i * CHAR_BIT + 1) << 6;
        out[i] |= in.at(i * CHAR_BIT + 2) << 5;
        out[i] |= in.at(i * CHAR_BIT + 3) << 4;
        out[i] |= in.at(i * CHAR_BIT + 4) << 3;
        out[i] |= in.at(i * CHAR_BIT + 5) << 2;
        out[i] |= in.at(i * CHAR_BIT + 6) << 1;
        out[i] |= in.at(i * CHAR_BIT + 7) << 0;
    }
}
