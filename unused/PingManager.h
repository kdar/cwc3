#ifndef _PINGMANAGER_H
#define _PINGMANAGER_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/thread.h>
#include <wx/dynarray.h>

//Maybe I'll make a class for this as well.
#include <libnet.h>

#include "Inet.h"
#include "PacketCapture.h"
#include "Client.h"

//-------------------------------
struct PingClient
{
  wxDateTime start;
  Client *pClient;
  u_int32_t unSequence;
};

WX_DECLARE_OBJARRAY (PingClient*, PingList);

//-------------------------------
class PingManager : public PacketCapture, public wxThread
{
  public:
    PingManager ();
    ~PingManager ();

    static PingManager *Get ();
    static PingManager *Set (PingManager *pInstance);

    bool Init ();
    void Shutdown ();

    void Render (Client *pClient);

    void ProcessPacket (u_char *pArgs, const struct pcap_pkthdr *pHeader, const u_char *pPacket);

  private:
    virtual void *Entry ();

  private:
    static PingManager *ms_pInstance;

    bool m_bShutdown;

    PingList m_pingList;
};

#endif
