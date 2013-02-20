#pragma once
#include "ChannelRequestPars.h"
#include "PTY.h"
#include "Repository.h"

typedef Repository<PTY, PTYPars> PTYRepository;

class CoreConnection;

class PTYPars : public ChannelRequestPars
{
public:
  u_int col;
  u_int row;
  u_int xpixel;
  u_int ypixel;
  std::string term;

  // Different terminal-based subsystems can exist.
  std::string subsystem_name;

  PTYPars
    (const char* _name/*,
     CoreConnection* con*/);

  virtual PTY* create_derivation 
    (const Repository<PTY, PTYPars>::
     ObjectCreationInfo&
     ) const;

  virtual PTY* transform_object
    (PTY* from) const
  {
    return from; // no transformation
  }

  // Overrides
  void outString (std::ostream& out) const;

protected:
  // Overrides
  void read_from_packet (CoreConnection* con);
};

