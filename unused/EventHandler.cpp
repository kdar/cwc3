#include <wx/tokenzr.h>

#include "EventHandler.h"
#include "Interface.h"
#include "Functions.h"
#include "Config.h"
#include "PingManager.h"
#include "PacketManager.h"

#include <wx/arrimpl.cpp> 
WX_DEFINE_OBJARRAY (EventList);

//===============================
Snapshot::Snapshot ()
{
  Reset ();
}

//===============================
void Snapshot::Reset ()
{
  bCreatingGame = false;
  bCreatedGame  = false;
  bStartingGame = false;
  bStartedGame  = false;
  bExitedGame = false;
  bEndedGame    = false;
}

//===============================
EventHandler::EventHandler ()
{
  m_pCommandHandler = new CommandHandler ();
}

//===============================
EventHandler::~EventHandler ()
{
  if (m_pCommandHandler)
    delete m_pCommandHandler;
}

//===============================
void EventHandler::AddEvent (Event *pEvent)
{
  m_eventList.Add (pEvent);
}

//===============================
void EventHandler::AddEvent (EventType type, u_char *pMatch, u_int nSize)
{
  Event *pEvent = new Event (type, pMatch, nSize);
  AddEvent (pEvent);
}

//===============================
void EventHandler::AddDefaultEvents ()
{
  u_char *pMatch;

  //evt_GameCreate
  pMatch = ValuesToString (2, 0xFF, 0x1C);
  AddEvent (evt_GameCreate, pMatch, 2);

  //evt_GameEnded
  pMatch = ValuesToString (2, 0xFF, 0x44);
  AddEvent (evt_GameEnded, pMatch, 2);

  //evt_GameStarted
  pMatch = ValuesToString (4, 0xF7, 0x0A, 0x04, 0x00);
  AddEvent (evt_GameStarted, pMatch, 4);

  //evt_GameExited
  pMatch = ValuesToString (9, 0xF7, 0x07, 0x09, 0x00, 0x01, 0x07, 0x00, 0x00, 0x00);
  AddEvent (evt_GameExited, pMatch, 9);

  //evt_GameLoading
  pMatch = ValuesToString (4, 0xF7, 0x0B, 0x04, 0x00);
  AddEvent (evt_GameLoading, pMatch, 4);

  //evt_CommandSend
  pMatch = ValuesToString (2, 0xFF, 0x0E);
  AddEvent (evt_CommandSend, pMatch, 2);

  //evt_HostAlone
  pMatch = ValuesToString (4, 0xFF, 0x02, 0x04, 0x00);
  AddEvent (evt_HostAlone, pMatch, 4);

  //evt_ChatSend
  pMatch = ValuesToString (2, 0xF7, 0x0F);
  AddEvent (evt_ChatSend, pMatch, 2);

  //evt_SlotTableInitial
  pMatch = ValuesToString (2, 0xF7, 0x04);
  AddEvent (evt_SlotTableInitial, pMatch, 2);

  //evt_SlotTableUpdated
  pMatch = ValuesToString (2, 0xF7, 0x09);
  AddEvent (evt_SlotTableUpdated, pMatch, 2);

  //evt_MapInfo
  pMatch = ValuesToString (2, 0xF7, 0x3D);
  AddEvent (evt_MapInfo, pMatch, 2);

  //evt_ClientInfo
  pMatch = ValuesToString (2, 0xF7, 0x06);
  AddEvent (evt_ClientInfo, pMatch, 2);

  //evt_ClientSlotKick
  pMatch = ValuesToString (8, 0xF7, 0x1C, 0x08, 0x00, 0x0D, 0x00, 0x00, 0x00);
  AddEvent (evt_ClientSlotKick, pMatch, 8);

  //evt_ClientConnectAttempt
  pMatch = ValuesToString (2, 0xF7, 0x1E);
  AddEvent (evt_ClientConnectAttempt, pMatch, 2);

  //evt_ClientLeft
  pMatch = ValuesToString (2, 0xF7, 0x21);
  AddEvent (evt_ClientLeft, pMatch, 2);

  //evt_ClientEnd
  pMatch = ValuesToString (2, 0xF7, 0x44);
  AddEvent (evt_ClientEnd, pMatch, 2);

  //evt_ClientMapStatus
  pMatch = ValuesToString (2, 0xF7, 0x42);
  AddEvent (evt_ClientMapStatus, pMatch, 2);

  //evt_InitiateMapUpload
  pMatch = ValuesToString (2, 0xF7, 0x3F);
  AddEvent (evt_InitiateMapUpload, pMatch, 2);
}

