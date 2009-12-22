#include "ExtraInfoModule/ExtraInfoModule.h"
#include "PhraseModule/PhraseModule.h"
#include "Game.h"
#include "Database.h"
#include "Config.h"
#include "Netclip.h"
#include "CommandHandler.h"
#include "ClientManager.h"
#include "SlotManager.h"
#include "Interface.h"
#include "Functions.h"
#include "NetEventDispatcher.h"

shared_ptr<Database> ExtraInfoModule::m_pDb;
bool ExtraInfoModule::m_bAddedInfos = false;
int ExtraInfoModule::m_nLastGameId = 0;
  
BEGIN_EVENT_TABLE(ExtraInfoModule, wxEvtHandler)
  EVT_NET(NET_ClientConnectAttempt, ExtraInfoModule::OnClientConnectAttempt)
  EVT_NET(NET_ClientInfo1, ExtraInfoModule::OnClientInfo1)
  EVT_NET(NET_GameFinishedLoading, ExtraInfoModule::OnGameFinishedLoading)
  EVT_NET(NET_GameEnded, ExtraInfoModule::OnGameEnded)
  EVT_NET(NET_GameExited, ExtraInfoModule::OnGameExited)
END_EVENT_TABLE()

//===============================
void ExtraInfoModule::Initialize(shared_ptr<Game> pGame)
{
  if (!pGame) return;
      
  m_pGame = pGame;
  
  shared_ptr<CommandHandler> pCmdHandler = pGame->GetCommandHandler();
  
  //Register commands to the command handler.
  if (pCmdHandler) {   
    BEGIN_REGISTER_COMMANDS(pCmdHandler);
  
    REGISTER_COMMAND(PlayerInfoCmd);
    REGISTER_COMMAND(CommentCmd);
    REGISTER_COMMAND(RateCmd);
    
    END_REGISTER_COMMANDS();
  }
  
  wxEvtHandler *pNext = pGame->GetNextHandler();
  pGame->SetNextHandler(this);
  SetNextHandler(pNext);
  
  InitConfig();
  InitDb();
}

//===============================
void ExtraInfoModule::Shutdown()
{
}

//===============================
void ExtraInfoModule::InitConfig()
{
  shared_ptr<Config> p = Config::Get();
  p->GetString("ExtraInfo/DbFile", "local.db");
  p->GetString("Speech/PlayerHasInfo", "");
}

//===============================
bool ExtraInfoModule::InitDb()
{
  wxString sFile = CONFIG(wxString, "ExtraInfo/DbFile"); 
  m_pDb = shared_ptr<Database>(new Database(":memory:"));  
  
  m_pDb->Exec("PRAGMA temp_store=2");
  //Load our database into memory.
  m_pDb->Exec(wxString::Format("ATTACH \"%s\" as local", sFile.c_str()).c_str());
}

//===============================
/*static*/ bool ExtraInfoModule::Check(shared_ptr<Client> pClient)
{  
  wxString sQuery = wxString::Format("SELECT comment,games,ratings,score,firstseen,lastseen FROM playerinfo WHERE lname=\"%s\" LIMIT 1", pClient->sName.Lower().c_str());
  m_pDb->Exec(sQuery.c_str());
  
  if (m_pDb->vdata.size() > 0) {
    wxString s = wxString::Format("</B/40>Info<!B!40>: \"</B>%s<!B>\" </B>Games<!B>[%s] </B>Score<!B>[%s] </B>Seen<!B>[%s]",
      pClient->sName.c_str(), 
      m_pDb->vdata[1].c_str(),
      m_pDb->vdata[3].c_str(),
      m_pDb->vdata[5].c_str());
    
    if (!m_pDb->vdata[0].IsEmpty())
      s += wxString::Format(" </B>Comment<!B>[%s]", m_pDb->vdata[0].c_str());
        
    Interface::Get()->Output(s);
    
    //Format this for the client's extra info
    wxString sInfo = wxString::Format("Games[%s] Score[%s]", m_pDb->vdata[1].c_str(), m_pDb->vdata[3].c_str());
    if (!m_pDb->vdata[0].IsEmpty())
      sInfo += wxString::Format(" Comment[%s]", m_pDb->vdata[0].c_str());
    pClient->extraInfo["info"] = sInfo;
    return true;
  }
  
  return false;
}

