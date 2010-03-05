#pragma once

#include "HasStringView.h"
#include "ChannelRequest.h"

class CoreConnection;

class ChannelRequestPars : public HasStringView
{
public:
  ChannelRequestPars
    (const char* _name/*,
     CoreConnection* con*/);

  virtual ~ChannelRequestPars ();

  // Overrides
  void outString (std::ostream& out) const
  {
    out << name;
  }

  const std::string name; // i.e., "pty-req"

  virtual void read_from_packet 
    (CoreConnection* con) = 0;
};
