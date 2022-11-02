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

int main() {
    const size_t DG1_LENGTH = 93;
    int REF_YEAR = 2002;
    int CUR_YEAR = 22;

    // trusted setup
    const r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp> keypair = trusted_setup(DG1_LENGTH, REF_YEAR, CUR_YEAR);
    std::stringstream pk_stream;
    pk_stream << keypair.pk;
    const std::string pk_base64 = base64_encode(pk_stream.str());

    std::stringstream vk_stream;
    vk_stream << keypair.vk;
    const std::string vk_base64 = base64_encode(vk_stream.str());

    json root;
    root["pk"] = pk_base64;
    root["vk"] = vk_base64;

    std::ofstream file("trustedSetup.json");
    file << root;
    file.close();

    //    Verification
    std::ifstream ifs("trustedSetup.json");
    json jf = json::parse(ifs);
    ifs.close();

    const std::string pk_base64_v = jf["pk"];
    const std::string vk_base64_v = jf["vk"];
    const std::string vk_base64_v_decoded = base64_decode(vk_base64_v);


    r1cs_ppzksnark_verification_key<default_r1cs_ppzksnark_pp> vk_global;
    std::stringstream vk_stream_v;
    vk_stream_v.write(reinterpret_cast<const char *>(vk_base64_v_decoded.data()), vk_base64_v_decoded.size());
    vk_stream_v >> vk_global;


    return 0;
}

