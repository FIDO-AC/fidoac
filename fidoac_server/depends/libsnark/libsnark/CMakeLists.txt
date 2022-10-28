set(SNARK_EXTRALIBS)
if(${CURVE} STREQUAL "BN128")
  set(
    SNARK_EXTRALIBS

    ${SNARK_EXTRALIBS}
    ${PROCPS_LIBRARIES}
    zm
  )
endif()

# TODO(madars): Revisit the decision to only offer a static
# library. In theory, it might be nice to offer a shared object, but
# it may be impractical for users, since many of the build-time
# options are mutually exclusive. At least as a static library with a
# simple, reproducible build, users of this library can pick the
# appropriate options themselves for embedding.
file(
  GLOB_RECURSE
  LIBSNARK_SOURCE

  *.?pp
  *.tcc
)
list(FILTER LIBSNARK_SOURCE EXCLUDE REGEX ".*/tests/.*")
list(FILTER LIBSNARK_SOURCE EXCLUDE REGEX ".*/profiling/.*")
list(FILTER LIBSNARK_SOURCE EXCLUDE REGEX ".*/examples/.*")

add_library(
  snark
  STATIC

  ${LIBSNARK_SOURCE}
)

if("${USE_LINKED_LIBRARIES}")
  target_link_libraries(
    snark

    ff
    ${GMP_LIBRARIES}
    ${GMPXX_LIBRARIES}
    ${CRYPTO_LIBRARIES}
    ${PROCPS_LIBRARIES}
    ${LIBFF_LIBRARIES}
    ${SNARK_EXTRALIBS}
  )
else()
  target_link_libraries(
    snark

    ff
    ${GMP_LIBRARIES}
    ${GMPXX_LIBRARIES}
    ${CRYPTO_LIBRARIES}
    ${PROCPS_LIBRARIES}
    ${SNARK_EXTRALIBS}
  )
  target_include_directories(
    snark

    PUBLIC
    ${DEPENDS_DIR}/libff
    ${DEPENDS_DIR}/libfqfft
  )
endif()

install(
  DIRECTORY "" DESTINATION "include/libsnark"
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.tcc"
  PATTERN "tests" EXCLUDE
  PATTERN "profiling" EXCLUDE
  PATTERN "examples" EXCLUDE
)

install(
  TARGETS
  snark

  DESTINATION lib
)

if("${WITH_SUPERCOP}")
  add_library(
    snark_adsnark

    common/default_types/r1cs_ppzkadsnark_pp.cpp
  )
  target_link_libraries(
    snark_adsnark

    snark
    snark_supercop
  )
  install(
    TARGETS
    snark_adsnark

    DESTINATION lib
  )
endif()

# Demos
function(libsnark_demo DEMO_NAME SOURCE_FILE)
  message("DEMO: ${DEMO_NAME} ${SOURCE_FILE}")
  add_executable(${DEMO_NAME} EXCLUDE_FROM_ALL ${SOURCE_FILE})
  target_link_libraries(${DEMO_NAME} snark ${Boost_LIBRARIES})
endfunction()

libsnark_demo(demo_arithmetization reductions/ram_to_r1cs/examples/demo_arithmetization.cpp)
libsnark_demo(demo_ram_ppzksnark zk_proof_systems/ppzksnark/ram_ppzksnark/examples/demo_ram_ppzksnark.cpp)
libsnark_demo(demo_ram_ppzksnark_generator zk_proof_systems/ppzksnark/ram_ppzksnark/examples/demo_ram_ppzksnark_generator.cpp)
libsnark_demo(demo_ram_ppzksnark_prover zk_proof_systems/ppzksnark/ram_ppzksnark/examples/demo_ram_ppzksnark_prover.cpp)
libsnark_demo(demo_ram_ppzksnark_verifier zk_proof_systems/ppzksnark/ram_ppzksnark/examples/demo_ram_ppzksnark_verifier.cpp)
if("${WITH_SUPERCOP}")
  libsnark_demo(demo_r1cs_ppzkadsnark zk_proof_systems/ppzkadsnark/r1cs_ppzkadsnark/examples/demo_r1cs_ppzkadsnark.cpp)
endif()

# Profiling:
#   libsnark_profile(<profile-name> <source-files> ...)
function(libsnark_profile PROFILE_NAME)
  message("PROFILE: ${PROFILE_NAME} ${ARGN}")
  add_executable(${PROFILE_NAME} EXCLUDE_FROM_ALL ${ARGN})
  target_link_libraries(${PROFILE_NAME} snark)
endfunction()

