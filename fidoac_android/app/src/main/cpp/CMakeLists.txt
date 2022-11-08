# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("fidoac")

#add_library( # Sets the name of the library.
#        fidoacverify
#
#        # Sets the library as a shared library.
#        SHARED
#
#        # Provides a relative path to your source file(s).
#        fidoac_verify.cpp)
#
#add_executable( # Sets the name of the exe.
#        mainexe
#        # Provides a relative path to your source file(s).
#        mainexe.cpp)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        fidoac

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        native-lib.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        fidoac

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})

############################
# libsnark specific part:
############################
# include depends directory
add_subdirectory(depends)

add_definitions(-DCURVE_ALT_BN128)

include_directories(
        depends/libsnark/include
        depends/libsnark-fido-ac/include
)

# link against libsnark-sha256
target_link_libraries(fidoac

        example)

#target_link_libraries(fidoacverify
#
#        example)
#
#target_link_libraries(mainexe
#
#        example)