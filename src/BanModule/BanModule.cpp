#include "BanModule/BanModule.h"
#include "KickModule/KickModule.h"
#include "PhraseModule/PhraseModule.h"
#include "CommandHandler.h"
#include "ClientManager.h"
#include "SlotManager.h"
#include "Interface.h"
#include "NetEventDispatcher.h"
#include "Game.h"
#include "Database.h"
#include "Functions.h"
#include "Netclip.h"
#include "Config.h"

shared_ptr<Database> BanModule::m_pDb;
ArrayString BanModule::m_tables;
wxString BanModule::m_local;
wxString BanModule::m_sLastBanned;

BEGIN_EVENT_TABLE(BanModule, wxEvtHandler)
  EVT_NET(NET_ClientInfo1, BanModule::OnClientInfo1)
END_EVENT_TABLE()

//===============================
void BanModule::Initialize(shared_ptr<Game> pGame)
{
  if (!pGame) return;
      
  m_pGame = pGame;
  
  shared_ptr<CommandHandler> pCmdHandler = pGame->GetCommandHandler();
  
  //Register commands to the command handler.
  if (pCmdHandler) {   
    BEGIN_REGISTER_COMMANDS(pCmdHandler);
  
    REGISTER_COMMAND(CheckCmd);
    REGISTER_COMMAND(BanCmd);
    REGISTER_COMMAND(EBanCmd);
    REGISTER_COMMAND(BanLastCmd);
    REGISTER_COMMAND(UnbanCmd);
    REGISTER_COMMAND(EUnbanCmd);
    REGISTER_COMMAND(UnbanLastCmd);
    
    END_REGISTER_COMMANDS();
  }
  
  wxEvtHandler *pNext = pGame->GetNextHandler();
  pGame->SetNextHandler(this);
  SetNextHandler(pNext);
  
  InitConfig();
  InitDb();
}

//===============================
void BanModule::Shutdown()
{
}

//===============================
void BanModule::InitConfig()
{
  shared_ptr<Config> p = Config::Get();
  p->GetString("Ban/DbFile", "local.db");
  p->GetBool("Ban/KickOnBan", true);
  p->GetString("Ban/DefaultPhrase", "leaver");
  
  p->GetString("Speech/PlayerBanned", "Player banned");
  p->GetString("Speech/PlayerIsBanned", /*"Banned player detected"*/"");
  
  const char *asDefaultPhraseKeys[] = {"leaver", "leaverno", "leaverdeath", "leaverfirst", "fleaver",
                                       "afk", "noob", "feeder", "itemdrop", "itemdestroy", 
                                       "rude", "inappropriate", "literal", "hacking", "lame"};
  const char *asDefaultPhraseValues[] = {"leaver ($ratio) $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "leaver ($ratio) w/o notification $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "leaver ($ratio) after death $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "leaver ($ratio) after first death $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "leaver (5v5*) $paramseparator -- 00:30:23 [$realm] ($date(%D)) (Banned by $myname)",
                                         "afk $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "noob $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "feeder | intentional feeding $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "itemdrop | intentional item drop $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "itemdestroy | intentional item destroy $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "rude | extreme rudeness and trash talking $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "inappropriate $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "$param -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "hacking $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)",
                                         "lame | extreme lameness $paramseparator -- $gametime [$realm] ($date(%D)) (Banned by $myname)"};
  
  p->GetMapByStrings("Ban/Phrases", sizeof(asDefaultPhraseKeys) / sizeof(char **), asDefaultPhraseKeys, asDefaultPhraseValues);
}

//===============================
bool BanModule::InitDb()
{
  wxString sFile = CONFIG(wxString, "Ban/DbFile"); 
  m_pDb = shared_ptr<Database>(new Database(":memory:"));  
  
  m_pDb->Exec("PRAGMA temp_store=2");
  //Load our database into memory.
  m_pDb->Exec(wxString::Format("ATTACH \"%s\" as local", sFile.c_str()).c_str());
  
  //Get all the database tables with the bans in them.
  m_pDb->Exec("SELECT name,type FROM config");
  
  //Get all the ban tables and put them into m_tables
  for (ArrayString::iterator i = m_pDb->vdata.begin(); i != m_pDb->vdata.end(); i++) {
    wxString s = *i;
    m_tables.push_back(s);
    if (i->IsSameAs("local", "false"))
      m_local = s;
  }
}

//===============================
/*static*/ bool BanModule::Check(shared_ptr<Client> pClient)
{
  m_pDb->Exec("BEGIN");
  
  for (ArrayString::iterator i = m_tables.begin(); i != m_tables.end(); i++) {
    wxString sQuery = wxString::Format("SELECT reason FROM %s WHERE lname=\"%s\" LIMIT 1", i->c_str(), pClient->sName.Lower().c_str());
    m_pDb->Exec(sQuery.c_str());
    
    if (m_pDb->vdata.size() > 0) {
      Interface::Get()->Output(wxString::Format("</B/32>Banned<!B!32>: \"</B>%s<!B>\" %s", pClient->sName.c_str(), m_pDb->vdata[0].c_str()));
      m_pDb->Exec("END");
      return true;
    }
  }
  
  m_pDb->Exec("END");
  
  return false;
}