//===============================
void EventHandler::Process (const EventType type, const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  switch (type) {
    case evt_GameCreate:
      OnGameCreate (pData, nSize, pIp, pTcp);
    break;

    case evt_GameEnded:
      OnGameEnded (pData, nSize, pIp, pTcp);
    break;

    case evt_GameStarted:
      OnGameStarted (pData, nSize, pIp, pTcp);
    break;

    case evt_GameExited:
      OnGameExited (pData, nSize, pIp, pTcp);
    break;

    case evt_GameLoading:
      OnGameLoading (pData, nSize, pIp, pTcp);
    break;

    case evt_CommandSend:
      OnCommandSend (pData, nSize, pIp, pTcp);
    break;

    case evt_HostAlone:
      OnHostAlone (pData, nSize, pIp, pTcp);
    break;

    case evt_ChatSend:
      OnChatSend (pData, nSize, pIp, pTcp);
    break;

    case evt_SlotTableInitial:
      OnSlotTableInitial (pData, nSize, pIp, pTcp);
    break;

    case evt_SlotTableUpdated:
      OnSlotTableUpdated (pData, nSize, pIp, pTcp);
    break;

    case evt_MapInfo:
      OnMapInfo (pData, nSize, pIp, pTcp);
    break;

    case evt_ClientInfo:
      OnClientInfo (pData, nSize, pIp, pTcp);
    break;

    case evt_ClientSlotKick:
      OnClientSlotKick (pData, nSize, pIp, pTcp);
    break;

    case evt_ClientConnectAttempt:
      OnClientConnectAttempt (pData, nSize, pIp, pTcp);
    break;

    case evt_ClientLeft:
      OnClientLeft (pData, nSize, pIp, pTcp);
    break;

    case evt_ClientEnd:
      OnClientEnd (pData, nSize, pIp, pTcp);
    break;

    case evt_ClientMapStatus:
      OnClientMapStatus (pData, nSize, pIp, pTcp);
    break;

    case evt_InitiateMapUpload:
      OnInitiateMapUpload (pData, nSize, pIp, pTcp);
    break;
  }
}

//===============================
EventList EventHandler::GetEventList ()
{
  return m_eventList;
}

//===============================
Snapshot EventHandler::GetSnapshot ()
{
  return m_snapshot;
}

//===============================
void EventHandler::OnGameCreate (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  if (nSize > 24 && pData[8] == 0x00) {
    //We're creating a game
    //Note: This is actually a status update packet. This packet
    //gets sent multiple times to the server. I'm guessing so it
    //can update its game list. The only way to tell afaik, is that
    //if the 9th byte is 0, then it is a create game. If it has any
    //other value it's a game status update packet.
    //*******************************
    //Format:
    //FF 1C [size of data] 00 [private/public] 00 00 00 00 00 00 00 01 [??] 49 00
    //FF 03 00 00 00 00 00 00 [game name]
    //
    //private:  11
    //public:   10

    if (m_snapshot.bCreatingGame) return;

    //Offset 24 is the game name. It is terminated with 0.
    const u_char *pGameName = pData+24;;

    //Offset 4 is the game type
    char szGameType[32] = {0};
    if (pData[4] == 0x11)
      StringCopy (szGameType, "Private");
    else if (pData[4] == 0x10)
      StringCopy (szGameType, "Public");

    m_snapshot.Reset ();
    ClientManager::Get ()->ClearClients ();
    m_snapshot.bCreatingGame = true;
    PacketManager::Get ()->ClearSequences ();

    Interface::Get ()->Output (wxString::Format ("Game: Creating [%s] \"%s\".", szGameType, pGameName));
  } else {
    //We're checking game creation status (success/failed)
    //This does NOT only pertain to a successful/failed game creation.
    //Battle.net uses this same string to confirm other things as well.
    //E.g., if you refresh slots for example, a packet is sent to battle.net
    //with the game name and other information. If it was sent successfully, 
    //then a packet with the below binary string gets transmitted back to our server.
    //This is why I use the concept of events.
    //*******************************
    //Format:
    //FF 1C 08 00 [success/failure] 00 00 00
    //
    //success: 00
    //failure: 01

    //Only do this once.
    if (!m_snapshot.bCreatingGame) return;

    m_snapshot.Reset ();

    if (pData[4] == 0x00) {
      m_snapshot.bCreatedGame = true;
      Interface::Get ()->Output ("Success: Game created.");
    } else if (pData[4] == 0x01) {    
      Interface::Get ()->Output ("Failed: Game name already in use.");
    }
  } 
}

