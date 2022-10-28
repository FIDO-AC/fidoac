set(FF_EXTRASRCS)
set(FF_EXTRALIBS)
if(${CURVE} STREQUAL "BN128")
  set(
    FF_EXTRASRCS

    ${FF_EXTRASRCS}
    algebra/curves/bn128/bn128_g1.cpp
    algebra/curves/bn128/bn128_g2.cpp
    algebra/curves/bn128/bn128_gt.cpp
    algebra/curves/bn128/bn128_init.cpp
    algebra/curves/bn128/bn128_pairing.cpp
    algebra/curves/bn128/bn128_pp.cpp
  )
  set(
    FF_EXTRALIBS

    ${FF_EXTRALIBS}
    zm
  )
endif()

file(
  GLOB_RECURSE
  LIBFF_SOURCE

  *.?pp
  *.tcc
)
list(FILTER LIBFF_SOURCE EXCLUDE REGEX ".*/tests/.*")
list(FILTER LIBFF_SOURCE EXCLUDE REGEX ".*/profile/.*")

add_library(
  ff
  STATIC

  ${LIBFF_SOURCE}
  ${FF_EXTRASRCS}
)
target_link_libraries(
  ff

  GMP::gmp
  ${PROCPS_LIBRARIES}
  ${FF_EXTRALIBS}
)
target_include_directories(
  ff
  PUBLIC .. ${OPENSSL_INCLUDE_DIR}
)

install(
  DIRECTORY "" DESTINATION "include/libff"
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.tcc"
  PATTERN "tests" EXCLUDE
  PATTERN "profile" EXCLUDE
  PATTERN "examples" EXCLUDE
)

install(
  TARGETS ff DESTINATION lib
)

# Tests
if ("${IS_LIBFF_PARENT}")

  include(CTest)

  # Test executables

  # libff_test(<test_name> <source_file>)
  function(libff_test TEST_NAME SOURCE_FILE)
    message("TEST: ${TEST_NAME} ${SOURCE_FILE}")
    add_executable(${TEST_NAME} EXCLUDE_FROM_ALL ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} ff gtest_main)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    add_dependencies(check ${TEST_NAME})
  endfunction()

  libff_test(test_common common/tests/test_common.cpp)
  libff_test(test_algebra_bilinearity algebra/curves/tests/test_bilinearity.cpp)
  libff_test(test_algebra_groups algebra/curves/tests/test_groups.cpp)
  libff_test(test_algebra_fields algebra/fields/tests/test_fields.cpp)
  libff_test(test_algebra_multiexp algebra/scalar_multiplication/tests/test_multiexp.cpp)

  # Profile executables

  # libff_profile(<profile_name> <source_file>)
  function(libff_profile PROFILE_NAME SOURCE_FILE)
    message("PROFILE: ${PROFILE_NAME} ${SOURCE_FILE}")
    add_executable(${PROFILE_NAME} EXCLUDE_FROM_ALL ${SOURCE_FILE})
    target_link_libraries(${PROFILE_NAME} ${OPENSSL_LIBRARIES} ff)
    if (UNIX AND NOT APPLE)
      target_link_libraries(${PROFILE_NAME} rt)
    endif()
    add_dependencies(profile ${PROFILE_NAME})
  endfunction()

  libff_profile(profile_multiexp algebra/scalar_multiplication/profile/profile_multiexp.cpp)
  libff_profile(profile_algebra_groups algebra/curves/profile/profile_algebra_groups.cpp)
  libff_profile(profile_algebra_groups_read algebra/curves/profile/profile_algebra_groups_read.cpp)
endif()
