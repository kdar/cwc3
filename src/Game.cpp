#include <stdlib.h>
#include <wx/tokenzr.h>

#include "Game.h"
#include "ClientManager.h"
#include "SlotManager.h"
#include "Interface.h"
#include "Defines.h"
#include "Config.h"
#include "AutoRefresh.h"
#include "CommandHandler.h"
#include "NetEventDispatcher.h"
#include "CoreCommands.h"
#include "PacketCapture.h"
#include "Netclip.h"

#include "PhraseModule/PhraseModule.h"
#include "KickModule/KickModule.h"
#include "BanModule/BanModule.h"
#include "ExtraInfoModule/ExtraInfoModule.h"
#include "InjectModule/InjectModule.h"

shared_ptr<Game> Game::ms_pInstance = shared_ptr<Game>();

//-------------------------------
//Callbacks
static void ProcessPacket(u_char *pArgs, const struct pcap_pkthdr *pHeader, const u_char *pPacket);
//-------------------------------

BEGIN_EVENT_TABLE(Game, wxEvtHandler)
  EVT_NET(NET_GameCreate, Game::OnGameCreate)
  EVT_NET(NET_GameEnded, Game::OnGameEnded)
  EVT_NET(NET_GameStarted, Game::OnGameStarted)
  EVT_NET(NET_GameExited, Game::OnGameExited)
  EVT_NET(NET_GameLoading, Game::OnGameLoading)
  EVT_NET(NET_CommandSend, Game::OnCommandSend)
  EVT_NET(NET_HostAlone, Game::OnHostAlone)
  EVT_NET(NET_SlotTableInitial, Game::OnSlotTableInitial)
  EVT_NET(NET_SlotTableUpdated, Game::OnSlotTableUpdated)
  EVT_NET(NET_MapInfo, Game::OnMapInfo)
  EVT_NET(NET_ClientInfo1, Game::OnClientInfo1)
  EVT_NET(NET_ClientInfo2, Game::OnClientInfo2)
  EVT_NET(NET_ClientSlotKick, Game::OnClientSlotKick)
  EVT_NET(NET_ClientConnectAttempt, Game::OnClientConnectAttempt)
  EVT_NET(NET_ClientLeftSent, Game::OnClientLeftSent)
  EVT_NET(NET_ClientLeftReply, Game::OnClientLeftReply)
  EVT_NET(NET_ClientEnd, Game::OnClientEnd)
  EVT_NET(NET_ClientMapStatus, Game::OnClientMapStatus)
  EVT_NET(NET_InitiateMapUpload, Game::OnInitiateMapUpload)
  EVT_NET(NET_Whisper, Game::OnWhisper)
  EVT_NET(NET_ChatSend, Game::OnChatSend)
  EVT_NET(NET_ChatReceive, Game::OnChatReceive)
  EVT_NET(NET_JoinChannel, Game::OnJoinChannel)
  EVT_NET(NET_LoginSuccessful, Game::OnLoginSuccessful)
  EVT_NET(NET_ClientFinishedLoading, Game::OnClientFinishedLoading)
  EVT_NET(NET_ClientIdFinishedLoading, Game::OnClientIdFinishedLoading)
  EVT_NET(NET_SendBattlenetGameName, Game::OnSendBattlenetGameName)
  EVT_NET(NET_GameFinishedLoading, Game::OnGameFinishedLoading)
  
  EVT_NET(NET_vClientDisconnected, Game::OnvClientDisconnected)
END_EVENT_TABLE()

//===============================
bool Game::Initialize()
{
  wxString sSniffDevice = CONFIG(wxString, "Network/SniffDevice");
  int nGamePort = CONFIG(int, "Network/GamePort");
  wxString sGameHost = CONFIG(wxString, "Network/GameHost");
  
  //------------------
  //Object creation
  
  m_pPacketCapture = shared_ptr<PacketCapture>(new PacketCapture());
  m_pCommandHandler = shared_ptr<CommandHandler>(new CommandHandler(CHT_NET));
  
  //Only needs to be done for one command handler. Its list is shared.
  RegisterCoreCommands(m_pCommandHandler);
  
  m_pNetEventDispatcher = shared_ptr<NetEventDispatcher>(new NetEventDispatcher(SharedPtr()));
  m_pAutoRefresh = shared_ptr<AutoRefresh>(new AutoRefresh(), NullDeleter());
  
  //Initialize modules
  m_pPhraseModule = shared_ptr<PhraseModule>(new PhraseModule());
  m_pKickModule = shared_ptr<KickModule>(new KickModule());
  m_pBanModule = shared_ptr<BanModule>(new BanModule());
  m_pExtraInfoModule = shared_ptr<ExtraInfoModule>(new ExtraInfoModule());
  m_pInjectModule = shared_ptr<InjectModule>(new InjectModule());
  m_pPhraseModule->Initialize(SharedPtr());
  m_pKickModule->Initialize(SharedPtr());
  m_pBanModule->Initialize(SharedPtr());
  m_pExtraInfoModule->Initialize(SharedPtr());
  m_pInjectModule->Initialize(SharedPtr());
  
  //Initialize some pointers to info structures. So we don't have to call Info::Get() all the time.
  m_pGameInfo = &Info::Get()->GetGameInfo();
  m_pNetInfo = &Info::Get()->GetNetInfo();
  m_pNodeInfoMap = &Info::Get()->GetNodeInfoMap();
  m_pRealmInfo = &Info::Get()->GetRealmInfo();
  
  //------------------
  //Thread stuff
  
  if (Create() != wxTHREAD_NO_ERROR) {
    Interface::Get()->Output("Could not create Game thread.", OUTPUT_ERROR);
    return false;
  }
  
  //------------------
  //Socket stuff
  
  if (!m_pPacketCapture->Init(sSniffDevice)) {
    Interface::Get()->Output(wxString::Format("Call to Init(%s) failed.", sSniffDevice.c_str()), OUTPUT_ERROR);
    return false;
  }

  if (!m_pPacketCapture->SetFilter(wxString::Format("tcp port %d and (src host %s or dst host %s)", nGamePort, sGameHost.c_str(), sGameHost.c_str()))) {
    Interface::Get()->Output(wxString::Format("Call to SetFilter(...) failed."), OUTPUT_ERROR);
    return false;
  }

  //if (!m_pPacketCapture->SetNonblock()) {
  //  Interface::Get()->Output(wxString::Format("Call to SetNonBlock() failed: %s", m_pPacketCapture->GetLastError().c_str()), OUTPUT_ERROR);
  //  return false;
  //}
  
  Netclip::Init();

  return true;
}

//===============================
void Game::Shutdown()
{
  m_pKickModule->Shutdown();
  m_pBanModule->Shutdown();
}

//===============================
/*static*/ shared_ptr<Game> Game::Get()
{
  if (!ms_pInstance)
    //We must not delete the memory allocated directly, as this is
    //a detatched thread and wxwidgets will take care of it.
    ms_pInstance = shared_ptr<Game>(new Game(), NullDeleter());
  return ms_pInstance;
}

