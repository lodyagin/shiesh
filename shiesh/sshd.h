#pragma once

#include <string>
#include "RInOutSocket.h"

namespace ssh {

void
sshd_exchange_identification
  (RInOutSocket& socket,
   std::string& server_version,
   std::string& client_version
   );

}