libsnark_profile(profile_routing_algorithms common/routing_algorithms/profiling/profile_routing_algorithms.cpp)
libsnark_profile(profile_routing_gadgets gadgetlib1/gadgets/routing/profiling/profile_routing_gadgets.cpp)
libsnark_profile(profile_r1cs_ppzksnark zk_proof_systems/ppzksnark/r1cs_ppzksnark/profiling/profile_r1cs_ppzksnark.cpp)
libsnark_profile(profile_r1cs_se_ppzksnark zk_proof_systems/ppzksnark/r1cs_se_ppzksnark/profiling/profile_r1cs_se_ppzksnark.cpp)
libsnark_profile(profile_r1cs_mp_ppzkpcd zk_proof_systems/pcd/r1cs_pcd/r1cs_mp_ppzkpcd/profiling/profile_r1cs_mp_ppzkpcd.cpp)
libsnark_profile(profile_r1cs_sp_ppzkpcd zk_proof_systems/pcd/r1cs_pcd/r1cs_sp_ppzkpcd/profiling/profile_r1cs_sp_ppzkpcd.cpp)
libsnark_profile(profile_bacs_ppzksnark zk_proof_systems/ppzksnark/bacs_ppzksnark/profiling/profile_bacs_ppzksnark.cpp)
libsnark_profile(profile_r1cs_gg_ppzksnark zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/profiling/profile_r1cs_gg_ppzksnark.cpp)
libsnark_profile(profile_ram_ppzksnark zk_proof_systems/ppzksnark/ram_ppzksnark/profiling/profile_ram_ppzksnark.cpp)
libsnark_profile(profile_tbcs_ppzksnark relations/circuit_satisfaction_problems/tbcs/examples/tbcs_examples.cpp zk_proof_systems/ppzksnark/tbcs_ppzksnark/profiling/profile_tbcs_ppzksnark.cpp)
libsnark_profile(profile_uscs_ppzksnark zk_proof_systems/ppzksnark/uscs_ppzksnark/profiling/profile_uscs_ppzksnark.cpp)
libsnark_profile(profile_ram_zksnark zk_proof_systems/zksnark/ram_zksnark/profiling/profile_ram_zksnark.cpp)

