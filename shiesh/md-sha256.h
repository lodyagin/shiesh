#pragma once
#include <openssl/evp.h>

namespace coressh {

const EVP_MD * evp_ssh_sha256(void);

}
