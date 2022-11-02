# set curve to the one libsnark is built with
set(CURVE ALT_BN128)
add_definitions(-DCURVE_ALT_BN128)

if(${USE_PREBUILT_LIBS})

    include_directories(
            ../include
            ../../libsnark/include
    )

    add_library(libsnark-fido-ac
            gadgets/sha256_93B_gadget.cpp
            gadgets/age_verification_gadget.cpp
            gadgets/fido_ac_gadget.cpp
            fido_ac_trusted_setup.cpp
            fido_ac_prover.cpp
            fido_ac_verifier.cpp
            )

    target_link_libraries(libsnark-fido-ac

            ${PREBUILT_PATH}/libsnark.a
            ${PREBUILT_PATH}/libff.a
            ${PREBUILT_PATH}/libgmp.a
            )
else()
    include_directories(
            ../include
            ../depends/libsnark
    )

    add_library(libsnark-fido-ac
            gadgets/sha256_93B_gadget.cpp
            gadgets/age_verification_gadget.cpp
            gadgets/fido_ac_gadget.cpp
            fido_ac_trusted_setup.cpp
            fido_ac_prover.cpp
            fido_ac_verifier.cpp
            )

    target_link_libraries(libsnark-fido-ac snark)
endif()