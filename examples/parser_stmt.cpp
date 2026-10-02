#include <fstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h> // SetConsoleOutputCP, SetConsoleCP

#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif

#include "common/Log.h"
#include "common/Defer.hpp"

#include "lexer.h"
#include "expr.h"
#include "parser.h"
#include "common/variant_match.h"


int internal_main(int argc,char** argv);
int main(int argc,char** argv)
{
#ifdef _WIN32
  SetConsoleOutputCP(CP_UTF8);  // CP_UTF8 = 65001
  SetConsoleCP(CP_UTF8);
#endif

  Log::Init("%^%v%$");
  DEFER[]{
    Log::Shutdown();
  };

  try {
    return internal_main(argc,argv);
  }
  catch (const std::exception& e) {
    LOG_CORE_ERROR("Exception: {}", e.what());
    return 1;
  }
}


constexpr auto UTF8_BOM_CSTR = "\xEF\xBB\xBF";


int internal_main(int argc,char** argv)
{
  LOG_CORE_INFO("hello world");

  if(argc < 2) {
    LOG_ERROR("Usage: {} <file_path>",argv[0]);
    LOG_ERROR("Usage: {} --raw <str>",argv[0]);
    return 1;
  }
  bool option_raw = sv_eq(argv[1],"--raw");

  std::string src;
  StringView input;
  if (option_raw)
  {
    src = argv[2];
    input = sv_trim(src);

    while (sv_starts_with(input,"\""))
    {
      input = sv_slice(input,1);
    }
    while (sv_ends_with(input,"\""))
    {
      input = sv_slice(input,0,input.size()-1);
    }
  }
  else
  {
    const char* file_path = argv[1];
    std::ifstream ifs(file_path,std::ios_base::binary);
    if (!ifs.is_open())
    {
      LOG_ERROR("not exist file at \"{}\"",file_path);
      return 1;
    }
    src = std::string(
      std::istreambuf_iterator<char>(ifs),
      std::istreambuf_iterator<char>());

    // auto detect utf-8 bom
    if (sv_starts_with(src,UTF8_BOM_CSTR))
    {
      input = sv_slice(src,3);
    } else {
      input = src;
    }
  }


  Lexer lexer;
  lexer.init(input);

#if 0
    Lexer lexer_copy = lexer;
    for(;;) {
      Token token = lexer_copy.get_token();
      if(token.type == ETK_EOF) {
        break;
      }
      // LOG_WARN("Lexer: Unknown character: {}", input_[0]);
      LOG_INFO("{} \t\t {}",token.to_str(),ETokenType_to_Str(token.type));
    }
    LOG_INFO("====Lexer End====");
#endif

  auto get_value = [](const Token& t)
  {
    return std::string(t.to_str());
  };

  /// parse_program
  while (lexer.peek().type != ETK_EOF)
  {
    auto* stmt = parse_stmt(&lexer);

    print(stmt);
    g_arena.reset();
  }

  lexer.expect(ETK_EOF);

  return 0;
}



