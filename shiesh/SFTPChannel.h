#pragma once
#include "sessionchannel.h"

class SubsystemPars;

class SFTPChannel : public SessionChannel
{
  friend ChannelPars;
public:
  // Overrides
  bool is_complete_packet_in_descending ();

protected:
  SFTPChannel (const SessionChannel& from);

  // Overrides
  void subproc2ascending ();

  // Overrides
  void descending2subproc ();

};
