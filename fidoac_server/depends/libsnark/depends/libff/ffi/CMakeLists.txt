
# A simple language-agnostic dynamic library for languages that
# support FFI functionality

file(
  GLOB_RECURSE
  FFI_SOURCE

  **.hpp
  **.tcc
  **.cpp
  **.h
)

add_library(ff-ffi SHARED ${FFI_SOURCE})
target_include_directories(ff-ffi PRIVATE .. .)
target_link_libraries(ff-ffi ff)
