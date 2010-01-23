#pragma once

#include <openssl/rc4.h>

namespace coressh {

class Arc4Random
{
public:
  Arc4Random(void);
  
  // function returns pseudo-random numbers 
  // in the range of 0 to (2**32)-1
  unsigned int arc4random(void);

protected:

  void arc4random_stir(void);
  void arc4random_buf(void *_buf, size_t n);
  u_int32_t arc4random_uniform(u_int32_t upper_bound);

private:
  int rc4_ready;
  RC4_KEY rc4;
};

}

