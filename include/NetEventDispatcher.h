#ifndef _NETEVENTDISPATCHER_H
#define _NETEVENTDISPATCHER_H

#include "Event.h"
#include "Object.h"

//-------------------------------
enum NetEventType
{
  NET_GameCreate,
  NET_GameEnded,
  NET_GameStarted,
  NET_GameExited,  
  NET_GameLoading,
  NET_GameClose,
  NET_CommandSend,
  NET_HostAlone,
  NET_SlotTableInitial,
  NET_SlotTableUpdated,
  NET_MapInfo,
  NET_ClientInfo1,
  NET_ClientInfo2,
  NET_ClientSlotKick,
  NET_ClientConnectAttempt,
  NET_ClientLeftSent,
  NET_ClientLeftReply,
  NET_ClientEnd,
  NET_ClientMapStatus,
  NET_InitiateMapUpload,
  NET_Whisper,
  NET_ChatSend,
  NET_ChatReceive,
  NET_JoinChannel,
  NET_LoginSuccessful,
  NET_ClientFinishedLoading,
  NET_ClientIdFinishedLoading,
  NET_SendBattlenetGameName,
  NET_GameFinishedLoading,
  NET_vClientDisconnected
};

//Forward declarations.
class NetEventMatch;

//typedef vector<NetEventMatch> NetEventMatchList;
typedef shared_ptr<NetEvent> NetEventPtr;
typedef map<wxString, NetEventPtr> NetEventMap;

//-------------------------------
//A class that allows us to store match information with a given
//net event type.
class NetEventMatch : public Object<NetEventMatch>
{
  public:
    NetEventMatch(NetEventType type, int nMatchSize, shared_array<u_char> pMatch)
      : m_type(type), m_nMatchSize(nMatchSize), m_pMatch(pMatch)
    { }
    
    ~NetEventMatch() { }
    
  public:
    NetEventType m_type;
    int m_nMatchSize;
    shared_array<u_char> m_pMatch;
};

//-------------------------------
class NetEventDispatcher : public Object<NetEventDispatcher>
{
  public:
    NetEventDispatcher(shared_ptr<wxEvtHandler> pHandler);
    ~NetEventDispatcher() { }
    
    bool Process(const net_ip *pIp, const net_tcp *pTcp, const u_char *pData, u_int nDataSize);
    
  protected:
    void AddDefaultEventMatches();
    
  private:
    //NetEventMatchList m_eventMatchList;
    NetEventMap m_netEventMap;
    
    shared_ptr<wxEvtHandler> m_pHandler;
};

#endif
