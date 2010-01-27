#pragma once
#include "Channel.h"
#include "ChannelPars.h"

class ChannelFactory : protected ChannelRepository
{
public:
  ChannelFactory(void);
  virtual ~ChannelFactory(void);

  Channel* open_new_channel (const ChannelPars& pars);
};
