#ifndef _PACKETCAPTURE_H
#define _PACKETCAPTURE_H

#include <pcap.h>

#ifdef BSD
#include <pcap-int.h>
#endif

#include <wx/string.h>

#include "Object.h"

//-------------------------------
//A C++ wrapper for the pcap library.
class PacketCapture : public Object<PacketCapture>
{
  public:
    PacketCapture()
      : m_pPcap(NULL)
    {}
    
    ~PacketCapture()
    { if (Get()) Close(); }

    bool Init(const wxString &sDevice);

    bool SetFilter(const wxString &sFilter);
    bool SetNonblock(bool bNonBlock = true);

    int GetDatalinkOffset();
    int DataLink();

    int Dispatch(int nCount, pcap_handler callback, u_char *szUser);
    int Loop(int nCount, pcap_handler callback, u_char *szUser);
    const u_char *Next(pcap_pkthdr *h);
    void BreakLoop();
    void Close();

    pcap_t *Get() const
    { return m_pPcap; }
    
    bpf_u_int32 GetNetworkNumber() const
    { return m_networkNumber; }
    
    bpf_u_int32 GetNetworkMask() const
    { return m_networkMask; }
    
    wxString &GetLastError()
    { return m_sLastError; }
    
    const wxString &GetLastError() const
    { return m_sLastError; }

  private:
    bool SystemHasDevice(const wxString &sDevice);

  private:
    pcap_t *m_pPcap;
    bpf_u_int32 m_networkNumber;
    bpf_u_int32 m_networkMask;
    
    wxString m_sLastError;
};

#endif
