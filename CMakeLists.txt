cmake_minimum_required(VERSION 3.20)
project(HaiLang)
set(CMAKE_CXX_STANDARD 20)

if(MSVC)
  add_compile_options(/utf-8)
endif()

add_executable(haic)

target_include_directories(haic PRIVATE
    src
    deps/spdlog/include
)
target_sources(haic PRIVATE
    src/common/Log.cpp
    src/common/StringView.cpp

    src/version.h
    src/tokens_def.h
    src/lexer.h
     src/syntax_parser.h

#    examples/lexer_path_arg.cpp
    examples/parser.cpp
)