//===============================
/*static*/ shared_ptr<Game> Game::Set(shared_ptr<Game> pInstance)
{
  shared_ptr<Game> pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
bool Game::TestShutdown()
{
  m_bShutdown = TestDestroy();
  return m_bShutdown;
}

//===============================
void Game::ExitAndKill()
{
  Kill();
}

//===============================
//Called from the main loop at a certain interval.
void Game::OnInterval()
{
  //Check if any clients have disconnected.
  CheckDisconnected();
}

//===============================
void *Game::Entry()
{
  while (!TestShutdown()) {
    m_pPacketCapture->Dispatch(-1, ::ProcessPacket, 0);

    //Somehow stops 100% cpu usage
    //We could probably use select() here instead to decide when to read the socket
    //and put a timeout on it so we can still shutdown. Not sure how portable usleep(0)
    //is and may have different effects on different kernels.
    Yield();
  }
  
  return 0;
}

//===============================
void Game::ProcessPacket(u_char *pArgs, const struct pcap_pkthdr *pHeader, const u_char *pPacket)
{
  int nIpOffset = m_pPacketCapture->GetDatalinkOffset();
  //You should not determine the length of the IP header or the TCP header using the size of the net_ip and
  //net_tcp structures. The reason is the size can actually change(Like for example if TCP options are appended
  //to the TCP header). So, you must use the macros IP_BYTE_HL and TH_BYTE_OFF respectively.
  const net_ip *pIp = reinterpret_cast<const net_ip *>(pPacket + nIpOffset);
  const net_tcp *pTcp = reinterpret_cast<const net_tcp *>(pPacket + nIpOffset + IP_BYTE_HL(pIp));
  const u_char *pData = (pPacket + nIpOffset + IP_BYTE_HL(pIp) + TH_BYTE_OFF(pTcp));

  ProcessNodeInfo(pIp, pTcp, pHeader);
  ProcessNetInfo(pIp, pTcp, pPacket);  
  //CheckDisconnected(pIp, pTcp);
  
  //If we sent a SYN packet to the game server, determine the realm.
  if (pTcp->th_flags == TH_SYN && strcmp(inet_ntoa(pIp->ip_dst), CONFIG(wxString, "Network/GameHost").c_str()) != 0)
    DetermineRealm(inet_ntoa(pIp->ip_dst));

  if (!(pTcp->th_flags & TH_ACK && pTcp->th_flags & TH_PUSH)) return;

  //Total packet length minus the total protocol headers' lengths = data size
  const u_int nDataLen = pHeader->len - (pData - pPacket);

  //if (!IsParseableData(pData, nDataLen)) return;

  DataSectionArray aData = BreakUpData(pData, nDataLen);
  for (DataSectionArray::iterator i = aData.begin(); i != aData.end(); i++) {
    m_pNetEventDispatcher->Process(pIp, pTcp, i->pData, i->nSize);
  }
}

//===============================
void Game::ProcessNetInfo(const net_ip *pIp, const net_tcp *pTcp, const u_char *pPacket)
{
  if (!m_pNetInfo->bInitialized) {
    wxString sGameHost = CONFIG(wxString, "Network/GameHost");
    m_pNetInfo->dataLink = m_pPacketCapture->DataLink();

    //We're dealing with an ethernet link layer
    if (m_pNetInfo->dataLink == DLT_EN10MB) {
      wxString sSrcIp = inet_ntoa(pIp->ip_src);
      wxString sDstIp = inet_ntoa(pIp->ip_dst);

      const net_ethernet *pEthernet = (net_ethernet *)pPacket;

      if (sSrcIp.IsSameAs(sGameHost)) {
        memcpy(m_pNetInfo->gamehost_ether, pEthernet->ether_shost, ETHER_ADDR_LEN);
      } else if (sDstIp.IsSameAs(sGameHost)) {
        memcpy(m_pNetInfo->gamehost_ether, pEthernet->ether_dhost, ETHER_ADDR_LEN);
      }
    }

    m_pNetInfo->bInitialized = true;
  }
}

//===============================
void Game::ProcessNodeInfo(const net_ip *pIp, const net_tcp *pTcp, const struct pcap_pkthdr *pHeader)
{
  //This method does a couple of things. First, we keep a hashmap of some basic packet/header info.
  //We store the last sequence number sent by our server to the user, that way we know the sequence
  //number to use when kicking someone.
  //The second part of this code calculates ping time based on the 3-way handshake of a TCP connection.
  //This is not the best way to do it, but it ensures it will work even if the user is behind a firewall.
  //It will give you a rough estimate of their real latency.
  wxString sGameHost = CONFIG(wxString, "Network/GameHost");

  wxString sSrcIp = inet_ntoa(pIp->ip_src);
  wxString sDstIp = inet_ntoa(pIp->ip_dst);
  if (sSrcIp.IsSameAs(sGameHost)) {
    wxString sKey = wxString::Format("%s:%d", inet_ntoa(pIp->ip_dst), ntohs(pTcp->th_dport));
    if (m_pNodeInfoMap->find(sKey) == m_pNodeInfoMap->end()) {
      //Interface::Get()->Output(wxString::Format("Adding key: %s", sKey));
      (*m_pNodeInfoMap)[sKey] = shared_ptr<NodeInfo>(new NodeInfo());
    }

    //If we sent a SYN+ACK packet, then set the time we sent it so we can calculate ping
    if (pTcp->th_flags & TH_SYN && pTcp->th_flags & TH_ACK) {
      //We use the timeval struct provided by pcap to calculate time. If we were to use
      //wxDataTime::UNow(), then it may be inaccurate, as pcap or the OS could queue packets.
      //I've had occurrences where some people get like 4 ping because of this.
      (*m_pNodeInfoMap)[sKey]->sent.Set(pHeader->ts.tv_sec);
      (*m_pNodeInfoMap)[sKey]->sent.SetMillisecond(pHeader->ts.tv_usec / 1000);
    }
    
    if (pTcp->th_flags == TH_RST) {
      (*m_pNodeInfoMap)[sKey]->bReceivedRst = true;
    }

    (*m_pNodeInfoMap)[sKey]->last_sentack = ntohl(pTcp->th_ack);
    (*m_pNodeInfoMap)[sKey]->last_sentseq = ntohl(pTcp->th_seq);
    (*m_pNodeInfoMap)[sKey]->last_sentwin = ntohs(pTcp->th_win);
  } else if (sDstIp.IsSameAs(sGameHost)) {
    wxString sKey = wxString::Format("%s:%d", inet_ntoa(pIp->ip_src), ntohs(pTcp->th_sport));
    if (m_pNodeInfoMap->find(sKey) == m_pNodeInfoMap->end()) {
      //Interface::Get()->Output(wxString::Format("Adding key: %s", sKey));
      (*m_pNodeInfoMap)[sKey] = shared_ptr<NodeInfo>(new NodeInfo());
    }

    //We're getting an ACK back from a client. Check if we pinged this client yet. If we didn't, then assume
    //that this is the packet we are looking for to calculate the ping.
    //Note: Sometimes we get their connect attempt packet (the one with their name) before we receive the
    //ack for the connection. So we can't simply do pTcp->th_flags === TH_ACK. So using & TH_ACK instead
    //this will run if we either receive an ack, or an ack-push packet.
    if (pTcp->th_flags & TH_ACK) {
      if (!(*m_pNodeInfoMap)[sKey]->bPinged) {
        if ((*m_pNodeInfoMap)[sKey]->sent.IsValid()) {
          wxDateTime n(pHeader->ts.tv_sec);
          n.SetMillisecond(pHeader->ts.tv_usec / 1000);
          
          (*m_pNodeInfoMap)[sKey]->ping = n.Subtract((*m_pNodeInfoMap)[sKey]->sent);
          (*m_pNodeInfoMap)[sKey]->bPinged = true;
        }
      }
    }
    
    if (pTcp->th_flags == TH_RST) {
      (*m_pNodeInfoMap)[sKey]->bSentRst = true;
    }

    (*m_pNodeInfoMap)[sKey]->lastResponse = wxDateTime::Now();

    (*m_pNodeInfoMap)[sKey]->last_recvack = ntohl(pTcp->th_ack);
    (*m_pNodeInfoMap)[sKey]->last_recvseq = ntohl(pTcp->th_seq);
    (*m_pNodeInfoMap)[sKey]->last_recvwin = ntohs(pTcp->th_win);
  }
}

//===============================
//Checks if a client disconnected and does the appropriate actions. It checks if
//their last response time is greater or equal to the client timeout
void Game::CheckDisconnected()
{
  //Only available if we are hosting.
  if (!m_pGameInfo->bHosted)
    return;
    
  int nClientTimeout = CONFIG(int, "Client/DisconnectMax");

  const ClientArray &clientArray = ClientManager::Get()->GetClientArray();
  wxString sKey;
  for (int i = 0; i < clientArray.size(); i++) {
    shared_ptr<Client> pClient = clientArray[i];
    //wxMutexLocker lock(pClient->mutex);

    //If they're not playing, then we don't do this. If we do, we will go into
    //an endless loop.
    if (!pClient->IsHere())
      continue;

    sKey = wxString::Format("%s:%d", pClient->sIp.c_str(), pClient->nPort);
    if (m_pNodeInfoMap->find(sKey) != m_pNodeInfoMap->end()) {
      if ((*m_pNodeInfoMap)[sKey]->lastResponse.IsValid()) {
        wxTimeSpan diff = wxDateTime::Now().Subtract((*m_pNodeInfoMap)[sKey]->lastResponse);
        if (diff.GetSeconds() >= nClientTimeout) {
          NetEvent e(NET_vClientDisconnected);
          e.m_pExtra = pClient;         
          ProcessEvent(e);
        }
      }
    }
  }
}

//===============================
//This is here to break up data when multiple packets are combined. For example, sometimes
//one packet can contain current users who are connected, the slot table, and other info.
//Or, sometimes each of these are contained within their own packet. Luckily WC3's protocol
//allows us to easily parse all of this data out.
//Warning: This data must be parseable before calling this function or the program will crash.
DataSectionArray Game::BreakUpData(const u_char *pData, const u_int nDataLen)
{
  DataSectionArray aData;
  u_int nTotal = 0;
  
  u_char *pSubData = const_cast<u_char *>(pData);
  int nDataLeft = nDataLen;
  while (nDataLeft > 3) {
    WC3_Hdr *pWC3Hdr = reinterpret_cast<WC3_Hdr *>(pSubData);
    u_short nReportedSize = pWC3Hdr->nsSize;
    //I REALLY don't understand why I don't need to convert from network byte order to host byte order, when
    //the reported size that comes on the line is in network byte order. Is there something I'm not understanding?
    //nReportedSize = ntohs(nReportedSize); //convert from network byte order

    if (nReportedSize < 3 || nReportedSize > nDataLen) break;

    DataSection sect;
    sect.pData = pSubData;
    sect.nSize = nReportedSize;
    aData.push_back(sect);
    
    nTotal += nReportedSize;

    pSubData += nReportedSize;
    nDataLeft -= nReportedSize;
  }

  return aData;
}

//===============================
//Data that we care about, basically.
bool Game::IsParseableData(const u_char *pData, const u_int nDataLen) const
{
  if (nDataLen < 4) return false;

  ArrayInt anDataPrefixes = CONFIG(ArrayInt, "Battlenet/DataPrefixes");

  for (u_int x = 0; x < anDataPrefixes.size(); x++) {
    if ((u_char)anDataPrefixes[x] == pData[0])
      return true;
  }

  return false;
}

//===============================
//Determines what realm we are on based on the IP passed and save it to
//the RealmInfo structure. It will only do this if the RealmInfo is not 
//initialized. The RealmInfo will be reset if we switch realms.
void Game::DetermineRealm(const wxString &sIp)
{
  if (!m_pRealmInfo->bInitialized) {    
    ArrayString asRealmHosts = CONFIG(ArrayString, "Realm/Hosts");
    
    regex e("(.*?)\\s*:\\s*(.*?)");
    cmatch what;
    for (ArrayString::iterator i = asRealmHosts.begin(); i != asRealmHosts.end(); i++) {
      if (regex_match(i->c_str(), what, e)) {
        string sName = what[1];
        string sHost = what[2];
        
        try {
          asio::io_service service;
        
          tcp::resolver resolver(service);
          tcp::resolver::query query(sHost.c_str(), "0");
          tcp::resolver::iterator ei = resolver.resolve(query);
          tcp::resolver::iterator end;
        
          while (ei != end) {
            asio::ip::address addr = ei->endpoint().address();
            if (sIp.IsSameAs(addr.to_string().c_str(), false)) {
              m_pRealmInfo->sRealmName = sName;
              m_pRealmInfo->sIp = sIp;
            }
            ei++;
          }
        } catch (std::exception& e) {
          //std::cout << "Exception: " << e.what() << "\n";
        }  
      }
    }
    
    m_pRealmInfo->bInitialized = true;
  }
}

//===============================
void Game::OnGameCreate(NetEvent &e)
{
  e.Skip();
  bool bAutoRefresh = CONFIG(bool, "AutoRefresh/Enabled");
  
  if (e.m_nDataSize > 24 && e.m_pData[8] == 0x00 && e.m_pData[9] == 0x00 && (e.m_pData[4] == 0x10 || e.m_pData[4] == 0x11)) {
    //We're creating a game
    //Note: This is actually a status update packet. This packet
    //gets sent multiple times to the server. I'm guessing so it
    //can update its game list. The only way to tell afaik, is that
    //if the 9th/10th/11th/12th byte is 0 and the 5th byte is 0x10 or 0x11 then it 
    //is a create game. If it has any other value it's a game status update packet.
    //*******************************
    //Format:
    //FF 1C [2 byte size of data] [private/public/full] 00 00 00 [4 byte timestamp] 01 [??] 49 00
    //FF 03 00 00 00 00 00 00 [game name] 00 00 [hosted count...]
    //
    //private:  11
    //public:   10

    if (m_pGameInfo->bCreatingGame) 
      return;
      
    m_pGameInfo->Reset();
    ClientManager::Get()->Reset();
    SlotManager::Get()->Reset();
    Info::Get()->ClearNodeInfoMap();

    //Offset 24 is the game name. It is terminated with 0.
    const u_char *pGameName = e.m_pData + 24;

    //Offset 4 is the game type
    char szGameType[32] = {0};
    if (e.m_pData[4] == 0x11) {
      strncpy(szGameType, "Private", sizeof(szGameType));
      m_pGameInfo->bPrivate = true;
    } else { //if (e.m_pData[4] == 0x10)
      strncpy(szGameType, "Public", sizeof(szGameType));
      m_pGameInfo->bPrivate = false;
    } /*else if (e.m_pData[4] == 0x12) {
      //This is part of the status update packet.
      //This is sent to the server telling it that this game is full.
    }*/

    m_pGameInfo->bCreatingGame = true;
    m_pGameInfo->sGameName = pGameName;
    m_pGameInfo->bHosted = true;
    
    //Get the hosted count. Needed as "authentication" for connecting
    //to a game with the auto refresher.
    //It is 8 bytes, but we need 9 for the null.
    char szHostedCount[9] = {0};
    int nSize = sizeof(szHostedCount) - 1;
    memcpy(szHostedCount, e.m_pData + m_pGameInfo->sGameName.Length() + 27, sizeof(szHostedCount));
    //It's in network byte order, so flip the string.
    for (int x = 0; x < nSize / 2; x++) {
      char hold = szHostedCount[x];
      szHostedCount[x] = szHostedCount[nSize-x-1];
      szHostedCount[nSize-x-1] = hold;       
    }
    //Use the strtoul function to convert it into number format.
    //It's in base 16.
    m_pGameInfo->nHostedCount = strtoul(szHostedCount, 0, 16);
    
    //The auto refresher needs to know of this hosted count to connect to the server.
    m_pAutoRefresh->SetHostedCount(m_pGameInfo->nHostedCount);
      
    //Determines the realm we are on based on the IP we send this to.
    DetermineRealm(inet_ntoa(e.m_pIp->ip_dst));

    Interface::Get()->Output(wxString::Format("</B/48>Game<!B!48>: Creating [</B>%s<!B>] \"</B/32>%s<!B!32>\".", szGameType, pGameName));
  } else if (e.m_pData[2] == 0x08 && e.m_pData[3] == 0x00) {
    //We're checking game creation status(success/failed)
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
    if (!m_pGameInfo->bCreatingGame || m_pGameInfo->bStartedGame) return;

    Interface::Get()->RefreshGameInfo();
    Interface::Get()->RefreshClientAll();

    if (e.m_pData[4] == 0x00) {
      m_pGameInfo->bCreatingGame = false;
      m_pGameInfo->bCreatedGame = true;
      Interface::Get()->Output("</N/48>Success<!N!48>: Game created.");
        
      //Start our autorefresh thread.
      //Only run it if it's a public game and it's enabled in the config.
      if (bAutoRefresh && !m_pGameInfo->bPrivate) {
        if (!m_pAutoRefresh->IsRunning())
          m_pAutoRefresh->Run();
        m_pAutoRefresh->Start();
      }
    } else if (e.m_pData[4] == 0x01) {
      Interface::Get()->Output("</N/48>Failed<!N!48>: Game name already in use.");
      m_pGameInfo->Reset();
    }
  }
}

//===============================
void Game::OnGameEnded(NetEvent &e)
{
  //This packet gets sent twice in two different directions when you leave a game scoreboard(after the game ends).
  //The first packet gets sent by us to battle.net. The second is sent from battle.net to us.
  //I'm still not sure what kind of information is contained within these packets.
  //*******************************
  //Format(from us to battle.net):
  //FF 44 09 00 07 [??] 00 00 00
  //Format(from battle.net to us):
  //FF 44 1D 00 07 [??] 00 00 00 01 00 ....
  //
  //Note: The last 6 bytes in the first format, is repeated in the second format after the first 3 bytes.
  e.Skip();

  GameEnded();
}

//===============================
void Game::OnGameStarted(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 0A 04 00
  e.Skip();
  
  if (*(e.m_pData + 2) != 0x04 || *(e.m_pData + 3) != 0x00)
    return;

  if (m_pGameInfo->bStartingGame || m_pGameInfo->bStartedGame)
    return;

  m_pGameInfo->bStartingGame = true;
  
  wxString sGameStarted = CONFIG(wxString, "Speech/GameStarted");
  
  //Fill in how many people are here per team.
  /*const SlotTable &table = SlotManager::Get()->GetSlotTable();
  int nTeamId = -1;
  for (int x = 0; x < table.size(); x++) {
    shared_ptr<Client> pClient = table[x]->pClient;
    int n = table[x]->nTeamId;
    if (n != nTeamId) {
      nTeamId = n;
      m_pGameInfo->herePerTeamCount[nTeamId] = 0;
    }
    
    if (pClient && pClient->IsHere()) {
      m_pGameInfo->herePerTeamCount[nTeamId]++;
      //Interface::Get()->Output(wxString::Format("Incremented team %d", nTeamId));
    }
  }*/
  
  //Update all of the clients' ratios.
  SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (SlotTable::iterator i = table.begin(); i != table.end(); i++) {
    if ((*i)->pClient) {
      (*i)->pClient->sRatio = GetGameRatio(*i);
    } 
  }  
  
  wxString sRatio = GetGameRatio();
  Interface::Get()->Output(wxString::Format("</B/48>Game<!B!48>: started [%s]", sRatio.c_str()));
  
  m_pAutoRefresh->Stop();
  
  Netclip::SendVoice(sGameStarted);
}

//===============================
void Game::OnGameExited(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 07 09 00 01 07 00 00 00
  e.Skip();
  
  GameEnded();
}

//===============================
void Game::OnGameLoading(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 0B 04 00
  e.Skip();
  
  if (*(e.m_pData + 2) != 0x04 || *(e.m_pData + 3) != 0x00)
    return;

  if (!m_pGameInfo->bStartingGame)
    return;

  m_pGameInfo->bCreatingGame = false;
  m_pGameInfo->bStartingGame = false;
  m_pGameInfo->bStartedGame  = true;
  m_pGameInfo->bExitedGame   = false;

  Interface::Get()->Output("</B/48>Game<!B!48>: loading.");

  //We started loading
  m_pGameInfo->startLoadTime = wxDateTime::Now();
}

//===============================
void Game::OnCommandSend(NetEvent &e)
{
  //*******************************
  //Format:
  //FF 0E [2 byte size of data] [/] [command]
  e.Skip();

  const u_char *pFullCommand = e.m_pData + 5;

  m_pCommandHandler->Process(pFullCommand);
}

//===============================
void Game::OnHostAlone(NetEvent &e)
{
  //This packet gets sent to battle.net when the host is alone and cancels/starts a game.
  //I'm not exactly sure of the relevance of this packet to battle.net, but it helps us a little.
  //*******************************
  //Format:
  //FF 02 04 00
  e.Skip();
  
  if (*(e.m_pData + 2) != 0x04 || *(e.m_pData + 3) != 0x00)
    return;

  if (m_pGameInfo->bCreatedGame && ClientManager::Get()->GetClientCount() == 0) {
    Interface::Get()->Output("</B/48>Game<!B!48>: started/canceled and only host is there.");
  }
  
  m_pAutoRefresh->Stop();
}

//===============================
void Game::OnSlotTableInitial(NetEvent &e)
{
  //I'm still not exactly certain about this packet. Sometimes it can come with a full slot table,
  //and sometimes the slot table is missing. What's most important though, is that the ID of the
  //person we're sending this packet to is contained in here. Even though the packet size can
  //vary, the ID is always located 17 bytes from the end.
  //
  //*******************************
  //Format #1:
  //F7 04 [2 byte size of data] [??] 00 [number of entries]
  //
  //[The slot table described in OnSendUpdateSlotTable(), the inner 12 lines]
  //
  //[??] [??] [??] [??] [??] [??] [client id]
  //[??] [??] [??] [??] [??] [??] [??] [??] 00 00 00 00 00 00 00 00
  //
  //Format #2:
  //F7 04 [2 byte size of data] 00 00 [client id]
  //[??] [??] [??] [??] [??] [??] [??] [??] 00 00 00 00 00 00 00 00
  //
  //client id:  This is the client id of the client we're sending this packet to.
  e.Skip();

  //The id is at the reverse offset of 17
  int nId = static_cast<int>(*(e.m_pData + e.m_nDataSize - 17));

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_dst), ntohs(e.m_pTcp->th_dport));
  if (!pClient) {
    //Interface::Get()->Output("Failed to get client in OnSlotTableInitial.", OUTPUT_ERROR);
    return;
  }

  pClient->nId = nId;

  //This is only true if he has already joined. I have had occurrences where we send this packet twice to a client. So this effectively
  //stops outputting that the client joined twice.
  if (SlotManager::Get()->TableContains(pClient))
    return;

  PrintJoined(pClient);
  //if (pClient->flags["core"] & CF_NEW)
  //  pClient->flags["core"] ^= CF_NEW;
  pClient->flags["core"] = CF_OLD;
}

