#pragma once
#include "ssingleton.h"
#include <string>

class Options : public SSingleton<Options>
{
public:
  Options(void);
  ~Options(void);

  const std::string& get_protocol_version_exchange_string ()
  {
    static std::string str 
      ("SSH-2.0-CoreSSH_1.0d1 obihod\r\n");
    return str;
  }
};