//===============================
void EventHandler::OnGameEnded (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //This packet gets sent twice in two different directions when you leave a game scoreboard (after the game ends).
  //The first packet gets sent by us to battle.net. The second is sent from battle.net to us.
  //I'm still not sure what kind of information is contained within these packets.
  //*******************************
  //Format (from us to battle.net):
  //FF 44 09 00 07 [??] 00 00 00
  //Format (from battle.net to us):
  //FF 44 1D 00 07 [??] 00 00 00 01 00 ....
  //
  //Note: The last 6 bytes in the first format, is repeated in the second format after the first 3 bytes.

  if (!m_snapshot.bCreatedGame || m_snapshot.bEndedGame || m_snapshot.bExitedGame)
    return;

  m_snapshot.Reset ();
  m_snapshot.bEndedGame = true;

  Interface::Get ()->Output ("Game: ended.");
}

//===============================
void EventHandler::OnGameStarted (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 0A 04 00

  if (m_snapshot.bStartingGame || m_snapshot.bStartedGame)
    return;

  m_snapshot.bStartingGame = true;

  Interface::Get ()->Output ("Game: started.");
}

//===============================
void EventHandler::OnGameExited (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 07 09 00 01 07 00 00 00

  if (!m_snapshot.bCreatedGame) 
    return;

  m_snapshot.Reset ();
  m_snapshot.bExitedGame = true;

  Interface::Get ()->Output ("Game: exited.");
}

//===============================
void EventHandler::OnGameLoading (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 0B 04 00

  if (!m_snapshot.bStartingGame)
    return;

  m_snapshot.bCreatingGame = false;
  m_snapshot.bStartingGame = false;
  m_snapshot.bStartedGame = true;
  m_snapshot.bExitedGame = false;

  Interface::Get ()->Output ("Game: loading.");
}

//===============================
void EventHandler::OnCommandSend (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format: 
  //FF 0E [size of data] 00 [/] [command]

  const u_char *pFullCommand = pData+5;

  m_pCommandHandler->Process (pFullCommand, pIp, pTcp);
}

//===============================
void EventHandler::OnHostAlone (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //This packet gets sent to battle.net when the host is alone and cancels/starts a game.
  //I'm not exactly sure of the relevance of this packet to battle.net, but it helps us a little.
  //*******************************
  //Format:
  //FF 02 04 00

  if (m_snapshot.bCreatedGame && ClientManager::Get ()->GetClientCount () == 0) {
    Interface::Get ()->Output ("Game: started/canceled and only host is there.");
  }
}

