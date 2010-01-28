#pragma once

#include <string>
#include "Channel.h"
#include "Repository.h"

struct ChannelPars;

typedef Repository<Channel, ChannelPars> ChannelRepository;

struct ChannelPars
{
  ChannelPars (const std::string& remName)
    : remote_id (-1), rwindow (0), rmaxpack (0),
    remote_name (remName)
  {}

  virtual ~ChannelPars () {}

  std::string remote_name; // remote hostname
  int remote_id;  // peer's id of the channel
  u_int rwindow;  // peer's max window size
  u_int rmaxpack; // peer's max packet size

  virtual Channel* create_derivation 
    (const ChannelRepository::ObjectCreationInfo&) const = 0;
};

// For session type of channel
struct SessionChannelPars : public ChannelPars
{
  SessionChannelPars ()
    : ChannelPars ("server-session")
  {}

  Channel* create_derivation 
    (const ChannelRepository::ObjectCreationInfo&) const;
};