//===============================
/*static*/ bool ExtraInfoModule::AddOrUpdateClient(shared_ptr<Client> pClient)
{
  if (!pClient || pClient->sName.IsEmpty()) {
    Interface::Get()->Output("Not a valid client", OUTPUT_ERROR);
    return false;
  }
  
  //Notice we don't care if this fails or not. We will update afterwards.
  wxString sQuery = wxString::Format("INSERT INTO playerinfo (lname, name, firstseen, lastseen, lastgameid) VALUES (\"%s\", \"%s\", DATETIME('NOW'), DATETIME('NOW'), %d)", 
    pClient->sName.Lower().c_str(), 
    pClient->sName.c_str(),
    m_nLastGameId);
    
  m_pDb->Exec(sQuery.c_str());
  
  //I just update here because the only other way that I can think of doing this is to
  //check if this entry exists or not. And that would just be more overhead.
  wxString sUpdate = wxString::Format("UPDATE playerinfo SET lastseen=DATETIME('now', 'localtime'), games=games+1, lastgameid=%d where lname=\"%s\"", 
    m_nLastGameId,
    pClient->sName.Lower().c_str());
  m_pDb->Exec(sUpdate.c_str());
  
  return true;
}

//===============================
/*static*/ bool ExtraInfoModule::UpdateClient(shared_ptr<Client> pClient, const wxString &sComment)
{
  if (!pClient || pClient->sName.IsEmpty()) {
    Interface::Get()->Output("Not a valid client", OUTPUT_ERROR);
    return false;
  }
  
  wxString sUpdate = wxString::Format("UPDATE playerinfo SET comment=\"%s\" where lname=\"%s\"", sComment.c_str(), pClient->sName.Lower().c_str());
  m_pDb->Exec(sUpdate.c_str());
  
  return true;
}

//===============================
/*static*/ bool ExtraInfoModule::UpdateClient(shared_ptr<Client> pClient, double dScore)
{
  if (!pClient || pClient->sName.IsEmpty()) {
    Interface::Get()->Output("Not a valid client", OUTPUT_ERROR);
    return false;
  }
  
  wxString sQuery = wxString::Format("SELECT score FROM playerinfo WHERE lname=\"%s\"", pClient->sName.Lower().c_str());
  m_pDb->Exec(sQuery.c_str());
  
  if (m_pDb->vdata.size() > 0) {
    double dTmp = 0.0;
    m_pDb->vdata[0].ToDouble(&dTmp);
    if (dTmp == 0.0)
      dTmp = dScore;
    dTmp = (dTmp+dScore)/2;
    
    wxString sUpdate = wxString::Format("UPDATE playerinfo SET ratings=ratings+1,score=%f WHERE lname=\"%s\"", dTmp, pClient->sName.Lower().c_str());
    m_pDb->Exec(sUpdate.c_str());
    
    return true;
  }
  
  return false;
}

//===============================
/*static*/ bool ExtraInfoModule::AddCurrentGame()
{
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();
  const RealmInfo &realmInfo = Info::Get()->GetRealmInfo();  
    
  int nClients = 12;
  
  wxString sInsert = wxString::Format("INSERT INTO gameinfo (realm, mapname, gamename,"
                     "duration, gamestart, myname, hostname, slot1,"
                     "slot2, slot3, slot4, slot5, slot6, slot7, slot8,"
                     "slot9, slot10, slot11, slot12) VALUES (\"%s\","
                     "\"%s\", \"%s\", \"\", DATETIME('now', 'localtime'),"
                     "\"%s\", \"%s\"",
                     realmInfo.sRealmName.c_str(),
                     gameInfo.sMapName.c_str(),
                     gameInfo.sGameName.c_str(),
                     gameInfo.sMyName.c_str(),
                     gameInfo.sHostName.c_str());
  
  wxString s;                
  for (int x = 1; x <= nClients; x++) {
    shared_ptr<Slot> pSlot = SlotManager::Get()->GetSlot(x);
    s = "";
    if (pSlot && pSlot->pClient) {
      s = pSlot->pClient->sName;
    }
    sInsert += wxString::Format(", \"%s\"", s.c_str());
  }
  
  sInsert += ")";
  
  m_pDb->Exec(sInsert.c_str());
  
  //Get the last game id
  m_pDb->Exec("SELECT id FROM gameinfo ORDER BY id DESC LIMIT 1");
  if (m_pDb->vdata.size() > 0) {
    long l;
    m_pDb->vdata[0].ToLong(&l);
    m_nLastGameId = static_cast<int>(l);
  }
  
  //Interface::Get()->Output(sInsert);
}

//===============================
/*static*/ bool ExtraInfoModule::UpdateCurrentGame()
{
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();
  wxString sUpdate = wxString::Format("UPDATE gameinfo SET duration=\"%s\" WHERE id=%d", gameInfo.gameDuration.Format("%H:%M:%S").c_str(), m_nLastGameId);
    
  m_pDb->Exec(sUpdate.c_str());
}

