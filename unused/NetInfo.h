#ifndef _NETINFO_H
#define _NETINFO_H

#include "Object.h"

//-------------------------------
//Contains netork info that needs to be known
class NetInfo : public Object<NetInfo>
{
  public:
    NetInfo()
      : dataLink(0), bInitialized(false)
    {}

  public:
    int dataLink;
    u_int8_t gamehost_ether[ETHER_ADDR_LEN];

    bool bInitialized;
};

#endif
