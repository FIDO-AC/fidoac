#include "main.hpp"
#include <time.h>
#include <stdio.h>
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;
using namespace std;

static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";


static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

typedef unsigned char uchar;
static const std::string b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";//=
static std::string base64_encode(const std::string &in) {
    std::string out;

    int val=0, valb=-6;
    for (uchar c : in) {
        val = (val<<8) + c;
        valb += 8;
        while (valb>=0) {
            out.push_back(b[(val>>valb)&0x3F]);
            valb-=6;
        }
    }
    if (valb>-6) out.push_back(b[((val<<8)>>(valb+8))&0x3F]);
    while (out.size()%4) out.push_back('=');
    return out;
}

static std::string base64_decode(const std::string &in) {

    std::string out;

    std::vector<int> T(256,-1);
    for (int i=0; i<64; i++) T[b[i]] = i;

    int val=0, valb=-8;
    for (uchar c : in) {
        if (T[c] == -1) break;
        val = (val<<6) + T[c];
        valb += 6;
        if (valb>=0) {
            out.push_back(char((val>>valb)&0xFF));
            valb-=8;
        }
    }
    return out;
}

r1cs_ppzksnark_verification_key<default_r1cs_ppzksnark_pp> vk_global;


//r1cs_ppzksnark_proof<default_r1cs_ppzksnark_pp> get_proof(std::string proof_decoded){
//    r1cs_ppzksnark_proof<default_r1cs_ppzksnark_pp> proof;
//    std::stringstream proof_stream;
//    proof_stream.write(reinterpret_cast<const char *>(proof_decoded.c_str()), proof_decoded.size());
//    proof_stream >> proof;
//    return proof;
//}

int main(int argc, char** argv) {
    const size_t DG1_LENGTH = 93;
    int REF_YEAR = 2002;
    int CUR_YEAR = 22;
    unsigned char dg1arr[DG1_LENGTH] = {0x61,0x5b,0x5f,0x1f,0x58,0x50,0x3c,0x44,0x3c,0x3c,0x4e,0x41,0x4d,0x45,0x3c,0x56,0x4f,0x52,0x4e,0x41,0x4d,0x45,0x3c,0x5a,0x57,0x45,0x49,0x54,0x4e,0x41,0x4d,0x45,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x50,0x41,0x53,0x53,0x4e,0x55,0x4d,0x4d,0x45,0x52,0x31,0x3c,0x3c,0x39,0x35,0x30,0x31,0x30,0x31,0x31,0x4d,0x30,0x31,0x30,0x31,0x30,0x39,0x39,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x36};

    std::ifstream ifs("../setup/trustedSetup.json");
    json jf = json::parse(ifs);
    ifs.close();

    const std::string pk_base64_v = jf["pk"];
    const std::string vk_base64_v = jf["vk"];
    const std::string vk_base64_v_decoded = base64_decode(vk_base64_v);

    default_ec_pp::init_public_params();

    const r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp> keypair = trusted_setup(DG1_LENGTH, REF_YEAR, CUR_YEAR);
    // VK
    std::stringstream vk_stream_v;
    vk_stream_v.write(reinterpret_cast<const char *>(vk_base64_v_decoded.data()), vk_base64_v_decoded.size());
    vk_stream_v >> vk_global;

//    return 0;

    std::string proof_encoded = argv[1];
    std::string proof_decoded = base64_decode(proof_encoded);

    //Reconstruct Public Inputs
    std::stringstream public_input_stream_original;
    fido_ac_proof proof_obj =  prove(dg1arr, DG1_LENGTH, REF_YEAR, CUR_YEAR,
                                     nullptr, nullptr, 0, true);
    std::stringstream public_input_stream_serverreconstruct;
    public_input_stream_serverreconstruct << proof_obj.get_primary_input();


    //Parse Proof
    std::stringstream proof_stream;
    proof_stream.write(reinterpret_cast<const char *>(proof_decoded.c_str()), proof_decoded.size());
    // If you comment the line below the "vk_stream_v >> vk_global" works
    proof_stream >> proof_obj.proof;
//    return 0;

    return verify(proof_obj, vk_global);
}

