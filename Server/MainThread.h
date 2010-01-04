#pragma once
#include "SThread.h"

class MainThread : public SThread
{
public:
  static MainThread* create ()
  {
    return new MainThread ();
  }

protected:
  // overrides
  void run ();
};