//===============================
void Game::OnSlotTableUpdated(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 09 [2 byte size of data] [??] 00 [number of entries]
  //[client id] [propagated] [slot status] [kind] [team id] [slot id - 00] [race] [level of difficulty] 64
  //...
  //number of entries times
  //...
  //[??] [??] [??] [??] [??] [??]
  //
  //number of entries:   The number of entries in the table, 0C for 12 entries
  //client id:           00 for no one in the slot, the client's id if they are in this slot
  //propagated:          FF if not propagated to every other client, 64 otherwise (I think)
  //kind:                00 for player, 01 for computer
  //teamid:              00 for nightelf/sentinel, 01 for undead/scourge
  //slot status:         00 for open, 01 for closed, 02 for occupied
  //slot id:             The ID of the slot. This is static(Sometimes initialized to [number of entries] before client joins)
  //level of difficulty: 00 for Easy, 01 for Normal, 02 for Insane. It is also set to 01 if it's a player character.
  //race:                An ID uniquely identifying a race. 04 is human and 08 is undead for dota.
  e.Skip();
  
  IntStrMap difficultyMap = CONFIG(IntStrMap, "Client/DifficultyMap");

  int nEntries_offset       = 6;
  int nEntries_start_offset = 7;
  int nEntry_size           = 9;

  int nId_offset            = 0;
  int nPropagated_offset    = 1;
  int nSlotstatus_offset    = 2;
  int nKind_offset          = 3;
  int nTeamId_offset        = 4;
  int nSlotId_offset        = 5;
  int nRace_offset          = 6;
  int nLevel_offset         = 7;
  int nDontKnow_offset      = 8;
  
  //Clear the here per team count map, as we will calculate it below
  m_pGameInfo->herePerTeamCount.clear();

  u_char nEntries = static_cast<u_char>(*(e.m_pData + nEntries_offset));
  SlotManager::Get()->SetMaxSlots(nEntries);

  for (int x = 0; x < nEntries; x++) {
    const u_char *pEntryIterate = e.m_pData + (x * nEntry_size + nEntries_start_offset);

    int nId         = static_cast<int>(*(pEntryIterate + nId_offset));
    int nPropagated = static_cast<int>(*(pEntryIterate + nPropagated_offset));
    int nSlotStatus = static_cast<int>(*(pEntryIterate + nSlotstatus_offset));
    int nKind       = static_cast<int>(*(pEntryIterate + nKind_offset));
    int nTeamId     = static_cast<int>(*(pEntryIterate + nTeamId_offset));
    int nSlotId     = static_cast<int>(*(pEntryIterate + nSlotId_offset));
    int nRace       = static_cast<int>(*(pEntryIterate + nRace_offset));
    int nLevel      = static_cast<int>(*(pEntryIterate + nLevel_offset));
    int nDontKnow   = static_cast<int>(*(pEntryIterate + nDontKnow_offset));
    
    int &ratio = m_pGameInfo->herePerTeamCount[nTeamId];

    //Sometimes the packet gets "bugged" and sends half of the slot table
    //and the rest is 0'ed out. So if the slot id is 0, and also the last
    //part of this slot table entry is 0, then just continue.
    if (nSlotId == 0 && nDontKnow == 0)
      return;

    shared_ptr<Client> pClient = ClientManager::Get()->GetClientById(nId);
    shared_ptr<Slot> pSlot = SlotManager::Get()->GetOrCreateSlot(nSlotId);
    if (pSlot) {
      pSlot->nSlotStatus = nSlotStatus;
      pSlot->nKind       = nKind;
      pSlot->nTeamId     = nTeamId;
      pSlot->nRace       = nRace;
      pSlot->nLevel      = nLevel;

      if (nKind == KIND_HUMAN && nSlotStatus == STATUS_OCCUPIED) {
        ratio++;
        if (pClient) {
          pSlot->pClient = pClient;
          pSlot->sDisplay = pClient->sName;          
        }        
      } else {
        switch (nSlotStatus) {
          case STATUS_OCCUPIED:
            if (nKind == KIND_COMPUTER) {
              pSlot->sDisplay = "Computer ";
              pSlot->sDisplay += difficultyMap[nLevel];
              pSlot->pClient.reset();
            }
            break;

          case STATUS_OPEN:
            pSlot->sDisplay = "Open";
            pSlot->pClient.reset();
            break;

          case STATUS_CLOSED:
            pSlot->sDisplay = "Closed";
            pSlot->pClient.reset();
            break;
        }
      }
    }
  }

  Interface::Get()->RefreshClientAll();
}


