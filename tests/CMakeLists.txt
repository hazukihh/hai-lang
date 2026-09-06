# Utils:
macro(Test_Case name src_text)
  add_test(NAME ${name} COMMAND haic --raw "${src_text}")
endmacro()

set(CASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
# 单个用例注册
function(Hai_Test name)
  set(f "${CASE_DIR}/${name}.hai")
  if(NOT EXISTS "${f}")
    message(FATAL_ERROR "[Error] missing test case: ${f}")
  endif()
  add_test(NAME ${name} COMMAND haic "${f}")
  set_tests_properties(${name} PROPERTIES TIMEOUT 15)
endfunction()

# 自动发现：新增 .hai 文件不用改 CMake
function(auto_hai_tests)
  file(GLOB cases RELATIVE "${CASE_DIR}" "${CASE_DIR}/*.hai")
  foreach(c ${cases})
    string(REGEX REPLACE "\\.hai$" "" name "${c}")
    Hai_Test(${name})
  endforeach()
endfunction()


# Tests:

## 0 test lexer
Test_Case(mlComment_1 [=[/**/]=])
Test_Case(mlComment_2 [=["/* /* */ */ "]=])
Test_Case(mlComment_3 [=["/* left more"]=])
Test_Case(mlComment_4 "/* left more/**/")
Test_Case(mlComment_5 "/* \n\n\n */ ")
Test_Case(mlComment_6 "/* a*b*c */ ")
Test_Case(mlComment_7 "/* ====*=== */")
Test_Case(mlComment_8 "/* http://example.com */")
Test_Case(mlComment_9 "/* a*b */")
Test_Case(mlComment_10 [=[/**/
/* */

/***
* //sdsdsd
* //sdsdsd
*/]=])
Test_Case(mlComment_e1 [=[/***/、/******/var x: s32 = 11;]=])
Test_Case(mlComment_e2 "/* right more */ */")

Hai_Test("test_unknown_token")


Hai_Test("expr__StrLit_and_CharLit")


## complete code file
Hai_Test("code1")

