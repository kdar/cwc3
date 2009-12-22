#include "NetEventDispatcher.h"

//===============================
NetEventDispatcher::NetEventDispatcher(shared_ptr<wxEvtHandler> pHandler)
{
  m_pHandler = shared_ptr<wxEvtHandler>(pHandler);
  AddDefaultEventMatches();
}

//===============================
//Uses all the event matches in the event match list to see if we can find a match
//to the network data we were given. If we find a match then we know the NetEventType.
//So then we can fire off the event to the event handler.
bool NetEventDispatcher::Process(const net_ip *pIp, const net_tcp *pTcp, const u_char *pData, u_int nDataSize)
{  
  char match[3];
  match[0] = pData[0];
  match[1] = pData[1];
  match[2] = 0;
  shared_ptr<NetEvent> e = m_netEventMap[match];
  if (e && e->GetId() != -1) {
    e->m_pData = pData;
    e->m_nDataSize = nDataSize;
    e->m_pTcp = pTcp;
    e->m_pIp = pIp;
    m_pHandler->ProcessEvent(*e);
  }

  return false;
}


//===============================
//Adds default events to the list.
//We use this to compare to network data, and fire off
//their corresponding events.
void NetEventDispatcher::AddDefaultEventMatches()
{ 
  m_netEventMap["\xFF\x02"] = shared_ptr<NetEvent>(new NetEvent(NET_HostAlone));
  m_netEventMap["\xFF\x0A"] = shared_ptr<NetEvent>(new NetEvent(NET_LoginSuccessful));
  m_netEventMap["\xFF\x0C"] = shared_ptr<NetEvent>(new NetEvent(NET_JoinChannel));
  m_netEventMap["\xFF\x0E"] = shared_ptr<NetEvent>(new NetEvent(NET_CommandSend)); 
  m_netEventMap["\xFF\x0F"] = shared_ptr<NetEvent>(new NetEvent(NET_Whisper));
  m_netEventMap["\xFF\x1C"] = shared_ptr<NetEvent>(new NetEvent(NET_GameCreate));
  m_netEventMap["\xFF\x22"] = shared_ptr<NetEvent>(new NetEvent(NET_SendBattlenetGameName));
  m_netEventMap["\xFF\x44"] = shared_ptr<NetEvent>(new NetEvent(NET_GameEnded));
  
  m_netEventMap["\xF7\x04"] = shared_ptr<NetEvent>(new NetEvent(NET_SlotTableInitial));
  m_netEventMap["\xF7\x06"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientInfo1));
  m_netEventMap["\xF7\x07"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientEnd));
  m_netEventMap["\xF7\x08"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientIdFinishedLoading));
  m_netEventMap["\xF7\x09"] = shared_ptr<NetEvent>(new NetEvent(NET_SlotTableUpdated));
  m_netEventMap["\xF7\x0A"] = shared_ptr<NetEvent>(new NetEvent(NET_GameStarted));
  m_netEventMap["\xF7\x0B"] = shared_ptr<NetEvent>(new NetEvent(NET_GameLoading)); 

  m_netEventMap["\xF7\x0C"] = shared_ptr<NetEvent>(new NetEvent(NET_GameFinishedLoading));   
  m_netEventMap["\xF7\x0F"] = shared_ptr<NetEvent>(new NetEvent(NET_ChatSend));
  m_netEventMap["\xF7\x1B"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientLeftReply));
  m_netEventMap["\xF7\x1C"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientSlotKick));
  m_netEventMap["\xF7\x1E"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientConnectAttempt));
  m_netEventMap["\xF7\x21"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientLeftSent));
  m_netEventMap["\xF7\x23"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientFinishedLoading));
  m_netEventMap["\xF7\x28"] = shared_ptr<NetEvent>(new NetEvent(NET_ChatReceive));
  m_netEventMap["\xF7\x37"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientInfo2));
  m_netEventMap["\xF7\x3D"] = shared_ptr<NetEvent>(new NetEvent(NET_MapInfo));  
  m_netEventMap["\xF7\x3F"] = shared_ptr<NetEvent>(new NetEvent(NET_InitiateMapUpload));  
  m_netEventMap["\xF7\x42"] = shared_ptr<NetEvent>(new NetEvent(NET_ClientMapStatus));
}
