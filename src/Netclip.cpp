#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Netclip.h"
#include "Config.h"

udp::endpoint Netclip::m_receiver;

//===============================
/*static*/ bool Netclip::Init()
{
  wxString sHost = CONFIG(wxString, "Netclip/Host");
  int nPort = CONFIG(int, "Netclip/Port");
  
  ostringstream sPort;
  sPort << nPort;
  
  //We do this so we only have to resolve once.
  try {
    asio::io_service io_service;
    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), sHost.c_str(), sPort.str());    
    m_receiver = *resolver.resolve(query);
  //} catch (asio::error &e) {
  //  return false; 
  } catch (std::exception &e) {
    return false; 
  }
  
  return true;
}

//===============================
/*static*/ int Netclip::Send(const wxString &sData)
{
  if (sData.IsEmpty())
    return -1;
    
  wxString sSend = sData;
  if (sSend[sSend.Length()-1] != '\n')
    sSend += "\n";
    
  try { 
    asio::io_service io_service;
    udp::socket socket(io_service);
    socket.open(udp::v4());
    return socket.send_to(asio::buffer(sSend.c_str(), sSend.Length()), m_receiver);
  } catch (...) {}
  
  return -1;
}
