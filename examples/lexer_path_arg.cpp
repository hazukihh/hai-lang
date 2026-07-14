#include <fstream>

#include "common/Log.h"
#include "common/Defer.hpp"

#include "lexer.h"

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

  if(argc != 2) {
    LOG_ERROR("Usage: {} <file_path>",argv[0]);
    return 1;
  }
  const char* file_path = argv[1];

  std::ifstream ifs(file_path,std::ios_base::binary);
  if (!ifs.is_open())
  {
    LOG_ERROR("not exist file at \"{}\"",file_path);
    return 1;
  }
  auto input = std::string(
    std::istreambuf_iterator<char>(ifs),
    std::istreambuf_iterator<char>());


  Lexer lexer{
    .input_ = input
  };


  for(;;) {
    Token token = lexer.get_token();
    if(token.type == ETK_EOF) {
      break;
    }
    LOG_INFO("{} \t\t {}",token.to_str(),kETokenTypeName[token.type]);
  }



  return 0;
}



