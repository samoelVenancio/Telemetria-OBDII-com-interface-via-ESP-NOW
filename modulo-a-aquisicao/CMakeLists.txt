# Raiz do build CMake do ESP-IDF — exigido mesmo sob PlatformIO.
# O código do app fica em src/ (registrado em src/CMakeLists.txt).
cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(modulo_a_aquisicao)
