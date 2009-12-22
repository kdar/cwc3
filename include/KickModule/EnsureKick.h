#ifndef _ENSUREKICK_H
#define _ENSUREKICK_H

#include <wx/thread.h>

#include "Object.h"
#include "TcpKill.h"

class Client;

typedef shared_ptr<Client> ClientPtr;
typedef list<ClientPtr> KickList;

//-------------------------------
class EnsureKick : public wxThread, public Object<EnsureKick>
{
  public:
    EnsureKick() 
      : wxThread(wxTHREAD_DETACHED)
    { Create(); }
    
    static shared_ptr<EnsureKick> Get();
    static shared_ptr<EnsureKick> Set(shared_ptr<EnsureKick> pInstance);
    
    void Add(shared_ptr<Client> pClient);
    
  private:
    virtual void *Entry();
    
  private:
    static shared_ptr<EnsureKick> ms_pInstance;
    
    KickList m_list;
    
    TcpKill m_tcpKill;
    
    wxCriticalSection m_cs;
};

#endif