//===============================
void Game::OnMapInfo(NetEvent &e)
{  
  //*******************************
  //Format:
  //F7 3D [2 byte size of data] 01 00 00 00 [Map location and name] 00 [3 byte map id]
  e.Skip();
  
  if (m_pGameInfo->sMapName.IsEmpty()) {
    int nMap_offset = 8;

    const u_char *pMap = e.m_pData + nMap_offset;
    m_pGameInfo->sMapName = pMap;

    int nBackSlashIndex = m_pGameInfo->sMapName.Find('\\', true);
    if (nBackSlashIndex != -1) {
      m_pGameInfo->sMapName = m_pGameInfo->sMapName.Mid(nBackSlashIndex + 1);
      
      m_pGameInfo->sShortMapName = m_pGameInfo->sMapName;
      m_pGameInfo->sShortMapName.Replace(".w3x", "");
      m_pGameInfo->sShortMapName.Replace(".w3m", "");
    }

    Interface::Get()->RefreshMapInfo();
  }
}

//===============================
void Game::OnClientInfo1(NetEvent &e)
{
  //This is sent by the server.
  //*******************************
  //Format:
  //F7 06 [2 byte size of data] [4 byte handle]
  //[1 byte client id] [client name] 00 
  //01 00 02 00        (might be race and something?)
  //[2 byte host port] (the port they configured to host with in the settings)
  //[12 byte external ip] (obvious with ipv4.. only the first 4 bytes are used)
  //02 00
  //[2 byte host port]
  //[12 byte internal ip]
  //No slot info is sent in this.  
  e.Skip();

  wxString sGameHost = CONFIG(wxString, "Network/GameHost");
  int nGamePort = CONFIG(int, "Network/GamePort");

  int nId_offset   = 8;
  int nName_offset = 9;

  int nId = static_cast<int>(*(e.m_pData + nId_offset));
  const u_char *pName = e.m_pData + nName_offset;

  if (nId == 0) return;
  
  //shared_ptr<Client> pClient = ClientManager::Get()->GetClientById(nId);
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByName(pName);
  
  //If we get this packet for this client, then we know he is still alive and
  //hasn't left. So reset the left count for this client.
  if (pClient)   
    pClient->nLeftCount = 0;
  
  //This is the case where we did not find the client, and the id is the host's
  if (!pClient && nId == ID_HOST) {
    pClient = ClientManager::Get()->AddClient(pName);
    pClient->nId = nId;
    pClient->sIp = inet_ntoa(e.m_pIp->ip_src);
    pClient->nPort = ntohs(e.m_pTcp->th_sport);
    pClient->flags["core"] = CF_OLD;
    pClient->flags["core"] |= CF_HOST;
    
    m_pGameInfo->sHostName = pName;
    if (m_pGameInfo->bHosted)
      m_pGameInfo->sMyName = pName;
    
    ClientManager::Get()->FillAdditionalInfo(pClient);
    PrintJoined(pClient);
  } 
  
  //This is the case where they sent us a ClientInfo2 packet, so we have their ip, port,
  //and id, but no name
  else if ((pClient && pClient->flags["core"] & CF_NEW) && !m_pGameInfo->bHosted) {
    pClient->sName = pName;
    pClient->nId = nId;
    pClient->flags["core"] = CF_OLD;    
    PrintJoined(pClient);
  } 
  
  //This is the case where we didn't find the client, or we did and they aren't here, and
  //we are not hosting a game.
  else if ((!pClient || !pClient->IsHere()) && !m_pGameInfo->bHosted) {
    pClient = ClientManager::Get()->AddClient(pName);
    pClient->nId = nId;
    pClient->flags["core"] = CF_OLD;    
    PrintJoined(pClient);
  }
  
  //Interface::Get()->RefreshClientDisplay();
}

