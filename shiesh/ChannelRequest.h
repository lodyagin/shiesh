#pragma once
#include <string>

class ChannelRequest
{
public:
  const std::string universal_object_id;

  ChannelRequest (const std::string& objectId) 
    : universal_object_id (objectId)
  {}
  virtual ~ChannelRequest() {}
};
