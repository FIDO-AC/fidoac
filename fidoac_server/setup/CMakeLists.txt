# set curve to the one libsnark is built with
set(CURVE ALT_BN128)
add_definitions(-DCURVE_ALT_BN128)
find_package(nlohmann_json 3.11.2 REQUIRED)

if(${USE_PREBUILT_LIBS})
    include_directories(
            ../include
            ../../libsnark/include
    )

    add_library(setup main.cpp)
    target_link_libraries(setup libsnark-fido-ac nlohmann_json::nlohmann_json)
else()
    include_directories(
            ../include
            ../depends/libsnark
    )
    configure_file(DG1_clean.bin DG1_clean.bin COPYONLY)
    add_executable(setup main.cpp)
    target_link_libraries(setup libsnark-fido-ac nlohmann_json::nlohmann_json)
endif()