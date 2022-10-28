#include "main.hpp"
#include <time.h>
#include <stdio.h>
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;
using namespace std;


std::string base64_encode(const std::string &s)
{
    static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i=0,ix=0,leng = s.length();
    std::stringstream q;

    for(i=0,ix=leng - leng%3; i<ix; i+=3)
    {
        q<< base64_chars[ (s[i] & 0xfc) >> 2 ];
        q<< base64_chars[ ((s[i] & 0x03) << 4) + ((s[i+1] & 0xf0) >> 4)  ];
        q<< base64_chars[ ((s[i+1] & 0x0f) << 2) + ((s[i+2] & 0xc0) >> 6)  ];
        q<< base64_chars[ s[i+2] & 0x3f ];
    }
    if (ix<leng)
    {
        q<< base64_chars[ (s[ix] & 0xfc) >> 2 ];
        q<< base64_chars[ ((s[ix] & 0x03) << 4) + (ix+1<leng ? (s[ix+1] & 0xf0) >> 4 : 0)];
        q<< (ix+1<leng ? base64_chars[ ((s[ix+1] & 0x0f) << 2) ] : '=');
        q<< '=';
    }
    return q.str();
}

int main() {
    const size_t DG1_LENGTH = 93;
    int REF_YEAR = 2002;
    int CUR_YEAR = 22;

    // trusted setup
    const r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp> keypair = trusted_setup(DG1_LENGTH, REF_YEAR, CUR_YEAR);
    std::stringstream pk_stream;
    pk_stream << keypair.pk;
    const std::string pk_base64 = base64_encode(pk_stream.str().c_str());

    std::stringstream vk_stream;
    vk_stream << keypair.vk;
    const std::string vk_base64 = base64_encode(vk_stream.str().c_str());

    json root;
    root["pk"] = pk_base64;
    root["vk"] = vk_base64;

    std::ofstream file("trustedSetup.json");
    file << root;

    return 0;
}

