//
// Created by yeohw on 08/11/2022.
//

#include "fidoac_verify.h"
#include <vector>
#include <iostream>
#include "depends/libsnark-fido-ac/example/main.hpp"
extern "C"{
    int fidoac_verify(std::vector<unsigned char> proof_data,std::vector<unsigned char> randomized_hash, int ageLimit,
                      std::vector<unsigned char> verification_key){
        std::cout << "Hello from Library\n";
    //    return 0;
        return verify_proof(proof_data, randomized_hash, ageLimit, verification_key);
    }
}

