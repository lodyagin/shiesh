#pragma once
#include "SThread.h"

class MainThread : public SThread
{
public:
  MainThread ()
    : SThread (main) 
  {};

protected:
  // overrides
  void run ();
};
