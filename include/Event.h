#ifndef _EVENT_H
#define _EVENT_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/object.h>

#include <wx/event.h>

#include "Object.h"

//Forward declarations.
struct net_ip;
struct net_tcp;

DECLARE_EVENT_TYPE(EVT_NET_ACTION, 8000)

//-------------------------------
class NetEvent : public wxEvent, public Object<NetEvent>
{
  public:
    NetEvent(int id = -1)
      : wxEvent(id, EVT_NET_ACTION)
    {}
    
    virtual ~NetEvent() {}
    
    NetEvent(const NetEvent &other)
      : wxEvent(other)
    {
      m_pData = other.m_pData;
      m_nDataSize = other.m_nDataSize;
      m_pIp = other.m_pIp;
      m_pTcp = other.m_pTcp;
      m_pExtra = other.m_pExtra;
      m_nExtraSize = other.m_nExtraSize;
    }
    
    virtual wxEvent *Clone() const {
      return new NetEvent(*this);
    }
 
  public:
    const u_char *m_pData;
    u_int m_nDataSize;
    const net_ip* m_pIp;
    const net_tcp* m_pTcp;
    
    shared_ptr<void> m_pExtra;
    int m_nExtraSize;
    
  private:
    //DECLARE_DYNAMIC_CLASS_NO_ASSIGN(NetEvent)
    DECLARE_DYNAMIC_CLASS(NetEvent)
};

typedef void (wxEvtHandler::*NetEventFunction)(NetEvent&);
  
#define NetEventHandler(func) \
  (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(NetEventFunction, &func)

#define EVT_NET(id, fn) \
  DECLARE_EVENT_TABLE_ENTRY( \
      EVT_NET_ACTION, id, wxID_ANY, \
      (wxObjectEventFunction)(wxEventFunction) wxStaticCastEvent(NetEventFunction, &fn), \
      (wxObject *) NULL \
  ),

#endif