//===============================
void Game::OnClientInfo2(NetEvent &e)
{
  //This is sent by each individual client. So we can map their id to their ip:port.
  //This can be sent multiple times if this client wants to change their port. So we
  //must update the port if this occurs.
  //*******************************
  //Format:
  //F7 37 [2 byte size of data] [??] 00 00 00 00 00 00 00 [client id] [??] [??] [team id] 00 00
  //
  //client id: The id of the client sending this packet
  //team id: I 'think' this has team id info. 02 for the first team, 03 for the second.
  e.Skip();
  
  char *pszIp = inet_ntoa(e.m_pIp->ip_src);
  int nPort = ntohs(e.m_pTcp->th_sport);

  int nId = static_cast<int>(*(e.m_pData + 12));

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientById(nId);
  if (SlotManager::Get()->TableContains(pClient)) {
    pClient->nId = nId;
    pClient->sIp = pszIp;
    pClient->nPort = nPort;    
  } else {
    pClient = ClientManager::Get()->AddClient(pszIp, nPort);
    pClient->nId = nId;
    pClient->flags["core"] = CF_NEW;
  }
  
  ClientManager::Get()->FillAdditionalInfo(pClient);
  
  //Get their ping
  wxString sKey = wxString::Format("%s:%d", pszIp, nPort);
  if (m_pNodeInfoMap->find(sKey) != m_pNodeInfoMap->end())
    pClient->ping = (*m_pNodeInfoMap)[sKey]->ping;
}

