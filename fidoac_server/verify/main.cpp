#include "main.hpp"
#include <time.h>
#include <stdio.h>
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;
using namespace std;


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

int main(int argc, char** argv) {
    std::ifstream ifs("../setup/trustedSetup.json");
    json jf = json::parse(ifs);
    ifs.close();

//    r1cs_ppzksnark_verification_key<default_r1cs_ppzksnark_pp> vk_global;
    r1cs_ppzksnark_proof<default_r1cs_ppzksnark_pp> proof;

    const size_t DG1_LENGTH = 93;
    int REF_YEAR = 2002;
    int CUR_YEAR = 22;
    unsigned char dg1arr[DG1_LENGTH] = {0x61,0x5b,0x5f,0x1f,0x58,0x50,0x3c,0x44,0x3c,0x3c,0x4e,0x41,0x4d,0x45,0x3c,0x56,0x4f,0x52,0x4e,0x41,0x4d,0x45,0x3c,0x5a,0x57,0x45,0x49,0x54,0x4e,0x41,0x4d,0x45,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x50,0x41,0x53,0x53,0x4e,0x55,0x4d,0x4d,0x45,0x52,0x31,0x3c,0x3c,0x39,0x35,0x30,0x31,0x30,0x31,0x31,0x4d,0x30,0x31,0x30,0x31,0x30,0x39,0x39,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x36};
    const r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp> keypair = trusted_setup(DG1_LENGTH, REF_YEAR, CUR_YEAR);
//    const r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp> keypair = trusted_setup(DG1_LENGTH, REF_YEAR, CUR_YEAR);
//    vk_global = keypair.vk;

    std::string vk_encoded = jf["vk"].get<std::string>();
    std::string vk_decoded = base64_decode(vk_encoded);

//    std::vector<char> verification_key;
//    std::copy(vk_decoded.begin(), vk_decoded.end(), std::back_inserter(verification_key));


    r1cs_ppzksnark_verification_key<default_r1cs_ppzksnark_pp> vk_global;
    vk_global = keypair.vk;
    std::stringstream vk_stream_v;
    vk_stream_v.write(reinterpret_cast<const char *>(vk_decoded.data()), vk_decoded.size());
    vk_stream_v >> vk_global;

    std::string proof_encoded = argv[0];
    std::string proof_decoded = base64_decode(proof_encoded);



    //Reconstruct Public Inputs
    std::stringstream public_input_stream_original;
    fido_ac_proof proof_obj =  prove(dg1arr, DG1_LENGTH, REF_YEAR, CUR_YEAR,
                                     nullptr, nullptr, 0, true);
    //Parse Proof
    std::stringstream proof_stream;
    proof_stream.write(reinterpret_cast<const char *>(proof_decoded.c_str()), proof_decoded.size());
    proof_stream >> proof_obj.proof;

    return verify(proof_obj, vk_global);
}

