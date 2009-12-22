#include <wx/datetime.h>

//Maybe I'll make a class for this as well.
#include <libnet.h>

#include "TcpKill.h"
#include "Config.h"
#include "Defines.h"
#include "Info.h"
#include "Inet.h"
#include "PacketCapture.h"

//===============================
TcpKill::TcpKill(const wxString &sIp, u_int nPort)
    : wxThread(wxTHREAD_DETACHED),
    m_sIp(sIp),
    m_nPort(nPort),
    m_nSeverity(15),
    m_nMaxTime(15),
    m_pPacketCapture(shared_ptr<PacketCapture>(new PacketCapture()))
{
  Init();
}

//===============================
bool TcpKill::Init()
{
  if (Create() != wxTHREAD_NO_ERROR)
    return false;
}

//===============================
bool TcpKill::InitPcap()
{
  wxString sSniffDevice = CONFIG(wxString, "Network/SniffDevice");
  wxString sGameHost = CONFIG(wxString, "Network/GameHost");

  if (!m_pPacketCapture->Init(sSniffDevice))
    return false;
    
  if (!m_pPacketCapture->SetNonblock())
    return false;

  if (!m_pPacketCapture->SetFilter(wxString::Format("src host %s and dst host %s", m_sIp.c_str(), sGameHost.c_str())))
    return false;

  return true;
}

//===============================
void *TcpKill::Entry()
{
  if (!m_nPort) {
    WaitToExecute();    
  } else {
    Execute();
  }

  //Has to be done AFTER the person is kicked or the program will segfault.
  Info::Get()->RemoveNodeInfoEntry(m_sIp, m_nPort);

  return 0;
}

//===============================
//I tried to do this same concept using LIBNET_RAW4 instead of LIBNET_LINK so
//I wouldn't have to worry about the ethernet header. But something weird happens when I
//try to do this. If there is already an established connection.. e.g. 70.116.23.2:3779 to
//192.168.0.2:6112, if we send the packet using raw4, in tcpdump we get
//70.116.23.2:1024 --> 192.168.0.2:6112. I am really not sure why it won't send the correct
//source port if that connection is already established, but using LIBNET_LINK seems to not
//have this problem. I believe it is a kernel related issue.
void TcpKill::Execute()
{
  char szErrBuf[LIBNET_ERRBUF_SIZE];
  libnet_ptag_t t;

  wxString sSniffDevice = CONFIG(wxString, "Network/SniffDevice");
  wxString sGameHost = CONFIG(wxString, "Network/GameHost");
  int nGamePort = CONFIG(int, "Network/GamePort");

  shared_ptr<NodeInfo> pNodeInfo = Info::Get()->GetNodeInfo(m_sIp, m_nPort);
  if (!pNodeInfo) {
    //Interface::Get()->Output(wxString::Format("Can't find %s:%d in net info map.", m_sIp.c_str(), m_nPort), OUTPUT_ERROR);
    return;
  }

  NetInfo netInfo = Info::Get()->GetNetInfo();

  //Here to make sure we kill it...
  for (u_int y = 0; y < 2; y++) {
    for (u_int x = 0; x < m_nSeverity; x++) {
      u_int32_t seq_out;
      
      if (y)
        seq_out = pNodeInfo->last_sentack;
      else
        seq_out = pNodeInfo->last_sentack + x*pNodeInfo->last_recvwin;
  
      libnet_t *pLibnet = libnet_init(LIBNET_LINK, const_cast<char *>(sSniffDevice.c_str()), szErrBuf);
      libnet_seed_prand(pLibnet);
  
      t = libnet_build_tcp(m_nPort, nGamePort,
                           seq_out, 0, TH_RST, 0, 0, 0, LIBNET_TCP_H, NULL, 0, pLibnet, 0);
  
      u_long src_ip = libnet_name2addr4(pLibnet, const_cast<char *>(m_sIp.c_str()), LIBNET_DONT_RESOLVE);
      u_long dst_ip = libnet_name2addr4(pLibnet, const_cast<char *>(sGameHost.c_str()), LIBNET_DONT_RESOLVE);
  
      t = libnet_build_ipv4(sizeof(net_ip) + sizeof(net_tcp), 0,
                            libnet_get_prand(LIBNET_PRu16), 0, 64, IPPROTO_TCP, 0,
                            src_ip, dst_ip, NULL, 0, pLibnet, 0);
  
      t = libnet_autobuild_ethernet(netInfo.gamehost_ether, ETHERTYPE_IP, pLibnet);
  
      libnet_write(pLibnet);
      libnet_destroy(pLibnet);
      
      TestDestroy();
    }
  }
}

