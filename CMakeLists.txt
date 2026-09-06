cmake_minimum_required(VERSION 3.20)
project(HaiLang)
set(CMAKE_CXX_STANDARD 20)

if(MSVC)
  add_compile_options(/utf-8)
endif()

add_executable(haic)

target_include_directories(haic PRIVATE
    src
    deps/
    deps/spdlog/include
    deps/parallel-hashmap/include # flat_hash_map ...
)
target_sources(haic PRIVATE
    src/common/Log.cpp
    src/common/StringView.cpp

    src/arena.h
    src/tokens_def.h
    src/lexer.h
    src/lexer.cpp
    src/expr.h
    src/expr.cpp
    src/parser.h
    src/parser.cpp

#    examples/lexer_path_arg.cpp
#    examples/parser_expr.cpp
    examples/parser_stmt.cpp
)

if (1)
  enable_testing()
  #  find_package(GTest CONFIG REQUIRED)
  #  target_link_libraries(haic PRIVATE
  #      GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main
  #  )
  add_subdirectory(tests)
endif ()






