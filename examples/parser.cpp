#include <fstream>

#include "common/Log.h"
#include "common/Defer.hpp"

#include "lexer.h"
#include "syntax_parser.h"

int internal_main(int argc,char** argv);
int main(int argc,char** argv)
{
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

    if (sv_starts_with(input,"\""))
    {
      input = sv_slice(input,1);
    }
    if (sv_ends_with(input,"\""))
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

    input = src;
  }


  Lexer lexer{
    .input_ = input
  };

  {
    Lexer lexer_copy = lexer;
    for(;;) {
      Token token = lexer_copy.get_token();
      if(token.type == ETK_EOF) {
        break;
      }
      LOG_INFO("{} \t\t {}",token.to_str(),kETokenTypeName[token.type]);
    }
  }

  auto root = parse_expression(&lexer,0);



  root->print([](const Token& t)
  {
    return std::string(t.to_str());
  });

#if defined(DEBUG) || defined(_DEBUG)
  LOG_INFO("total alloc {} bytes",g_allocted_size);
  root.release();
  if(g_allocted_size != 0)
  {
    LOG_INFO("allocator leak {} bytes",g_allocted_size);
  }
#endif

  return 0;
}



