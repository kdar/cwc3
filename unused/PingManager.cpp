#include "PingManager.h"
#include "Interface.h"
#include "Config.h"

#include <wx/arrimpl.cpp> 
WX_DEFINE_OBJARRAY (PingList);

PingManager *PingManager::ms_pInstance = NULL;

//-------------------------------
//Callbacks
static void ProcessPacket (u_char *pArgs, const struct pcap_pkthdr *pHeader, const u_char *pPacket);
//-------------------------------

//===============================
PingManager::PingManager ()
  : wxThread (wxTHREAD_JOINABLE),
    m_bShutdown(false)
{
}

//===============================
PingManager::~PingManager ()
{
}

//===============================
/*static*/ PingManager *PingManager::Get ()
{
  if (!ms_pInstance)
    ms_pInstance = new PingManager ();
  return ms_pInstance;
}

//===============================
/*static*/ PingManager *PingManager::Set (PingManager *pInstance)
{
  PingManager *pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
bool PingManager::Init ()
{
  if (Create () != wxTHREAD_NO_ERROR)
    return false;

  if (!P_Init (INTERNET_DEVICE) || !P_SetFilter (wxString::Format ("tcp [tcpflags] & (tcp-rst) != 0 and tcp [tcpflags] & (tcp-ack) != 0", GAME_PORT)) || !P_SetNonblock ())
    return false;

  return true;
}

//===============================
void PingManager::Shutdown ()
{
  m_bShutdown = true;
  P_BreakLoop ();
}

//===============================
void *PingManager::Entry ()
{
  while (!m_bShutdown) {
    P_Dispatch (-1, ::ProcessPacket, 0);

    //Somehow stops 100% cpu usage
    usleep (0);
  }

  return 0;
}

//===============================
void PingManager::Render (Client *pClient)
{
  libnet_t *pLibnet;
  char szErrBuf[LIBNET_ERRBUF_SIZE];
  u_long src_ip, dst_ip;
  libnet_ptag_t t;

  //Maybe I'll randomly generate this one day... or not.
  u_int32_t unSequence = 0x01010101;

  pLibnet = libnet_init (LIBNET_RAW4, INTERNET_DEVICE, szErrBuf);
  libnet_seed_prand (pLibnet);

  src_ip  = libnet_get_ipaddr4 (pLibnet);
  dst_ip  = libnet_name2addr4 (pLibnet, const_cast<char *>(pClient->sIp.c_str ()), LIBNET_DONT_RESOLVE);
 
  t = libnet_build_tcp (GAME_PORT, pClient->nPort, unSequence, 0x02020202, TH_SYN, 512, 0, 10, LIBNET_TCP_H, NULL, 0, pLibnet, 0);
  t = libnet_build_ipv4 (LIBNET_IPV4_H+LIBNET_TCP_H, 0, libnet_get_prand (LIBNET_PRu16), 0, 64, IPPROTO_TCP, 0, src_ip, dst_ip, NULL, 0, pLibnet, 0);

  pClient->nFlags ^= CF_PINGING;  

  PingClient *pPingClient = new PingClient;
  pPingClient->pClient = pClient;
  pPingClient->unSequence = unSequence;
  pPingClient->start = wxDateTime::UNow (); //We do this at the last second so the start timestamp is correct
  m_pingList.Add (pPingClient);

  //Write the packet
  libnet_write (pLibnet);
}

//===============================
void PingManager::ProcessPacket (u_char *pArgs, const struct pcap_pkthdr *pHeader, const u_char *pPacket)
{
  int nIpOffset       = P_GetDatalinkOffset ();
  const net_ip *pIp   = (net_ip*)(pPacket + nIpOffset);
  const net_tcp *pTcp = (net_tcp*)(pPacket + nIpOffset + sizeof (net_ip));

  for (u_int x = 0; x < m_pingList.Count (); x++) {
    Client *pClient = m_pingList.Item (x)->pClient;
    if (pClient->nFlags & CF_PINGING && 
        pClient->sIp.IsSameAs (inet_ntoa (pIp->ip_src)) && 
        pClient->nPort == ntohs (pTcp->th_sport) &&
        m_pingList.Item (x)->unSequence == ntohl (pTcp->th_ack)-1) {
      pClient->ping = wxDateTime::UNow ().Subtract (m_pingList.Item (x)->start);
      pClient->nFlags ^= CF_PINGED;
      pClient->nFlags ^= CF_PINGING;

      Interface::Get ()->Output (wxString::Format ("Ping: \"%s\" (%u)", pClient->sName.c_str (), pClient->ping.GetMilliseconds ().GetValue ()));

      m_pingList.RemoveAt (x);
    }
  }
}


//*******************************
//Callbacks
//*******************************

//===============================
void ProcessPacket (u_char *pArgs, const struct pcap_pkthdr *pHeader, const u_char *pPacket)
{
  PingManager::Get ()->ProcessPacket (pArgs, pHeader, pPacket);
}
