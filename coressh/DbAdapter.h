#pragma once
#include "ssingleton.h"

class DbAdapter : public SSingleton<DbAdapter>
{
public:
  DbAdapter(void);
  ~DbAdapter(void);
};