//===============================
void Game::OnClientSlotKick(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 1C 08 00 0D 00 00 00
  e.Skip();
  
  if (memcmp(e.m_pData + 2, "\x08\x00\x0D\x00\x00\x00", 6) != 0)
    return;

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_dst), ntohs(e.m_pTcp->th_dport));
  if (!pClient) {
    return;
  }

  //If they aren't in the table, return
  if (!SlotManager::Get()->TableContains(pClient))
    return;

  if (!pClient->sName.IsEmpty()) {
    Interface::Get()->Output(wxString::Format("</N/56>Left<!N!56>: \"</B>%s<!B>\" by slot kick", pClient->sName.c_str()));
  }

  Info::Get()->RemoveNodeInfoEntry(pClient->sIp, pClient->nPort);
  pClient->Left();
  if (!m_pGameInfo->bStartedGame)
    SlotManager::Get()->RemoveClient(pClient);
  Interface::Get()->RefreshClientAll();
}

//===============================
void Game::OnClientConnectAttempt(NetEvent &e)
{
  //Sent by the client
  //*******************************
  //Format:
  //F7 1E [2 byte size of data] [nHostedCount] 00 00 00 [4 byte unique id by server] 00
  //[2 byte port] [4 byte client<->client communication handle/descriptor] [client name]
  e.Skip();
  
  wxString sGameHost = CONFIG(wxString, "Network/GameHost");
  //wxString sRefresherName = CONFIG(wxString, "AutoRefresh/Name");
  IntStrMap refresherNameMap = CONFIG(IntStrMap, "AutoRefresh/NameMap");
  
  char *pszIp = inet_ntoa(e.m_pIp->ip_src);
  int nPort = ntohs(e.m_pTcp->th_sport);
  
  //Offset 19 is the client name. It is terminated with 0.
  const char *pClientName = reinterpret_cast<const char *>(e.m_pData) + 19;

  //If this occurs, then that means we joined a game
  if (sGameHost.IsSameAs(pszIp)) {
    m_pGameInfo->Reset();

    m_pGameInfo->bCreatedGame = true;

    ClientManager::Get()->Reset();
    SlotManager::Get()->Reset();
    Info::Get()->ClearNodeInfoMap();
      
    m_pGameInfo->sMyName = pClientName;
  }
  
  //We started the game, so we're not accepting anymore client connections.
  if (m_pGameInfo->bStartedGame || m_pGameInfo->bStartedGame)
    return;
  
  //Don't process this client if it's the auto refresher.
  //if (sRefresherName.IsSameAs(pClientName))
  //  return;
  for (IntStrMap::iterator i = refresherNameMap.begin(); i != refresherNameMap.end(); i++) {
    if (i->second.IsSameAs(pClientName, false))
      return; 
  }
    
  //FIXME:
  //This may be temporary, but sometimes during game creation I don't think it gets the nHostedCount correctly. 
  //We fix the problem by getting the correct nHostedCount from a client connect.
  int nHostedCount = static_cast<int>(*(e.m_pData + 4));
  if (nHostedCount != m_pGameInfo->nHostedCount) {
    Interface::Get()->Output("Failed to get initial nHostedCount", OUTPUT_WARNING);
    m_pGameInfo->nHostedCount = nHostedCount;
    //Fix the nHostedCount for the auto refresher
    m_pAutoRefresh->SetHostedCount(nHostedCount);
  }

  //Not sure if this is necessary, but just in case.
  //Client *pClient = ClientManager::Get()->GetClientByIpPort(pszIp, nPort);
  //shared_ptr<Client> pClient = ClientManager::Get()->GetClientByName(pClientName);
  //if (!pClient)
  shared_ptr<Client> pClient = ClientManager::Get()->AddClient(pClientName);

  pClient->sName = pClientName;
  pClient->flags["core"] = CF_NEW;
  pClient->sIp = pszIp;
  pClient->nPort = nPort;
  pClient->nId = 0;

  //Must be done after we know as much as possible about the client.
  ClientManager::Get()->FillAdditionalInfo(pClient);

  //Interface::Get()->Output(wxString::Format("Attempt connect from: \"%s\" (%s:%d).", pClient->sName.c_str(), pszIp, nPort), OUTPUT_DEBUG);

  wxString sKey = wxString::Format("%s:%d", pszIp, nPort);
  if (m_pNodeInfoMap->find(sKey) != m_pNodeInfoMap->end())
    pClient->ping = (*m_pNodeInfoMap)[sKey]->ping; //Put their ping in their client data
}

//===============================
void Game::OnClientLeftSent(NetEvent &e)
{
  //Sent by the client telling us he left.
  //*******************************
  //Format:
  //F7 21 08 00 07 00 00 00
  e.Skip();

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_src), ntohs(e.m_pTcp->th_sport));
  if (!pClient) {
    return;
  }

  ClientLeft(pClient);
}

//===============================
void Game::OnClientLeftReply(NetEvent &e)
{
  //A reply from whomever ClientLeftSent was sent to, acknowledging that
  //they are leaving
  //*******************************
  //Format:
  //F7 1B 04 00
  e.Skip();
  
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_dst), ntohs(e.m_pTcp->th_dport));
  if (!pClient) {
    return;
  }
  
  //Interface::Get()->Output(wxString::Format("Leaving acknowledged for \"%s\"", pClient->sName.c_str()));
}

//===============================
void Game::OnClientEnd(NetEvent &e)
{
  //Sent by the server to inform the receiver that the client id in the packet left.
  //*******************************
  //Format:
  //F7 07 [2 byte size of data] [client id] [not sure] 00 00 00
  //
  //client id: The id of the client who ended. If this is equal to 1, that means the host left and
  //           the game ended.
  //not sure:  Maybe it's how many clients are receiving this message?
  e.Skip();

  int nId_offset      = 4;
  int nNotSure_offset = 5;

  int nId = static_cast<int>(*(e.m_pData + nId_offset));
  int nNotSure = static_cast<int>(*(e.m_pData + nNotSure_offset));
  
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientById(nId);
  if (!pClient) {
    return;
  }
  
  ClientLeft(pClient);
}

//===============================
void Game::OnClientMapStatus(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 42 0D 00 01 00 00 00 01 [3 byte map id] 00
  //
  //Has map: The 3 byte map id is filled with the map id from evt_MapInfo
  //No map:  These 3 bytes are 0x00  
  e.Skip();
}

//===============================
void Game::OnInitiateMapUpload(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 3F 09 00 01 00 00 00 01
  e.Skip();

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_dst), ntohs(e.m_pTcp->th_dport));
  if (!pClient) {
    return;
  }

  //If they aren't in the table, return
  if (!SlotManager::Get()->TableContains(pClient))
    return;

  //We know they are downloading already, so just return.
  if (pClient->flags["core"] & CF_DOWNLOADING)
    return;

  pClient->flags["core"] |= CF_DOWNLOADING;

  Interface::Get()->Output(wxString::Format("</B>Map<!B>: uploading to \"</B>%s<!B>\".", pClient->sName.c_str()));
}

