#include "StdAfx.h"
#include "ChannelFactory.h"

ChannelFactory::ChannelFactory(void)
: ChannelRepository (10000) //UT overflow
{
}

ChannelFactory::~ChannelFactory(void)
{
}

Channel* ChannelFactory::open_new_channel (const ChannelPars& pars)
{
  return create_object (pars);
}

