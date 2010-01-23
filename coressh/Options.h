#pragma once
#include "ssingleton.h"
#include <string>

class Options : public SSingleton<Options>
{
public:
  static const std::string version;
  static const std::string progName;

  const std::string
  prog_name_version () const
  {
    return progName + '_' + version;
  }

  const std::string 
  get_protocol_version_exchange_string () const;

  // TODO change to number in time interval
  int get_max_login_attempts () const;
};

