#include "PacketCapture.h"

//===============================
bool PacketCapture::Init(const wxString &sDevice)
{
  char szErrBuf[PCAP_ERRBUF_SIZE];
  char *pDevice = const_cast<char *>(sDevice.c_str());

  if (!SystemHasDevice(pDevice)) {
    m_sLastError = wxString::Format("System doesn't have device: %s.", pDevice);
    return false;
  }

  if (pcap_lookupnet(pDevice, &m_networkNumber, &m_networkMask, szErrBuf) == -1) {
    m_sLastError = wxString::Format("pcap_lookupnet(): %s", szErrBuf);
    return false;
  }

  m_pPcap = pcap_open_live(pDevice, 65536, 1, 1000, szErrBuf);
  if (!m_pPcap) {
    m_sLastError = wxString::Format("pcap_open_live(): %s", szErrBuf);
    return false;
  }

#ifdef BSD
  int on = 1;
  if (ioctl(pd->fd, BIOCIMMEDIATE, &on) < 0) {
    m_sLastError = wxString::Format("Call to ioctl() failed.");
    m_pPcap = NULL;
    return false;
  }
#endif

  return true;
}


//===============================
bool PacketCapture::SystemHasDevice(const wxString &sDevice)
{
  char szErrBuf[PCAP_ERRBUF_SIZE];
  pcap_if_t *pAllDevsp = 0;

  if (pcap_findalldevs(&pAllDevsp, szErrBuf) == -1) {
    m_sLastError = wxString::Format("pcap_findalldevs(): %s", szErrBuf);
    return false;
  }

  for (pcap_if_t *pIterate = pAllDevsp; pIterate != NULL; pIterate = pIterate->next) {
    if (sDevice.IsSameAs(pIterate->name)) {
      return true;
    }
  }

  return false;
}

//===============================
bool PacketCapture::SetFilter(const wxString &sFilter)
{
  if (!m_pPcap)
    return false;

  bpf_program fcode;
  if (pcap_compile(m_pPcap, &fcode, const_cast<char *>(sFilter.c_str()), 1, m_networkMask) == -1) {
    m_sLastError = wxString::Format("pcap_compile(): %s", pcap_geterr(m_pPcap));
    return false;
  }

  if (pcap_setfilter(m_pPcap, &fcode) == -1) {
    m_sLastError = wxString::Format("pcap_setfilter(): %s", pcap_geterr(m_pPcap));
    return false;
  }

  return true;
}

//===============================
bool PacketCapture::SetNonblock(bool bNonBlock)
{
  if (!m_pPcap)
    return false;

  char szErrBuf[PCAP_ERRBUF_SIZE];

  int nNonBlock = bNonBlock ? 1 : 0;

  //We have to do this so when we shutdown the program, we can actually exit immediately without having to wait
  //for a packet to be received for pcap_dispatch to return so we can check if we should exit or not.
  if (pcap_setnonblock(m_pPcap, nNonBlock, szErrBuf) == -1) {
    m_sLastError = wxString::Format("pcap_setnonblock(): %s", szErrBuf);
    return false;
  }
}

//===============================
int PacketCapture::DataLink()
{
  return pcap_datalink(m_pPcap);
}

//===============================
int PacketCapture::GetDatalinkOffset()
{
  int nOffset = -1;

  int nDatalink = DataLink();
  switch (nDatalink) {
    case DLT_EN10MB:
      nOffset = 14;
      break;

    case DLT_IEEE802:
      nOffset = 22;
      break;

    case DLT_FDDI:
      nOffset = 21;
      break;

#ifdef DLT_LOOP
    case DLT_LOOP:
#endif
    case DLT_NULL:
      nOffset = 4;
      break;

    case DLT_LINUX_SLL:
      nOffset = 16;
      break;

    default:
      m_sLastError = wxString::Format("Datalink not supported: %d.", nDatalink);
      break;
  }

  return nOffset;
}

//===============================
int PacketCapture::Dispatch(int nCount, pcap_handler callback, u_char *szUser)
{
  //if (!m_pPcap)
  //  return -1;

  return pcap_dispatch(m_pPcap, nCount, callback, szUser);
}

//===============================
int PacketCapture::Loop(int nCount, pcap_handler callback, u_char *szUser)
{
  //if (!m_pPcap)
  //  return -1;

  return pcap_loop(m_pPcap, nCount, callback, szUser);
}

//===============================
const u_char *PacketCapture::Next(pcap_pkthdr *h)
{
  //if (!m_pPcap)
  //  return NULL;

  return pcap_next(m_pPcap, h);
}

//===============================
void PacketCapture::BreakLoop()
{
  //if (!m_pPcap)
  //  return;

  pcap_breakloop(m_pPcap);
}

//===============================
void PacketCapture::Close()
{
  //if (!m_pPcap)
  //  return;

  pcap_close(m_pPcap);
}
