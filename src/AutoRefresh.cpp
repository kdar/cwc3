#include <wx/string.h>

#include "AutoRefresh.h"
#include "Config.h"

//Must change the third  and fourth byte and update it with the length of this whole packet.
//Must also change the 5th byte and change it to the host count. This is obtained
//from the create game packet.
#define WC3_CONNECT_START "\xF7\x1E\x2C\x00\x08\x00\x00" \
                          "\x00\x00\x00\x00\x00\x00\xE4" \
                          "\x17\x00\x00\x00\x00"
                          //Name goes here, non-null terminated!
#define WC3_CONNECT_END   "\x00\x01\x00\x02\x00\x17\xE0" \
                          "\x7F\x00\x00\x01\x00\x00\x00" \
                          "\x00\x00\x00\x00\x00"

//===============================
void *AutoRefresh::Entry()
{ 
  //Contains the index of the next name to use
  static int nRefresherNameNext = 0;
  
  int nClients = 12;  
  
  wxString sGameHost = CONFIG(wxString, "Network/GameHost");
  int nGamePort = CONFIG(int, "Network/GamePort");
  int nDelay = CONFIG(int, "AutoRefresh/Delay");
  //wxString sName = CONFIG(wxString, "AutoRefresh/Name");
  IntStrMap nameMap = CONFIG(IntStrMap, "AutoRefresh/NameMap");
  
  ostringstream sPort;
  sPort << nGamePort;
  
  asio::io_service io_service;
  tcp::resolver resolver(io_service);
  tcp::resolver::query query(tcp::v4(), sGameHost.c_str(), sPort.str());
  tcp::endpoint receiver = *resolver.resolve(query);

  shared_ptr<tcp::socket> *socks = (shared_ptr<tcp::socket> *)new shared_ptr<tcp::socket>[nClients];
  for (int x = 0; x < nClients; x++)
    socks[x] = shared_ptr<tcp::socket>(new tcp::socket(io_service));
  
  char szSendBuffer[1000];
  char szReadBuffer[1000];
  
  while (!TestDestroy()) {    
    //A better sleep mechanism. Allows us to test if we need to destroy ourselves.
    //Keep looping if we're not suppose to run.
    for (int x = 0; x < nDelay || !m_bRun; x++) {
      Sleep(1000);
      if (TestDestroy()) return 0;
      
      //Reset x if we're not running.
      if (!m_bRun)
        x = 0;
    }
    
    //No need to continue if we're shutting down.
    if (TestDestroy()) return 0;
    
    int nClientsUsed = 0;
    for (int x = 0; x < nClients; x++) {
      wxString sName = nameMap[nRefresherNameNext];
      nRefresherNameNext = (x + 1) % nClients;
      
      try {
        socks[x]->connect(receiver);
        nClientsUsed++;
                
        //Construct our packet for sending
        memset(szSendBuffer, 0, sizeof(szSendBuffer));
        int index = 0;
        memcpy(szSendBuffer + index, WC3_CONNECT_START, sizeof(WC3_CONNECT_START));
        index += sizeof(WC3_CONNECT_START)-1;
        memcpy(szSendBuffer + index, sName.c_str(), sName.Length());
        index += sName.Length();
        memcpy(szSendBuffer + index, WC3_CONNECT_END, sizeof(WC3_CONNECT_END));
        index += sizeof(WC3_CONNECT_END)-1;
        
        //Update the size
        szSendBuffer[2] = index;
        //Set the hosted count
        szSendBuffer[4] = m_nHostedCount;
          
        socks[x]->send(asio::buffer(szSendBuffer, index));
        
        memset(szReadBuffer, 0, sizeof(szReadBuffer));      
        socks[x]->receive(asio::buffer(szReadBuffer, sizeof(szReadBuffer)));
        if (szReadBuffer[1] == 0x05 && szReadBuffer[2] == 0x08) {
          //  Game full                  Game started
    		  if (szReadBuffer[4] == 0x09 || szReadBuffer[4] == 0x0A) {
    			  break;
    			}
    		}    		
    	//} catch (boost::asio::error &e) {
      } catch (std::exception &e) {
        //Usually occurs if the server closes our connection.
      }
    }
    
    Sleep(800);
    
    //Only destroy the sockets we've created.
    for (int x = 0; x < nClientsUsed; x++) {
      try {
        socks[x]->close();
      //} catch (boost::asio::error &e) {
      } catch (std::exception &e) {
      }
    }
  }
  
  return 0;
}

