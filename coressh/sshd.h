#pragma once

#include <string>

namespace coressh {

void
sshd_exchange_identification
  (SOCKET sock_in, 
   SOCKET sock_out,
   std::string& server_version,
   std::string& client_version
   );

}