//===============================
void ExtraInfoModule::OnClientConnectAttempt(NetEvent &e)
{
  e.Skip();  
  
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();
  if (gameInfo.bHosted) {
    const char *pClientName = reinterpret_cast<const char *>(e.m_pData) + 19;
    OnClientConnectInfoCommon(pClientName);
  }
}

//===============================
void ExtraInfoModule::OnClientInfo1(NetEvent &e)
{
  e.Skip();  
  
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();
  if (!gameInfo.bHosted) {
    const char *pClientName = reinterpret_cast<const char *>(e.m_pData) + 9;
    OnClientConnectInfoCommon(pClientName);
  }
}

//===============================
//This is called if a client connects or we get their info. However, this is not
//called when a client connects if we aren't hosting. And, this is not called by 
//ClientInfo1 if we are hosting.
void ExtraInfoModule::OnClientConnectInfoCommon(const char *pClientName)
{  
  //wxString sRefresherName = CONFIG(wxString, "AutoRefresh/Name");
  IntStrMap refresherNameMap = CONFIG(IntStrMap, "AutoRefresh/NameMap");
  wxString sPlayerHasInfo = CONFIG(wxString, "Speech/PlayerHasInfo"); 
  
  //If we get this packet, then we need to add infos eventually. Starting the game
  //will set this to false.
  m_bAddedInfos = false;
  
  //Don't get info if this is the refresher
  for (IntStrMap::iterator i = refresherNameMap.begin(); i != refresherNameMap.end(); i++) {
    if (i->second.IsSameAs(pClientName, false))
      return;
  }
  
  shared_ptr<Client> pClient = ClientManager::Get()->GetClientByName(pClientName);  
  if (!pClient)
    return;
    
  if (!(pClient->flags["extrainfo"] & EICF_CHECKED)) {
    if (Check(pClient)) {
      Netclip::SendVoice(sPlayerHasInfo);
    }
    
    pClient->flags["extrainfo"] |= EICF_CHECKED;
  }
}

//===============================
void ExtraInfoModule::OnGameFinishedLoading(NetEvent &e)
{
  e.Skip();
  
  if (m_bAddedInfos)
    return;
  
  wxString sMyName = Info::Get()->GetGameInfo().sMyName;
    
  //Add game info
  AddCurrentGame();
  
  //Add client infos
  const SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (SlotTable::const_iterator i = table.begin(); i != table.end(); i++) {
    if ((*i)->pClient && !(*i)->pClient->sName.IsSameAs(sMyName)) {
      AddOrUpdateClient((*i)->pClient); 
    }
  }
  
  m_bAddedInfos = true;
}

//===============================
void ExtraInfoModule::OnGameEnded(NetEvent &e)
{
  //Take advantage of the fact that this is only true if the game started.
  if (!m_bAddedInfos) return;
  
  //Update the current game.
  UpdateCurrentGame();
  
  //We can safely set this to false now.
  m_bAddedInfos = false;
}

//===============================
void ExtraInfoModule::OnGameExited(NetEvent &e)
{
  if (!m_bAddedInfos) return;
    
  UpdateCurrentGame();
  m_bAddedInfos = false;
}

//-------------------------------
//Commands

//===============================
DEFINE_COMMAND(PlayerInfoCmd)
{
  if (asCmd.size() != 2) {
    Interface::Get()->Output("You must provide name to get the player info from.", OUTPUT_ERROR);
    return;
  }
  
  //Create a fake client to check
  shared_ptr<Client> pClient = shared_ptr<Client>(new Client());
  pClient->sName = asCmd[1];
  
  if (!ExtraInfoModule::Check(pClient))
    Interface::Get()->Output(wxString::Format("\"</B>%s<!B>\" has no player info.", pClient->sName.c_str()));
}

//===============================
DEFINE_COMMAND(CommentCmd)
{
  if (asCmd.size() < 3) {
    Interface::Get()->Output("You must provide a client name and a comment.", OUTPUT_ERROR);
    return;
  }
  
  //Create a fake client
  shared_ptr<Client> pClient = shared_ptr<Client>(new Client());
  pClient->sName = asCmd[1];
  
  ArrayString::const_iterator begin = asCmd.begin() + 2;
  ArrayString::const_iterator end = asCmd.end();
  wxString sComment = Join(begin, end);
  
  ExtraInfoModule::UpdateClient(pClient, sComment);
}

//===============================
DEFINE_COMMAND(RateCmd)
{
  if (asCmd.size() != 3) {
    Interface::Get()->Output("You must provide a client name and a score.", OUTPUT_ERROR);
    return;
  }
  
  //Create a fake client
  shared_ptr<Client> pClient = shared_ptr<Client>(new Client());
  pClient->sName = asCmd[1];
  
  double dScore;
  asCmd[2].ToDouble(&dScore);
  
  ExtraInfoModule::UpdateClient(pClient, dScore);
}