//===============================
void Game::OnWhisper(NetEvent &e)
{
  //Even though it's a whisper, all whispers are relayed through battle.net. So you receive this packet from battle.net.
  //*******************************
  //Format:
  //FF 0F [2 byte size of data] [type] 00 00 00 00 00 00 00 [??] 00 00 00
  //00 00 00 00 [??] [??] [??] [??] [??] [??] [??] [??] [client name] 00 [message] 00
  //
  //type: 0x04 for client whisper, 0x07 and 0x0E for you joining a channel, 0x01 for channel ID(I think)
  //      0x0A for your whisper.
  //client name: The client's name.
  //message: The message sent.
  e.Skip();

  if (*(e.m_pData + 4) != 0x04 && *(e.m_pData + 4) != 0x0A) return;

  //Offset 28 is the client name. It is terminated with 0.
  const u_char *pClientName = e.m_pData + 28;

  const u_char *pMessage = (const u_char *)(pClientName + strlen((const char *)pClientName) + 1);

  if (*(e.m_pData + 4) == 0x04)
    Interface::Get()->Output(wxString::Format("</B/24>W> [%s]<!B!24>: %s", pClientName, pMessage));
  else if (*(e.m_pData + 4) == 0x0A)
    Interface::Get()->Output(wxString::Format("</B/24><W [%s]<!B!24>: %s", pClientName, pMessage));
}

//===============================
void Game::OnChatSend(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 0F [2 byte size of data] [count]
  //[client id] ... [sender id] 0x10 [message]
  //count:     Number of clients this message will get sent to.
  //client id: The id of a client that this message will get sent to. This repeats
  //           a 'count' number of times.
  //sender id: The id of the sender of this message
  //message:   The message sent.
  e.Skip();

  //There is another packet that has the same signature, but this value
  //is 0x11 instead of 0x10. Not sure what that packet is. Maybe a 'ping'?
  if (*(e.m_pData + e.m_nDataSize - 1) == 0x11) return;

  u_int nClients = static_cast<u_int>(*(e.m_pData + 4));

  //I haven't implemented this yet because this gets called multiple times (If we're hosting) because
  //the message has to be relayed to everyone. My suggestion is we maintain a list of chat messages.
  //This first time this is called, it is added to the list with the message, the sender, and the number [count].
  //Every consecutive call to this from the same person and message, will decrement that [count] variable in
  //the list. As long as that message is in the list, we do not display it in cwc3. Once the count goes to
  //0 we remove it from the list. This effectively will only show the message once in cwc3 even though
  //it is repeatedly sent.
}

//===============================
void Game::OnChatReceive(NetEvent &e)
{
  //*******************************
  //Format:
  //F7 28 [2 byte size of data] [count]
  //[client id] ... [sender id] 10 [message]
  //count:     Number of clients this message will get sent to.
  //client id: The id of a client that this message will get sent to. This repeats
  //           a 'count' number of times.
  //sender id: The id of the second of this message
  //message:   The message sent.  
  e.Skip();
}

//===============================
void Game::OnJoinChannel(NetEvent &e)
{
  //*******************************
  //Format:
  //FF 0C [2 byte size of data] [something] 00 00 00 [channel name]
  //
  //channel name: Obviously the name of the channel you joined
  //something: This packet gets sent twice sometimes, but I only catch a channel join if this is 0x00.
  e.Skip();

  if (*(e.m_pData + 4) != 0x00) return;

  const u_char *pChannel = e.m_pData + 8;

  Interface::Get()->Output(wxString::Format("</B>Joined channel<!B>: </32>%s<!32>", pChannel));
}

//===============================
//FIXME: I really don't think this is a login successful packet. But it works I guess.
void Game::OnLoginSuccessful(NetEvent &e)
{
  //Sent by the server, telling us our name after a successful login
  //*******************************
  //Format:
  //FF 0A [2 byte size of data] [your name] [PX3W 0] [your name]
  e.Skip();
  
  m_pRealmInfo->Reset();
  DetermineRealm(inet_ntoa(e.m_pIp->ip_src));
  
  if (!m_pRealmInfo->sRealmName.IsEmpty())
    Interface::Get()->Output(wxString::Format("</B>Logged in<!B>: realm [%s]", m_pRealmInfo->sRealmName.c_str()));
}

//===============================
void Game::OnClientFinishedLoading(NetEvent &e)
{
  //Sent by the client himself to the server.
  //*******************************
  //Format:
  //F7 23 04 00
  e.Skip();
  
  if (*(e.m_pData + 2) != 0x04 || *(e.m_pData + 3) != 0x00)
    return;

  //Client *pClient = ClientManager::Get()->GetClientByIpPort(inet_ntoa(e.m_pIp->ip_src), ntohs(e.m_pTcp->th_sport));
  //Interface::Get()->Output(wxString::Format("</N/56>Finished Loading<!N!56>: %s", pClient->sName.c_str()));
}

//===============================
void Game::OnClientIdFinishedLoading(NetEvent &e)
{
  //Sent by the server to every other client when
  //OnClientFinishedLoading() occurs
  //*******************************
  //Format:
  //F7 08 05 00 [client id]
  e.Skip();
  
  if (*(e.m_pData + 2) != 0x05 || *(e.m_pData + 3) != 0x00)
    return;

  shared_ptr<Client> pClient = ClientManager::Get()->GetClientById(*(e.m_pData + 4));
  if (pClient && !(pClient->flags["core"] & CF_LOADED)) {
    //How long it took this client to load the game.
    if (m_pGameInfo->startLoadTime.IsValid())
      pClient->loadTime = wxDateTime::Now().Subtract(m_pGameInfo->startLoadTime);
    
    pClient->flags["core"] |= CF_LOADED;
    //Interface::Get()->Output(wxString::Format("</N/56>Finished Loading<!N!56>: %s", pClient->sName.c_str()));
    Interface::Get()->RefreshClientDisplay();
  }
}

//===============================
void Game::OnSendBattlenetGameName(NetEvent &e)
{
  //This is sent to battlenet when you join someone else's game.
  //*******************************
  //Format:
  //FF 22 [2 byte size of data] 00 00 00 00 [??]
  //00 00 00 [game name] 00
  e.Skip();

  if (m_pGameInfo->sGameName.IsEmpty()) {
    m_pGameInfo->sGameName = e.m_pData + 12;
    Interface::Get()->RefreshGameInfo();

    Interface::Get()->Output(wxString::Format("</B/48>Game<!B!48>: Joined \"</B/32>%s<!B!32>\".", m_pGameInfo->sGameName.c_str()));
  }
  
  //Determine the realm we are on based on the IP we send this to.
  DetermineRealm(inet_ntoa(e.m_pIp->ip_dst));
}

//===============================
void Game::OnGameFinishedLoading(NetEvent &e)
{
  //Sent by the server to everyone. Note: Not really a game loaded packet.
  //It is the delta compression packet that is sent constantly.
  //*******************************
  //Format:
  //F7 0C [2 byte size of data] [??] 00
  e.Skip();

  if (!m_pGameInfo->bLoadedGame && m_pGameInfo->bStartedGame) {
    wxString sGameLoaded = CONFIG(wxString, "Speech/GameLoaded");

    //The game is officially going.
    m_pGameInfo->startTime = wxDateTime::Now();

    m_pGameInfo->bLoadedGame = true;

    Interface::Get()->Output(wxString::Format("</B/48>Game<!B!48>: loaded"));
      
    Netclip::SendVoice(sGameLoaded);
  }
}

//===============================
void Game::OnvClientDisconnected(NetEvent &e)
{
  e.Skip();

  shared_ptr<Client> pClient = static_pointer_cast<Client>(e.m_pExtra);
  if (!pClient) {
    return;
  }

  //Force the client to leave in ClientLeft() by setting its left count
  //greater or equal to the amount of clients still here.
  pClient->nLeftCount = 12;
  
  ClientLeft(pClient);
}

//===============================
void Game::GameEnded()
{
  if (!m_pGameInfo->bCreatedGame ||  m_pGameInfo->bEndedGame || m_pGameInfo->bExitedGame)
    return;

  m_pGameInfo->bExitedGame = true;

  //Calculate endgame info
  m_pGameInfo->endTime = wxDateTime::Now();
  if (m_pGameInfo->startTime.IsValid() && m_pGameInfo->endTime.IsValid())
    m_pGameInfo->gameDuration = m_pGameInfo->endTime.Subtract(m_pGameInfo->startTime);

  if (m_pGameInfo->bStartedGame)
    Interface::Get()->Output(wxString::Format("</B/48>Game<!B!48>: ended (</B>%s<!B>).", m_pGameInfo->gameDuration.Format("%H:%M:%S").c_str()));
  else
    Interface::Get()->Output(wxString::Format("</B/48>Game<!B!48>: ended."));
  
  m_pAutoRefresh->Stop(); 
}