//===============================
void EventHandler::OnChatSend (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 0F [size of data] 00 [# of clients we send the msg to]
  //[client id] [client id] ... [??] 10 [chat msg]
}

//===============================
void EventHandler::OnSlotTableInitial (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //I'm still not exactly certain about this packet. Sometimes it can come with a full slot table,
  //and sometimes the slot table is missing. What's most important though, is that the ID of the
  //person we're sending this packet to is contained in here. Even though the packet size can
  //vary, the ID is always located 17 bytes from the end.
  //
  //*******************************
  //Format #1: 
  //F7 04 [size of data] 00 73 00 [number of entries]
  //
  //[The slot table described in OnSendUpdateSlotTable(), the inner 12 lines]
  //
  //[??] [??] [??] [??] [??] [??] [client id] 
  //[??] [??] [??] [??] [??] [??] [??] [??] 00 00 00 00 00 00 00 00
  //
  //Format #2:
  //F7 04 [size of data] 00 00 00 [client id] 
  //[??] [??] [??] [??] [??] [??] [??] [??] 00 00 00 00 00 00 00 00
  //
  //client id:  This is the client id of the client we're sending this packet to.

  //The id is at the reverse offset of 17
  int nId = static_cast<int>(*(pData+nSize-17));

  Client *pClient = ClientManager::Get ()->GetClientByIpPort (inet_ntoa (pIp->ip_dst), ntohs (pTcp->th_dport));
  if (!pClient) {
    Interface::Get ()->Output ("Freak error: We\'re somehow sending a initial slot table to a client not in the list.", OUTPUT_DEBUG);
    return;
  }

  //This is only true if he has already joined. I have had occurences where we send this packet twice to a client. So this effectively
  //stops outputting that the client joined twice.
  if (pClient->bConnected)
    return;

  pClient->nId = nId;
  pClient->bConnected = true;

  Interface::Get ()->Output (wxString::Format ("Joined: \"%s\" (%s)", pClient->sName.c_str (), pClient->sCountry.c_str ()));
  //CRefreshClientDisplay ();

  //Maybe we'll do an automatic kick later.
  bool bCountryFound = false;
  wxStringTokenizer tokenizer (ALLOWED_COUNTRIES, ',', wxTOKEN_STRTOK);
  while (tokenizer.HasMoreTokens ()) {
    const wxString sToken = tokenizer.GetNextToken ();
    
    if (sToken.IsSameAs (pClient->sCountry, false))
      bCountryFound = true;
  }

  if (!bCountryFound) {
    Interface::Get ()->Output (wxString::Format ("Warning: \"%s\" is not from an allowable country.", pClient->sName.c_str ()));
  }
}

//===============================
void EventHandler::OnSlotTableUpdated (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 09 [size of data] 00 73 00 [number of entries] 
  //[client id] [FF/64] [slot status] [kind] [team id] [slot id - 00] [??] 01 64 
  //...
  //number of entries times
  //...
  //5A 44 EA 06 03 0C
  //
  //number of entries: The number of entries in the table, 0C for 12 entries
  //client id:         00 for no one in the slot, the client's id if they are in this slot
  //kind:              00 for player, 01 for computer
  //teamid:            00 for nightelf/sentinel, 01 for undead/scourge
  //slot status:       00 for open, 01 for closed, 02 for occupied

  int nEntries_offset       = 6;
  int nEntries_start_offset = 7;
  int nEntry_size           = 9;

  int nId_offset            = 0;
  int nSlotstatus_offset    = 2;
  int nKind_offset          = 3;
  int nTeam_offset          = 4;
  int nSlot_offset          = 5;

  int nEntries = static_cast<int>(*(pData+nEntries_offset));
  
  for (int x = 0; x < nEntries; x++) {
    const u_char *pEntryIterate = pData + (x * nEntry_size + nEntries_start_offset);

    int nId = static_cast<int>(*(pEntryIterate+nId_offset)); 
    if (nId == 0) continue;

    int nSlotstatus = static_cast<int>(*(pEntryIterate+nSlotstatus_offset));
    int nKind       = static_cast<int>(*(pEntryIterate+nKind_offset));
    int nTeam       = static_cast<int>(*(pEntryIterate+nTeam_offset));
    int nSlot       = static_cast<int>(*(pEntryIterate+nSlot_offset));

    Client *pClient = ClientManager::Get ()->GetClientById (nId);
    if (!pClient) {
      Interface::Get ()->Output ("Freak error: We\'re somehow missing a client sent in the update slot table.", OUTPUT_DEBUG);
      return;
    }

    pClient->nTeam     = nTeam;
    pClient->nSlot     = nSlot;
    pClient->sColor    = g_asColorMap[nSlot];
    pClient->sTeamName = g_asTeamMap[nTeam];
  }

  //CRefreshClientDisplay ();
}


//===============================
void EventHandler::OnMapInfo (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 3D 3B 00 01 00 00 00 [Map location and name] 00 [3 byte map id]
}

//===============================
void EventHandler::OnClientInfo (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 06 [size of data] 00 [??] 00 00 00 
  //[client id] [client name] 00 01
  //
  //No slot info is sent in this.

  int nId_offset = 8;
  int nName_offset = 9;

  int nId = static_cast<int>(*(pData+nId_offset));
  const u_char *pName = pData+nName_offset;

  Client *pClient = ClientManager::Get ()->GetClientById (nId);
  if (!pClient) {
    //If we get this, this usually means this is the host's info. So we'll get add him.
    pClient = ClientManager::Get ()->AddClient (GAME_HOST, GAME_PORT);
  }

  pClient->sName = pName;
  pClient->nId = nId;
  pClient->bConnected = true;

  //CRefreshClientDisplay ();
}

//===============================
void EventHandler::OnClientSlotKick (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 1C 08 00 0D 00 00 00

  Client *pClient = ClientManager::Get ()->GetClientByIpPort (inet_ntoa (pIp->ip_dst), ntohs (pTcp->th_dport));
  if (!pClient) {
    Interface::Get ()->Output ("Freak error: We tried to slot kick someone not in the list.", OUTPUT_DEBUG);
    return;
  }

  ClientManager::Get ()->RemoveClient (pClient);

  Interface::Get ()->Output (wxString::Format ("Kicked: \"%s\" by slot.", pClient->sName.c_str ()));
  //CRefreshClientDisplay ();
}

//===============================
void EventHandler::OnClientConnectAttempt (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format: 
  //F7 1E [size of data] 00 [??] 00 00 00 00 00 00 00 00 
  //[??] [??] [??] 00 00 00 [client name]

  //We started the game, so we're not accepting anymore client connections.
  if (m_snapshot.bStartedGame || m_snapshot.bStartedGame)
    return;

  //Offset 19 is the client name. It is terminated with 0.
  const u_char *pClientName = pData+19;

  //Not sure if this is necessary, but just in case.
  Client *pClient = ClientManager::Get ()->GetClientByName (pClientName);
  if (pClient) {
    pClient->sIp = inet_ntoa (pIp->ip_src);
    pClient->nPort = ntohs (pTcp->th_sport);
  } else {
    pClient = ClientManager::Get ()->AddClient (inet_ntoa (pIp->ip_src), ntohs (pTcp->th_sport));
    pClient->sName = pClientName;
    pClient->nFlags = CF_NEW;

    PingManager::Get ()->Render (pClient);
  }
}

//===============================
void EventHandler::OnClientLeft (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 21 08 00 07 00 00 00

  Client *pClient = ClientManager::Get ()->GetClientByIpPort (inet_ntoa (pIp->ip_src), ntohs (pTcp->th_sport));
  if (!pClient) {
    Interface::Get ()->Output ("Freak error: A client left who wasn't in the list.", OUTPUT_DEBUG);
    return;
  }

  //If we started the game, just mark them as a leaver instead of removing them from the client list.
  if (m_snapshot.bStartedGame) {
    pClient->nFlags &= CF_LEFT;
  } else {
    ClientManager::Get ()->RemoveClient (pClient);
  }

  Interface::Get ()->Output (wxString::Format ("Left: \"%s\"", pClient->sName.c_str ()));
  //CRefreshClientDisplay ();
}

//===============================
void EventHandler::OnClientEnd (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 44 0E 00 [??] [??] [??] [??] [??] [??] [??] [??] [??] [??]

  Client *pClient = ClientManager::Get ()->GetClientByIpPort (inet_ntoa (pIp->ip_src), ntohs (pTcp->th_sport));
  if (!pClient) {
    Interface::Get ()->Output ("Freak error: A client ended who wasn't in the list.", OUTPUT_DEBUG);
    return;
  }

  //If we started the game, just mark them as kicked instead of removing them from the client list.
  if (m_snapshot.bStartedGame) {
    pClient->nFlags &= CF_KICKED;
  } else {
    ClientManager::Get ()->RemoveClient (pClient);
  }

  Interface::Get ()->Output (wxString::Format ("Kicked: \"%s\".", pClient->sName.c_str ()));
  //CRefreshClientDisplay ();
}


//===============================
void EventHandler::OnClientMapStatus (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 42 0D 00 01 00 00 00 01 [3 byte map id] 00
  //
  //Has map: The 3 byte map id is filled with the map id from evt_MapInfo
  //No map:  These 3 bytes are 0x00
}

//===============================
void EventHandler::OnInitiateMapUpload (const u_char *pData, u_int nSize, const net_ip *pIp, const net_tcp *pTcp)
{
  //*******************************
  //Format:
  //F7 3F 09 00 01 00 00 00 01

  Client *pClient = ClientManager::Get ()->GetClientByIpPort (inet_ntoa (pIp->ip_dst), ntohs (pTcp->th_dport));
  if (!pClient) {
    Interface::Get ()->Output ("Freak error: Uploading to a client who isn't in the list.", OUTPUT_DEBUG);
    return;
  }

  Interface::Get ()->Output (wxString::Format ("Map: uploading to \"%s\".", pClient->sName.c_str ()));
}
