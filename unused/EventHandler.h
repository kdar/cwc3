#ifndef _EVENTHANDLER_H
#define _EVENTHANDLER_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/dynarray.h>

#include "Inet.h"
#include "Event.h"
#include "CommandHandler.h"
#include "ClientManager.h"

WX_DECLARE_OBJARRAY (Event*, EventList);

typedef void (TYPEOF_fnEventCallback)(const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);

//-------------------------------
//A class that defines the snapshot of our current status.
class Snapshot
{
  public:
    Snapshot ();
    void Reset ();

  public:
    bool bCreatingGame;
    bool bCreatedGame;
    bool bStartingGame;
    bool bStartedGame;
    bool bExitedGame;
    bool bEndedGame;
};

//-------------------------------
class EventHandler
{
  public:
    EventHandler ();
    ~EventHandler ();

    void AddEvent (Event *pEvent);
    void AddEvent (EventType type, u_char *pMatch, u_int nSize);
    void AddDefaultEvents ();

    void Process (EventType type, const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);

    EventList GetEventList ();
    Snapshot GetSnapshot ();

  protected:
    void OnGameCreate (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnGameEnded (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnGameStarted (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnGameExited (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnGameLoading (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnCommandSend (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnHostAlone (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnChatSend (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnSlotTableInitial (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnSlotTableUpdated (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnMapInfo (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnClientInfo (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnClientSlotKick (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnClientConnectAttempt (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnClientLeft (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnClientEnd (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnClientMapStatus (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);
    void OnInitiateMapUpload (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp);

  private:
    EventList m_eventList;
    Snapshot m_snapshot;
    CommandHandler *m_pCommandHandler;
};

#endif
