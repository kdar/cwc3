#ifndef _TCPKILL_H
#define _TCPKILL_H

#include <wx/defs.h>
#include <wx/string.h>

#include "Object.h"

//class PacketCapture;

//-------------------------------
class TcpKill : public Object<TcpKill>
{
  public:
    TcpKill();

    void Execute(const wxString &sIp, u_int nPort);

  private:
    int m_nSeverity;
    int m_nMaxTime;
};

#endif
