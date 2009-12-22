#ifndef _BANMODULE_H
#define _BANMODULE_H

#include "Module.h"
#include "Command.h"
#include "Event.h"

//Forward declarations
class Database;
class Client;

//-------------------------------
enum BanClientFlagType
{
  BCF_CHECKED = 0x00000001,
  BCF_BANNED  = 0x00000002
};

//-------------------------------
class BanModule : public wxEvtHandler, public Module
{
  public:
    BanModule()
    : Module()
    {}
    
    void Initialize(shared_ptr<Game> pGame);
    void Shutdown();
    
    static ArrayString &GetTables()
    { return m_tables; }
    
    static shared_ptr<Database> GetDatabase()
    { return m_pDb; }
    
    static bool Check(shared_ptr<Client> pClient);   
    
    static bool Ban(shared_ptr<Client> pClient, const ArrayString &asParams = ArrayString());
    static bool Unban(shared_ptr<Client> pClient);
    
    static wxString GetLastBanned()
    { return m_sLastBanned; }
    
  protected:
    void InitConfig();
    bool InitDb();
    
    void OnClientInfo1(NetEvent &e);
    
    DECLARE_EVENT_TABLE()
    
  private:
    shared_ptr<Game> m_pGame;

    static shared_ptr<Database> m_pDb;
    
    static ArrayString m_tables;
    static wxString m_local;
    
    static wxString m_sLastBanned;
};

//-------------------------------
//Commands
DECLARE_COMMAND(CheckCmd, "check")
DECLARE_COMMAND(BanCmd, "ban")
DECLARE_COMMAND(BanLastCmd, "banlast")
DECLARE_COMMAND(EBanCmd, "eban")
DECLARE_COMMAND(UnbanCmd, "unban")
DECLARE_COMMAND(EUnbanCmd, "eunban")
DECLARE_COMMAND(UnbanLastCmd, "unbanlast")

#endif
