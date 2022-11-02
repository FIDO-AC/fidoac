#ifndef LIBSNARK_FIDO_AC_MAIN_HPP
#define LIBSNARK_FIDO_AC_MAIN_HPP

#include <libsnark-sha256/fido_ac_trusted_setup.hpp>
#include <libsnark-sha256/fido_ac_prover.hpp>
#include <libsnark-sha256/fido_ac_verifier.hpp>
#include <fstream>
#include <iterator>

typedef libff::Fr<libff::default_ec_pp> FieldT;

int main(int argc, char** argv);

#endif //LIBSNARK_FIDO_AC_MAIN_HPP