//===============================
void TcpKill::WaitToExecute()
{
  pcap_pkthdr packethdr;
  net_ethernet *pEth;
  net_ip *pIp;
  net_tcp *pTcp;
  libnet_ptag_t t;
  u_char *pPacket;
  u_int32_t seq_in, seq_out;
  u_short win_in;
  char szErrBuf[LIBNET_ERRBUF_SIZE];
  libnet_t *pLibnet;

  wxString sSniffDevice = CONFIG(wxString, "Network/SniffDevice");

  InitPcap();

  wxDateTime dt1 = wxDateTime::Now();

  int nIpOffset = m_pPacketCapture->GetDatalinkOffset();

  for (;;) {
    if ((pPacket = (u_char *)m_pPacketCapture->Next(&packethdr)) != NULL) {
      pEth = (net_ethernet *)(pPacket);

      pIp = (net_ip *)(pPacket + nIpOffset);
      if (pIp->ip_p != IPPROTO_TCP) continue;

      pTcp = (net_tcp *)(pPacket + nIpOffset + sizeof(net_ip));
      if ((pTcp->th_flags & TH_SYN) || (pTcp->th_flags & TH_FIN) || (pTcp->th_flags & TH_RST)) continue;

      seq_in = ntohl(pTcp->th_seq);
      win_in = ntohs(pTcp->th_win);

      for (int i = 0; i < m_nSeverity; i++) {
        seq_out = seq_in + (i * win_in);

        pLibnet = libnet_init(LIBNET_LINK, const_cast<char *>(sSniffDevice.c_str()), szErrBuf);
        libnet_seed_prand(pLibnet);

        t = libnet_build_tcp(ntohs(pTcp->th_sport), ntohs(pTcp->th_dport),
                             seq_out, 0, TH_RST, 0, 0, 0, LIBNET_TCP_H, NULL, 0, pLibnet, 0);

        t = libnet_build_ipv4(sizeof(net_ip) + sizeof(net_tcp), 0,
                              libnet_get_prand(LIBNET_PRu16), 0, 64, IPPROTO_TCP, 0,
                              pIp->ip_src.s_addr, pIp->ip_dst.s_addr, NULL, 0, pLibnet, 0);

        t = libnet_build_ethernet(pEth->ether_dhost, pEth->ether_shost,
                                  ETHERTYPE_IP, NULL, 0, pLibnet, 0);

        libnet_write(pLibnet);
        libnet_destroy(pLibnet);

        /*wxString sIpSrc = inet_ntoa(pIp->ip_src);
        wxString sIpDest = inet_ntoa(pIp->ip_dst);
        Interface::Get()->Output(wxString::Format("%s:%d > %s:%d: R %u:%u(0) win 0",
            sIpSrc.c_str(),
            ntohs(pTcp->th_sport),
            sIpDest.c_str(),
            ntohs(pTcp->th_dport), seq_out, seq_out));*/
      }
    }

    wxDateTime dt2 = wxDateTime::Now();
    if (dt1.IsValid() && dt2.IsValid())
      if (dt2.Subtract(dt1).GetSeconds() >= m_nMaxTime)
        break;

    Sleep(100);
    TestDestroy();
  }
}
