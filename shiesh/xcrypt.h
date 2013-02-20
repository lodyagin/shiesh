#pragma once

#include <string>

namespace coressh {

std::string xcrypt(const char *password, const char *salt);

}
