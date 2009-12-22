#ifndef _TCPKILL_H
#define _TCPKILL_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/thread.h>

#include "Object.h"

class PacketCapture;

//-------------------------------
class TcpKill : public wxThread, public Object<TcpKill>
{
  public:
    TcpKill(const wxString &sIp, u_int nPort = 0);

  protected:
    bool Init();
    bool InitPcap();

    void Execute();
    void WaitToExecute();

  private:
    virtual void *Entry();

  private:
    const wxString m_sIp;
    u_int m_nPort;

    int m_nSeverity;
    int m_nMaxTime;
    
    shared_ptr<PacketCapture> m_pPacketCapture;
};

#endif