//===============================
/*static*/ bool BanModule::Ban(shared_ptr<Client> pClient, const ArrayString &asParams)
{
  if (!pClient || pClient->sName.IsEmpty()) {
    Interface::Get()->Output("Not a valid client", OUTPUT_ERROR);
    return false;
  }
  
  bool bRet = false;
  
  wxString sDefaultPhrase = CONFIG(wxString, "Ban/DefaultPhrase");
  StrStrMap map = CONFIG(StrStrMap, "Ban/Phrases");
  wxString sPhrase;
  wxString sParam;
  int nAsParamsOffset = 1;
  
  if (asParams.size() > 0) {
    wxString sFoundPhrase;
    for (StrStrMap::iterator i = map.begin(); i != map.end(); i++) {
      //Interface::Get()->Output(wxString::Format("%s - %s - %s", asParams[0].c_str(), i->first.c_str(), i->second.c_str()));
      if (asParams[0].compare(i->first.c_str()) == 0)
        sFoundPhrase = i->second;
    }
    
    if (sFoundPhrase.IsEmpty()) {      
      nAsParamsOffset = 0;
    } else {
      sPhrase = sFoundPhrase;
    }
    
    ArrayString::const_iterator begin = asParams.begin() + nAsParamsOffset;
    ArrayString::const_iterator end = asParams.end();
    sParam = Join(begin, end);
  }
  
  if (sPhrase.IsEmpty()) {
    sPhrase = map[sDefaultPhrase];
  }
  
  sPhrase = PhraseModule::Parse(sPhrase, sParam, pClient);
  
  wxString sSelect = wxString::Format("SELECT reason FROM %s WHERE lname=\"%s\"", m_local.c_str(), pClient->sName.Lower().c_str());
  m_pDb->Exec(sSelect.c_str());
  if (m_pDb->vdata.size() > 0) {
    Interface::Get()->Output(wxString::Format("\"</B>%s<!B>\" is already banned: %s", pClient->sName.c_str(), m_pDb->vdata[0].c_str()), OUTPUT_ERROR);
  } else {  
    wxString sQuery = wxString::Format("INSERT INTO %s (lname, name, reason) VALUES (\"%s\", \"%s\", \"%s\")", m_local.c_str(), pClient->sName.Lower().c_str(), pClient->sName.c_str(), sPhrase.c_str());
    m_pDb->Exec(sQuery.c_str());
    
    Interface::Get()->Output(wxString::Format("</B/32>Banned<!B!32>: \"</B>%s<!B>\" %s", pClient->sName.c_str(), sPhrase.c_str()));
    bRet = true;
  }
    
  pClient->flags["ban"] |= BCF_CHECKED;
  m_sLastBanned = pClient->sName;
  
  return bRet;
}

//===============================
/*static*/ bool BanModule::Unban(shared_ptr<Client> pClient)
{
  if (!pClient || pClient->sName.IsEmpty()) {
    Interface::Get()->Output("Not a valid client", OUTPUT_ERROR);
    return false;
  }
  
  wxString sSelect = wxString::Format("SELECT reason FROM %s WHERE lname=\"%s\"", m_local.c_str(), pClient->sName.Lower().c_str());
  m_pDb->Exec(sSelect.c_str());
  if (m_pDb->vdata.size() > 0) {
    Interface::Get()->Output(wxString::Format("</B/32>Removed ban<!B!32>: \"</B>%s<!B>\" %s", pClient->sName.c_str(), m_pDb->vdata[0].c_str()));
    
    wxString sDelete = wxString::Format("DELETE FROM %s WHERE lname=\"%s\"", m_local.c_str(), pClient->sName.Lower().c_str());
    m_pDb->Exec(sDelete.c_str());    
    return true;
  } else {
    Interface::Get()->Output(wxString::Format("\"</B>%s<!B>\" was not banned", pClient->sName.c_str()), OUTPUT_ERROR);
  }
  
  return false;
}

