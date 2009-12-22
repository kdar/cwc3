#ifndef _GAMEMANAGER_H
#define _GAMEMANAGER_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/thread.h>
#include <wx/datetime.h>

#include "Inet.h"
#include "Event.h"
#include "Info.h"

#include "Object.h"

//Forward declarations
class CommandHandler;
class AutoRefresh;
class NetEventDispatcher;
class PacketCapture;
class Client;
class Slot;

class PhraseModule;
class KickModule;
class BanModule;
class ExtraInfoModule;
class InjectModule;

//-------------------------------
//A section of data sent by battle.net
struct DataSection
{
  const u_char *pData;
  u_int nSize;
};

//-------------------------------
//The WC3 header. The payload follows this.
#pragma pack(1)
struct WC3_Hdr
{
  u_char chDestination;
  u_char chType;
  u_short nsSize;
};
#pragma pack()

typedef vector<DataSection> DataSectionArray;

//-------------------------------
class Game : public wxEvtHandler, public wxThread, public Object<Game>
{
  public:
    Game()
      : wxThread(wxTHREAD_DETACHED),
        m_bShutdown(false)
    {}
    ~Game() {}
    
    bool Initialize();
    void Shutdown();

    static shared_ptr<Game> Get();
    static shared_ptr<Game> Set(shared_ptr<Game> pInstance);

    bool TestShutdown();
    void ExitAndKill();
    
    void OnInterval();

    void ProcessPacket(u_char *pArgs, const struct pcap_pkthdr *pHeader, const u_char *pPacket);
    void ProcessNetInfo(const net_ip *pIp, const net_tcp *pTcp, const u_char *pPacket);
    void ProcessNodeInfo(const net_ip *pIp, const net_tcp *pTcp, const struct pcap_pkthdr *pHeader);
    void CheckDisconnected();
      
    shared_ptr<CommandHandler> GetCommandHandler() const
    { return m_pCommandHandler; }

  protected:
    DataSectionArray BreakUpData(const u_char *pData, const u_int nDataLen);
    bool IsParseableData(const u_char *pData, const u_int nDataLen) const;
    
    void DetermineRealm(const wxString &sIp);  

    //Actual Events. These are either sent by battlenet or us over the network.
    void OnGameCreate(NetEvent &e);
    void OnGameEnded(NetEvent &e);
    void OnGameStarted(NetEvent &e);
    void OnGameExited(NetEvent &e);
    void OnGameLoading(NetEvent &e);
    void OnCommandSend(NetEvent &e);
    void OnHostAlone(NetEvent &e);
    void OnSlotTableInitial(NetEvent &e);
    void OnSlotTableUpdated(NetEvent &e);
    void OnMapInfo(NetEvent &e);
    void OnClientInfo1(NetEvent &e);
    void OnClientInfo2(NetEvent &e);
    void OnClientSlotKick(NetEvent &e);
    void OnClientConnectAttempt(NetEvent &e);
    void OnClientLeftSent(NetEvent &e);
    void OnClientLeftReply(NetEvent &e);
    void OnClientEnd(NetEvent &e);
    void OnClientMapStatus(NetEvent &e);
    void OnInitiateMapUpload(NetEvent &e);
    void OnWhisper(NetEvent &e);
    void OnChatSend(NetEvent &e);
    void OnChatReceive(NetEvent &e);
    void OnJoinChannel(NetEvent &e);
    void OnLoginSuccessful(NetEvent &e);
    void OnClientFinishedLoading(NetEvent &e);
    void OnClientIdFinishedLoading(NetEvent &e);
    void OnSendBattlenetGameName(NetEvent &e);
    void OnGameFinishedLoading(NetEvent &e);

    //Virtual events. These events aren't sent by battlenet or us, but we use them
    //to signify an event occurred.
    void OnvClientDisconnected(NetEvent &e);
    
    DECLARE_EVENT_TABLE()

  private:
    virtual void *Entry();
    
    void GameEnded();
    void ClientLeft(shared_ptr<Client> pClient);    
    void PrintJoined(shared_ptr<Client> pClient);
    wxString GetGameRatio(shared_ptr<Slot> pSlot = shared_ptr<Slot>());

  private:
    static shared_ptr<Game> ms_pInstance;

    volatile bool m_bShutdown;
    
    shared_ptr<CommandHandler> m_pCommandHandler;
    shared_ptr<NetEventDispatcher> m_pNetEventDispatcher;   
    shared_ptr<AutoRefresh> m_pAutoRefresh;
    shared_ptr<PacketCapture> m_pPacketCapture;
    
    shared_ptr<PhraseModule> m_pPhraseModule;
    shared_ptr<KickModule> m_pKickModule;
    shared_ptr<BanModule> m_pBanModule;    
    shared_ptr<ExtraInfoModule> m_pExtraInfoModule;
    shared_ptr<InjectModule> m_pInjectModule;
    
    GameInfo *m_pGameInfo;
    NetInfo *m_pNetInfo;
    NodeInfoMap *m_pNodeInfoMap;
    RealmInfo *m_pRealmInfo;
};

#endif
