#pragma once

#include <string>

namespace ssh {

std::string xcrypt(const char *password, const char *salt);

}
