#include "PhraseModule/PhraseModule.h"
#include "CommandHandler.h"
#include "SlotManager.h"
#include "Client.h"
#include "Game.h"
#include "Config.h"
#include "Info.h"

//===============================
void PhraseModule::Initialize(shared_ptr<Game> pGame)
{
  m_pGame = pGame;
}

//===============================
void PhraseModule::Shutdown()
{
}

//===============================
/*static*/ wxString PhraseModule::Parse(const wxString &sPhrase, const wxString &sParam, shared_ptr<Client> pClient)
{  
  wxDateTime date = wxDateTime::Now();
    
  wxString sDefaultNick = CONFIG(wxString, "Battlenet/DefaultNick");
    
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();

  string s = sPhrase.c_str();
  string::const_iterator begin = s.begin();
  string::const_iterator end = s.end();
  smatch what;
  
  regex dateReg("\\$date\\((.*?)\\)");  
  int offset = 0;
  while (regex_search(begin + offset, end, what, dateReg)) {
    string n = date.Format(what.str(1).c_str()).c_str();
    s = s.replace(offset + what.position(0), what.length(0), n);
    offset += what.position(0) + n.size();
  }
  
  wxString sMyName = gameInfo.sMyName;
  if (sMyName.IsEmpty())
    sMyName = sDefaultNick;
  s = regex_replace(s, regex("\\$myname"), sMyName.c_str());
 
  wxString sHostName = gameInfo.sHostName;
  if (sHostName.IsEmpty())
    sHostName = "n/a";
  s = regex_replace(s, regex("\\$hostname"), sHostName.c_str());
  
  wxString sMapName = gameInfo.sMapName;
  if (sMapName.IsEmpty())
    sMapName = "n/a";
  s = regex_replace(s, regex("\\$mapname"), sMapName.c_str());
  
  wxString sShortMapName = gameInfo.sShortMapName;
  if (sShortMapName.IsEmpty())
    sShortMapName = "n/a";
  s = regex_replace(s, regex("\\$smapname"), sShortMapName.c_str());
  
  wxString sGameName = gameInfo.sGameName;
  if (sGameName.IsEmpty())
    sGameName = "n/a";
  s = regex_replace(s, regex("\\$gamename"), sGameName.c_str());
  
  wxString sState = SlotManager::Get()->GetVersusString();
  if (sState.IsEmpty())
    sState = "n/a";
  s = regex_replace(s, regex("\\$state"), sState.c_str());
  
  wxString sRealm = Info::Get()->GetRealmInfo().sRealmName;
  if (sRealm.IsEmpty())
    sRealm = "n/a";
  s = regex_replace(s, regex("\\$realm"), sRealm.c_str());
  
  wxTimeSpan gameTime;
  if (gameInfo.startTime.IsValid())
    gameTime = date.Subtract(gameInfo.startTime);
  s = regex_replace(s, regex("\\$gametime"), gameTime.Format("%H:%M:%S").c_str());
  
  wxString sRatio;
  if (pClient) sRatio = pClient->sRatio;
  if (sRatio.IsEmpty())
    sRatio = "n/a";
  s = regex_replace(s, regex("\\$ratio"), sRatio.c_str());
  
  //This one works a bit differently because if sParam is empty, we want to remove one
  //space as to close any gaps.
  if (sParam.IsEmpty()) {
    s = regex_replace(s, regex("\\s\\$paramnospace|\\$paramnospace\\s"), "");
    s = regex_replace(s, regex("\\$paramnospace"), "");
  } else {
    s = regex_replace(s, regex("\\$paramnospace"), sParam.c_str());
  }
  
  //This one works like the above, except it adds a separator if the param isn't empty
  if (sParam.IsEmpty()) {
    s = regex_replace(s, regex("\\s\\$paramseparator|\\$paramseparator\\s"), "");
    s = regex_replace(s, regex("\\$paramseparator"), "");
  } else {
    string new_s = "| ";
    new_s.append(sParam.c_str());
    s = regex_replace(s, regex("\\$paramseparator"), new_s);
  }
  
  wxString sTmpParam = sParam;
  if (sParam.IsEmpty())
    sTmpParam = "n/a";
  s = regex_replace(s, regex("\\$param"), sTmpParam.c_str());
  
  return s.c_str();
}
