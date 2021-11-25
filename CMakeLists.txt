cmake_minimum_required( VERSION 3.10 )
project( fair-placer )

add_executable( fair-placer main.cpp placer.h placer.cpp )

target_compile_features( fair-placer PRIVATE cxx_std_17 )

FILE( COPY dataset/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )