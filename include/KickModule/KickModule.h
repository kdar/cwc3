#ifndef _KICKMODULE_H
#define _KICKMODULE_H

#include "Module.h"
#include "Command.h"
#include "Event.h"

class Client;
class EnsureKick;

//-------------------------------
//A structure defining a client(or possibly clients if that ip belongs to
//more than one person) who which we are to auto kick if they join.
struct AutoKick
{
  wxString sIp;
  wxString sReason;
};

//-------------------------------
enum KickFlagType
{
  KICK_NONE = 0,
  KICK_FORCE
};

typedef vector<AutoKick> AutoKickList;

//-------------------------------
class KickModule : public wxEvtHandler, public Module
{
  public:
    KickModule();    
    ~KickModule();
    
    void Initialize(shared_ptr<Game> pGame);
    void Shutdown();
    
    static void Kick(const shared_ptr<Client> &pClient, char *szReason = 0, KickFlagType flags = KICK_NONE);
    
    const AutoKickList &GetAutoKickList() const
    { return m_autoKickList; }
    
  protected:
    void InitConfig();
    
    void AddAutoKick(const wxString &sIp, const wxString &sReason = "");
    void CheckKick(const net_ip *pIp, const net_tcp *pTcp);
    
    void OnSlotTableInitial(NetEvent &e);
    void OnClientSlotKick(NetEvent &e);
    void OnInitiateMapUpload(NetEvent &e);
    
    DECLARE_EVENT_TABLE()
    
  private:
    shared_ptr<Game> m_pGame;
    
    AutoKickList m_autoKickList;
};

//-------------------------------
//Commands
DECLARE_COMMAND(KickCmd, "kick")
DECLARE_COMMAND(FKickCmd, "fkick")
DECLARE_COMMAND(KickTeamCmd, "kickteam")
DECLARE_COMMAND(KickAllCmd, "kickall")
DECLARE_COMMAND(KickIpPortCmd, "kick_ipport")

#endif