//===============================
void BanModule::OnClientInfo1(NetEvent &e)
{
  e.Skip();
  
  //wxString sRefresherName = CONFIG(wxString, "AutoRefresh/Name");
  IntStrMap refresherNameMap = CONFIG(IntStrMap, "AutoRefresh/NameMap");
  bool bKick = CONFIG(bool, "Ban/KickOnBan");
  wxString sPlayerIsBanned = CONFIG(wxString, "Speech/PlayerIsBanned");
  ArrayString asExceptions = CONFIG(ArrayString, "AutoKick/Exceptions");
  
  const char *pName = reinterpret_cast<const char *>(e.m_pData) + 9;
  
  //if (sRefresherName.IsSameAs(pName, false))
  //  return;
  //Don't ban if this is the refresher
  for (IntStrMap::iterator i = refresherNameMap.begin(); i != refresherNameMap.end(); i++) {
    if (i->second.IsSameAs(pName, false))
      return; 
  }
  
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByName(pName);  
  if (!pClient)
    return;
    
  //--Check if this client is in the autokick exception list--
  bool bAutoKickException = false;
  for (int x = 0; x < asExceptions.size(); x++) {
    if (asExceptions[x].IsSameAs(pClient->sName, false))
      bAutoKickException = true;
  }
    
  if (!(pClient->flags["ban"] & BCF_CHECKED)) {
    if (BanModule::Check(pClient)) {
      Netclip::SendVoice(sPlayerIsBanned);
      if (Info::Get()->GetGameInfo().bHosted) {
        if (bKick && !bAutoKickException)
          KickModule::Kick(pClient, "Banned");
      }
    }
    
    pClient->flags["ban"] |= BCF_CHECKED;
  }
}

//-------------------------------
//Commands

//===============================
DEFINE_COMMAND(CheckCmd)
{
  if (asCmd.size() != 2) {
    Interface::Get()->Output("You must provide name to check.", OUTPUT_ERROR);
    return;
  }
  
  //Create a fake client to check
  shared_ptr<Client> pClient = shared_ptr<Client>(new Client());
  pClient->sName = asCmd[1];
  
  if (!BanModule::Check(pClient))
    Interface::Get()->Output(wxString::Format("\"</B>%s<!B>\" is not banned.", pClient->sName.c_str()));
}

//===============================
DEFINE_COMMAND(BanCmd)
{
  if (asCmd.size() < 2) {
    Interface::Get()->Output("You must provide a client to ban.", OUTPUT_ERROR);
    return;
  }
  
  wxString sPlayerBanned = CONFIG(wxString, "Speech/PlayerBanned");
  
  wxString sName = asCmd[1];
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByNameMatch(sName);
  
  //Remove the command and the client
  asCmd.erase(asCmd.begin(), asCmd.begin()+2);
  
  if (BanModule::Ban(pClient, asCmd)) {   
    Netclip::SendVoice(sPlayerBanned);
  }
}

//===============================
DEFINE_COMMAND(BanLastCmd)
{
  wxString sPlayerBanned = CONFIG(wxString, "Speech/PlayerBanned");
  
  //Get the last client that left.
  shared_ptr<Client> pClient;
  wxDateTime max;
  const ClientArray &clientArray = ClientManager::Get()->GetClientArray();
  for (int i = 0; i < clientArray.size(); i++) {
    shared_ptr<Client> p = clientArray[i];
    if (p->left.IsValid() && (!max.IsValid() || p->left.IsLaterThan(max))) {
      max = p->left;
      pClient = p;
    }
  }
  
  if (pClient) {
    //Remove the command
    asCmd.erase(asCmd.begin(), asCmd.begin()+1);
    
    if (BanModule::Ban(pClient, asCmd)) {      
      Netclip::SendVoice(sPlayerBanned);
    }
  } else {
    Interface::Get()->Output("Could not find a client to ban.", OUTPUT_ERROR);
  }
}

//===============================
//Explicit banning. Does not require a client list to work.
//Typing /eban ca <reason> would ban the nick "ca" with reason "<reason>"
DEFINE_COMMAND(EBanCmd)
{
  if (asCmd.size() < 2) {
    Interface::Get()->Output("You must provide a client to ban.", OUTPUT_ERROR);
    return;
  }
  
  //Create a fake client to ban.
  shared_ptr<Client> pClient = shared_ptr<Client>(new Client());
  pClient->sName = asCmd[1];
  
  //Remove the command and the client
  asCmd.erase(asCmd.begin(), asCmd.begin()+2);
  BanModule::Ban(pClient, asCmd);
}

//===============================
DEFINE_COMMAND(UnbanCmd)
{
  if (asCmd.size() != 2) {
    Interface::Get()->Output("You must provide name to unban.", OUTPUT_ERROR);
    return;
  }
  
  wxString sName = asCmd[1];
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByNameMatch(sName);
  
  BanModule::Unban(pClient);
}

//===============================
//Explicit unbanning. Does not require a client list to work.
DEFINE_COMMAND(EUnbanCmd)
{
  if (asCmd.size() != 2) {
    Interface::Get()->Output("You must provide name to unban.", OUTPUT_ERROR);
    return;
  }
  
  //Create a fake client to unban.
  shared_ptr<Client> pClient = shared_ptr<Client>(new Client());
  pClient->sName = asCmd[1];
  
  BanModule::Unban(pClient);
}

//===============================
DEFINE_COMMAND(UnbanLastCmd)
{
  //Create a fake client to unban.
  shared_ptr<Client> pClient = shared_ptr<Client>(new Client());
  pClient->sName = BanModule::GetLastBanned();
  
  BanModule::Unban(pClient);
}