# Tests
if ("${IS_LIBSNARK_PARENT}")
  include(CTest)

  # libsnark_test(<test_name> <source_files> ...)
  function(libsnark_test TEST_NAME)
    message("TEST: ${TEST_NAME} ${ARGN}")
    add_executable(${TEST_NAME} EXCLUDE_FROM_ALL ${ARGN})
    target_link_libraries(${TEST_NAME} snark gtest_main)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    add_dependencies(check ${TEST_NAME})
  endfunction()

  libsnark_test(gadgetlib1_simple_test gadgetlib1/tests/gadgetlib1_test.cpp)
  libsnark_test(gadgetlib2_adapters_test gadgetlib2/tests/adapters_UTEST.cpp)
  libsnark_test(gadgetlib2_constraint_test gadgetlib2/tests/constraint_UTEST.cpp)
  libsnark_test(gadgetlib2_gadget_test gadgetlib2/tests/gadget_UTEST.cpp)
  libsnark_test(
    gadgetlib2_integration_test
    gadgetlib2/examples/simple_example.hpp
    gadgetlib2/tests/integration_UTEST.cpp
    relations/constraint_satisfaction_problems/r1cs/examples/r1cs_examples.hpp
    relations/constraint_satisfaction_problems/r1cs/examples/r1cs_examples.tcc
    zk_proof_systems/ppzksnark/r1cs_ppzksnark/examples/run_r1cs_ppzksnark.hpp
    zk_proof_systems/ppzksnark/r1cs_ppzksnark/examples/run_r1cs_ppzksnark.tcc
    gadgetlib2/examples/simple_example.cpp
    gadgetlib2/examples/simple_example.hpp
    )
  libsnark_test(gadgetlib2_protoboard_test gadgetlib2/tests/protoboard_UTEST.cpp)
  libsnark_test(gadgetlib2_variable_test gadgetlib2/tests/variable_UTEST.cpp)
  libsnark_test(relations_qap_test relations/arithmetic_programs/qap/tests/test_qap.cpp)
  libsnark_test(relations_sap_test relations/arithmetic_programs/sap/tests/test_sap.cpp)
  libsnark_test(relations_ssp_test relations/arithmetic_programs/ssp/tests/test_ssp.cpp)
  libsnark_test(zk_proof_systems_bacs_ppzksnark_test zk_proof_systems/ppzksnark/bacs_ppzksnark/tests/test_bacs_ppzksnark.cpp)
  libsnark_test(zk_proof_systems_r1cs_ppzksnark_test zk_proof_systems/ppzksnark/r1cs_ppzksnark/tests/test_r1cs_ppzksnark.cpp)
  libsnark_test(zk_proof_systems_r1cs_se_ppzksnark_test zk_proof_systems/ppzksnark/r1cs_se_ppzksnark/tests/test_r1cs_se_ppzksnark.cpp)
  libsnark_test(zk_proof_systems_r1cs_gg_ppzksnark_test zk_proof_systems/ppzksnark/r1cs_gg_ppzksnark/tests/test_r1cs_gg_ppzksnark.cpp)
  libsnark_test(zk_proof_systems_ram_ppzksnark_test zk_proof_systems/ppzksnark/ram_ppzksnark/tests/test_ram_ppzksnark.cpp)
  libsnark_test(
    zk_proof_systems_tbcs_ppzksnark_test
    relations/circuit_satisfaction_problems/tbcs/examples/tbcs_examples.cpp
    zk_proof_systems/ppzksnark/tbcs_ppzksnark/tests/test_tbcs_ppzksnark.cpp
    )
  libsnark_test(zk_proof_systems_uscs_ppzksnark_test zk_proof_systems/ppzksnark/uscs_ppzksnark/tests/test_uscs_ppzksnark.cpp)
  libsnark_test(test_basic_gadgets gadgetlib1/tests/test_basic_gadgets.cpp)
  libsnark_test(test_knapsack_gadget gadgetlib1/gadgets/hashes/knapsack/tests/test_knapsack_gadget.cpp)
  libsnark_test(test_merkle_tree_gadgets gadgetlib1/gadgets/merkle_tree/tests/test_merkle_tree_gadgets.cpp)
  libsnark_test(test_set_commitment_gadget gadgetlib1/gadgets/set_commitment/tests/test_set_commitment_gadget.cpp)
  libsnark_test(test_sha256_gadget gadgetlib1/gadgets/hashes/sha256/tests/test_sha256_gadget.cpp)
  libsnark_test(test_polynomial_commitments polynomial_commitments/tests/test_polynomial_commitments.cpp)
  libsnark_test(test_common_routing_algorithms common/routing_algorithms/tests/test_routing_algorithms.cpp)
  libsnark_test(test_pairing gadgetlib1/tests/test_pairing.cpp)
  libsnark_test(test_fp6_3over2_gadgets gadgetlib1/tests/test_fp6_3over2_gadgets.cpp)
  libsnark_test(test_fp12_2over3over2_gadgets gadgetlib1/tests/test_fp12_2over3over2_gadgets.cpp)
  libsnark_test(test_curve_gadgets gadgetlib1/tests/test_curve_gadgets.cpp)
  libsnark_test(test_bls12_377_membership_checks gadgetlib1/tests/test_bls12_377_membership_checks.cpp)
  libsnark_test(test_bls12_377_pairing gadgetlib1/tests/test_bls12_377_pairing.cpp)
  libsnark_test(test_pairing_checks gadgetlib1/tests/test_pairing_checks.cpp)
  libsnark_test(test_r1cs_ppzksnark_verifier_gadget gadgetlib1/tests/test_r1cs_ppzksnark_verifier_gadget.cpp)
  libsnark_test(test_r1cs_gg_ppzksnark_verifier_gadget gadgetlib1/tests/test_r1cs_gg_ppzksnark_verifier_gadget.cpp)
  libsnark_test(test_kzg10_verifier_gadget gadgetlib1/tests/test_kzg10_verifier_gadget.cpp)

  # TODO (howardwu): Resolve runtime on targets:
  # libsnark_test(gadgetlib1_fooram_test gadgetlib1/gadgets/cpu_checkers/foora# m/examples/test_fooram.cpp)
  # libsnark_test(zk_proof_systems_r1cs_mp_ppzkpcd_test zk_proof_systems/pcd/r1cs_pcd/r1cs_mp_ppzkpcd/tests/test_r1cs_mp_ppzkpcd.cpp)
  # libsnark_test(zk_proof_systems_r1cs_sp_ppzkpcd_test zk_proof_systems/pcd/r1cs_pcd/r1cs_sp_ppzkpcd/tests/test_r1cs_sp_ppzkpcd.cpp)
  # libsnark_test(zk_proof_systems_ram_zksnark_test zk_proof_systems/zksnark/ram_zksnark/tests/test_ram_zksnark.cpp)
endif()
