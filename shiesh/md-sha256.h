#pragma once
#include <openssl/evp.h>

namespace ssh {

const EVP_MD * evp_ssh_sha256(void);

}