//===============================
void Game::ClientLeft(shared_ptr<Client> pClient)
{
  if (m_pGameInfo->bEndedGame || m_pGameInfo->bExitedGame) return;
    
  int nDisconnectThreshold = CONFIG(int, "Client/DisconnectThreshold");
  wxString sPlayerLeft = CONFIG(wxString, "Speech/PlayerLeft");
  
  //If they already aren't here, then return (if not, 'Left' will get spammed multiple times)
  if (!pClient->IsHere())
    return;
  
  shared_ptr<NodeInfo> pNodeInfo = Info::Get()->GetNodeInfo(pClient->sIp, pClient->nPort);
  
  //Ok, I will explain that the heck this is. (Note: Only a problem with hosting)
  //Let's give this scenario:
  /*1. Joe joins, gets id 10
    2. Joe leaves
    3. Because Joe left, his slot is empty
    4. Jim joins and fills the slot Joe was in, and he gets id 10
    5. WC3 sends to some other client (say with id 5) in the game that:
       1. Client 10 left (Joe, packet: 0xF7 0x07)
       2. The new slot table with that empty slot (Joe, packet: 0xF7 0x09)
       3. A new client joined (Jim, id 10, packet: 0xF7 0x06)
       4. New slot table, with Jim now in it (packet: 0xF7 0x09)
    
    But, cwc3 thinks that Jim has left because of this. Since Joe left and Jim gets id 10, and
    WC3 sends to some other client that client 10 left AFTER Jim has joined and been assigned
    the id 10, then cwc3 thinks that Jim left (since Joe is already not here).
    
    If you think this is really ackward for WC3 to do this, think about it this way. Every client only
    learns about other clients through the 0xF7 0x06 packet, which contains the client's name and their id.
    So, when the client, say Johnny, receives this entire packet, Jim hasn't even joined yet. It tells Johnny
    that Joe has left, the new slot table with him gone, that Jim has join, and now the new slot table with
    Jim in it. But this causes problems for us when we are hosting, because we see ALL of these packets, not
    just what the client sees. So we have to account for all of the duplication and "out of order" sequence of packets.
    I say "out of order" because of this situation, sending to all the clients except one that Joe left (id 10), then Jim
    joins (id 10) and sending that last client that Joe left (id 10) and Jim joined (id 10), making Jim (id 10) seemed to have left.
    
    My fix for this is to only say a client left, if the 0xF7 0x07 packet containing a certain client, is
    sent to at least 50% of the clients currently in the game. We keep a nLeftCount in each object.
    With this implemented and using the scenario above, Jim would only have a nLeftCount of 1, which would
    not be 50% of all the clients currently in the game. So he would not have said to have left.
    
    As a precaution, in OnClientInfo1, nLeftCount is reset for the client that is in that packet, because
    if we send that packet as the server, we know the client is still alive. This helps eliminate any 
    problems with incrementation not being unique (we increment nLeftCount for every packet, not for
    every unique client we send it to)*/
    
  //Interface::Get()->Output(wxString::Format("%s: %s - %s", pClient->sName.c_str(), pNodeInfo->bReceivedRst ? "true" : "false",  pNodeInfo->bSentRst ? "true" : "false"));
    
  if (m_pGameInfo->bHosted && (pNodeInfo && !pNodeInfo->bReceivedRst && !pNodeInfo->bSentRst)) {
    pClient->nLeftCount++;
    int nPercentage = 0;
    int c1 = pClient->nLeftCount;
    //Only interested in clients that are here.
    int c2 = SlotManager::Get()->GetHereClientCount();
    nPercentage = static_cast<int>((static_cast<double>(c1) / c2) * 100);
    //Interface::Get()->Output(wxString::Format("%s - %d [%d] (%d)", pClient->sName.c_str(), c1, c2, nPercentage));
    if (nPercentage < 50)
      return;
  }
  
  //Check if they were disconnected, or just left (host only).
  bool bDisconnected = false;
  if (m_pGameInfo->bHosted) {
    wxString sKey = wxString::Format("%s:%d", pClient->sIp.c_str(), pClient->nPort);
    if (m_pNodeInfoMap->find(sKey) != m_pNodeInfoMap->end()) {
      if ((*m_pNodeInfoMap)[sKey]->lastResponse.IsValid()) {
        wxTimeSpan diff = wxDateTime::Now().Subtract((*m_pNodeInfoMap)[sKey]->lastResponse);
        if (diff.GetSeconds() >= nDisconnectThreshold) {
          bDisconnected = true;
        }
      }
    }
  }
  
  shared_ptr<Slot> pSlot = SlotManager::Get()->GetSlot(pClient);
  //wxString sRatio = GetGameRatio(pSlot);
  //pClient->sRatio = sRatio;
  
  //Update all the client's ratios that are _still here_
  //If you update every client, then you set incorrect ratios for clients who already left.
  SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (SlotTable::iterator i = table.begin(); i != table.end(); i++) {
    if ((*i)->pClient && (*i)->pClient->IsHere()) {
      (*i)->pClient->sRatio = GetGameRatio(*i);
    } 
  } 
  
  //Now decrement the amount of players currently playing for that team
  if (pSlot)
    m_pGameInfo->herePerTeamCount[pSlot->nTeamId]--;
  
  if (bDisconnected) {
    Interface::Get()->Output(wxString::Format("</N/56>Disconnected<!N!56>: \"</B>%s<!B>\"", pClient->sName.c_str()));
    pClient->Disconnected();
  } else {
    Interface::Get()->Output(wxString::Format("</N/56>Left<!N!56>: \"</B>%s<!B>\"", pClient->sName.c_str()));
    
    if ((m_pGameInfo->bStartingGame || m_pGameInfo->bStartedGame) && 
        !m_pGameInfo->sMyName.IsSameAs(pClient->sName, false)) {
      Netclip::SendVoice(sPlayerLeft); 
    }
  }

  Info::Get()->RemoveNodeInfoEntry(pClient->sIp, pClient->nPort);
  pClient->Left();
  if (!m_pGameInfo->bStartedGame)
    SlotManager::Get()->RemoveClient(pClient);
  Interface::Get()->RefreshClientAll();
}

//===============================
void Game::PrintJoined(shared_ptr<Client> pClient)
{
  int nPing = pClient->GetPing();
  wxString sOutput = wxString::Format("</N/56>Joined<!N!56>: \"</B>%s<!B>\"", pClient->sName.c_str());
  if (!pClient->sCountry.IsEmpty())
    sOutput += wxString::Format(" (</B/24>%s<!B!24>)", pClient->sCountry.c_str());
  if (nPing)
    sOutput += wxString::Format("[</B/32>%d<!B!32>]", nPing);
  
  Interface::Get()->Output(sOutput);
}

//===============================
wxString Game::GetGameRatio(shared_ptr<Slot> pSlot)
{
  //Determine the current game ratio, and set it in the client.
  wxString sRatio;
  for (IntIntMap::iterator i = m_pGameInfo->herePerTeamCount.begin(); i != m_pGameInfo->herePerTeamCount.end(); i++) {   
    sRatio += wxString::Format("%d", i->second);
    if (pSlot && pSlot->nTeamId == i->first)
      sRatio += "*";
    if (next(i) != m_pGameInfo->herePerTeamCount.end())
      sRatio += "v";
  }
  
  return sRatio;
}

//*******************************
//Callbacks
//*******************************

//===============================
void ProcessPacket(u_char *pArgs, const struct pcap_pkthdr *pHeader, const u_char *pPacket)
{
  Game::Get()->ProcessPacket(pArgs, pHeader, pPacket);
}
