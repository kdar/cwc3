#ifndef _PLAYERINFOMODULE_H
#define _PLAYERINFOMODULE_H

#include "Module.h"
#include "Command.h"
#include "Event.h"

//Forward declarations
class Database;
class Client;

//-------------------------------
enum ExtraInfoClientFlagType
{
  EICF_CHECKED = 0x00000001
};

//-------------------------------
class ExtraInfoModule : public wxEvtHandler, public Module
{
  public:
    ExtraInfoModule()
      : Module()
    {}
    
    void Initialize(shared_ptr<Game> pGame);
    void Shutdown();
    
    static bool Check(shared_ptr<Client> pClient);
    
    static bool AddOrUpdateClient(shared_ptr<Client> pClient);
    static bool UpdateClient(shared_ptr<Client> pClient, const wxString &sComment);
    static bool UpdateClient(shared_ptr<Client> pClient, double dScore);
    
    static bool AddCurrentGame();
    static bool UpdateCurrentGame();
    
  protected:
    void InitConfig();
    bool InitDb();
    
    void OnClientConnectAttempt(NetEvent &e);
    void OnClientInfo1(NetEvent &e);
    void OnClientConnectInfoCommon(const char *pClientName);    
    
    void OnGameFinishedLoading(NetEvent &e);
    void OnGameEnded(NetEvent &e);
    void OnGameExited(NetEvent &e);
    
    DECLARE_EVENT_TABLE()
    
  private:
    shared_ptr<Game> m_pGame;

    static shared_ptr<Database> m_pDb;
    
    static bool m_bAddedInfos;
    static int m_nLastGameId;
};

//-------------------------------
//Commands
DECLARE_COMMAND(PlayerInfoCmd, "playerinfo")
DECLARE_COMMAND(CommentCmd, "comment")
DECLARE_COMMAND(RateCmd, "rate")

#endif
