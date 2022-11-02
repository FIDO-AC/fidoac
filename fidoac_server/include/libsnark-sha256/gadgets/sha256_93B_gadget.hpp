#ifndef LIBSNARK_SHA256_SHA256_93B_GADGET_HPP
#define LIBSNARK_SHA256_SHA256_93B_GADGET_HPP

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

class sha256_93B_gadget : gadget<FieldT> {
private:
    shared_ptr<block_variable<FieldT>> block1;
    shared_ptr<block_variable<FieldT>> block2;
    shared_ptr<block_variable<FieldT>> block3; //Intermediate output of inner hash
    shared_ptr<block_variable<FieldT>> block4; //Nonce + padding

    shared_ptr<sha256_compression_function_gadget<FieldT>> hasher1;
    shared_ptr<digest_variable<FieldT>> intermediate_hash1;
    shared_ptr<sha256_compression_function_gadget<FieldT>> hasher2;
    shared_ptr<digest_variable<FieldT>> intermediate_hash2;

    shared_ptr<sha256_compression_function_gadget<FieldT>> hasher3;
    shared_ptr<digest_variable<FieldT>> intermediate_hash3;
//    shared_ptr<sha256_compression_function_gadget<FieldT>> hasher4;
//    shared_ptr<digest_variable<FieldT>> intermediate_hash4;

public:
    sha256_93B_gadget(
            protoboard<FieldT> &pb,
            pb_variable_array<FieldT> &data1,
            pb_variable_array<FieldT> &data2,
            pb_variable<FieldT> &ZERO,
            shared_ptr<digest_variable<FieldT>> result,
            pb_variable_array<FieldT> &client_nonce_data,
            const std::string &annotation_prefix = ""
    )
    : gadget<FieldT>(pb, annotation_prefix)
    {
        block1.reset(new block_variable<FieldT>(pb, {
                data1,
        }, "block1"));


        // final padding
        pb_variable_array<FieldT> padding =
                from_bits(this->pb,{
                                  // 216 bits padding
                                  1,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,

                                  // encoded length of message 93 Bytes = 744 bits
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,0,0,
                                  0,0,0,0,0,0,1,0,
                                  1,1,1,0,1,0,0,0
                          }, ZERO);

        pb_variable_array<FieldT> padding_256 =
                from_bits(this->pb,{
                        // 192-128 = 64, or 8 lines (of padding)
                        1,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,

                        // encoded length of message (in 64 bits) = 256 bits + 128 = 384 bits
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,0,
                        0,0,0,0,0,0,0,1,
                        1,0,0,0,0,0,0,0
                }, ZERO);

        block2.reset(new block_variable<FieldT>(pb, {
                data2,
                padding
        }, "block2"));

        pb_linear_combination_array<FieldT> IV = SHA256_default_IV(pb);

        intermediate_hash1.reset(new digest_variable<FieldT>(pb, 256, "intermediate_hash1"));

        hasher1.reset(new sha256_compression_function_gadget<FieldT>(
                pb,
                IV,
                block1->bits,
                *intermediate_hash1,
                "hasher1"));

        pb_linear_combination_array<FieldT> IV2(intermediate_hash1->bits);
        intermediate_hash2.reset(new digest_variable<FieldT>(pb, 256, "intermediate_hash2"));

        hasher2.reset(new sha256_compression_function_gadget<FieldT>(
                pb,
                IV2,
                block2->bits,
                *intermediate_hash2,
                "hasher2"));

        //New Hash chain
        //intermediate_hash3.reset(new digest_variable<FieldT>(pb, 256, "intermediate_hash3"));
        block3.reset(new block_variable<FieldT>(pb, {
                intermediate_hash2->bits,
                client_nonce_data,
                padding_256
        }, "block3"));
        hasher3.reset(new sha256_compression_function_gadget<FieldT>(
                pb,
                IV,
                block3->bits,
                *result,
                "hasher3"));
    }


    void generate_r1cs_constraints();
    void generate_r1cs_witness();

    static pb_variable_array<FieldT> from_bits(protoboard<FieldT> &pb, vector<bool> bits, pb_variable<FieldT>& ZERO);
    static vector<bool> from_char(unsigned char in);
    static vector<bool> from_char_array(unsigned char *in, size_t in_length);
    static vector<pb_variable_array<FieldT>> feed_dg1(protoboard<FieldT> &pb,
                                                      unsigned char* dg1, size_t dg1_length,
                                                      pb_variable<FieldT> &ZERO);
    static vector<pb_variable_array<FieldT>> feed_client_nonce(protoboard<FieldT> &pb,
                                                      unsigned char* client_nonce, size_t client_nonce_length,
                                                      pb_variable<FieldT> &ZERO);
    unsigned char *get_digest(shared_ptr<digest_variable<FieldT>> result);
    static void bit_vector_to_char_array(vector<bool> in, unsigned char *out, size_t out_length);
};
#endif //LIBSNARK_SHA256_SHA256_93B_GADGET_HPP
