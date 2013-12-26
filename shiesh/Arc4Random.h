/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Shielded Shell. 
  The Shielded Shell (ShieSH) is a port of Open SSH to
  Windows operating system.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <openssl/rc4.h>

namespace ssh {

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

