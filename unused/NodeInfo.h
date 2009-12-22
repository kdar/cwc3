#ifndef _NODEINFO_H
#define _NODEINFO_H

#include "Object.h"

//-------------------------------
//Contains network info about a specific node(ip:port)
class NodeInfo : public Object<NodeInfo>
{
  public:
    NodeInfo()
      : last_sentack(0), last_recvack(0), last_sentseq(0),
        last_recvseq(0), last_sentwin(0), last_recvwin(0),
        bPinged(false)
    {}
    
  public:
    u_int32_t last_sentack;
    u_int32_t last_recvack;
    u_int32_t last_sentseq;
    u_int32_t last_recvseq;
    u_short last_sentwin;
    u_short last_recvwin;

    wxDateTime sent;
    wxTimeSpan ping;
    bool bPinged;

    wxDateTime lastResponse;
};

#endif
