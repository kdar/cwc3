#include <wx/tokenzr.h>

#include "Config.h"
#include "Functions.h"

shared_ptr<Config> Config::ms_pInstance = shared_ptr<Config>();

//===============================
/*static*/ shared_ptr<Config> Config::Get()
{
  if (!ms_pInstance)
    ms_pInstance = shared_ptr<Config>(new Config());
  return ms_pInstance;
}

//===============================
/*static*/ shared_ptr<Config> Config::Set(shared_ptr<Config> pInstance)
{
  shared_ptr<Config> pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
bool Config::Load(const wxString &sFileName)
{
  m_sFileName = sFileName;
  m_pConfig = shared_ptr<wxFileConfig>(new wxFileConfig(wxEmptyString, wxEmptyString, sFileName, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH));
  m_pConfig->SetRecordDefaults(true);

  //Not only do we load settings, but we also load defaults here.

  GetString("Network/SniffDevice", "eth1");
  GetString("Network/InternetDevice", "ppp0");
  GetString("Network/GameHost", "192.168.0.2");
  GetInt("Network/GamePort", 6112);

  GetInt("Battlenet/BattlePort", 6112);
  GetArrayIntHexToDec("Battlenet/DataPrefixes", "FF,F7");
  GetString("Battlenet/DefaultNick", "binds");
  
  GetInt("Client/DisconnectMax", 60);
  GetInt("Client/DisconnectThreshold", 20);
  
  GetBool("AutoRefresh/Enabled", true);
  GetInt("AutoRefresh/Delay", 20);
  /*const char *asDefaultNameMap[] = {"|CFFFF0303\xEF\x80\x80|R", "|CFF00FF42\xEF\x80\x80|R", 
                                    "|CFF0042FF\xEF\x80\x80|R", "|CFFFF0303\xEF\x80\x80|R", 
                                    "|CFF00FF42\xEF\x80\x80|R", "|CFF0042FF\xEF\x80\x80|R", 
                                    "|CFFFF0303\xEF\x80\x80|R", "|CFF00FF42\xEF\x80\x80|R", 
                                    "|CFF0042FF\xEF\x80\x80|R", "|CFFFF0303\xEF\x80\x80|R", 
                                    "|CFF00FF42\xEF\x80\x80|R", "|CFF0042FF\xEF\x80\x80|R"};*/
  const char *asDefaultNameMap[] = {"|CFF0DB5E7\xEF\x80\x80|R", "|CFF0DB5E7\xEF\x80\x80|R", 
                                    "|CFF0DB5E7\xEF\x80\x80|R", "|CFF0DB5E7\xEF\x80\x80|R", 
                                    "|CFF0DB5E7\xEF\x80\x80|R", "|CFF0DB5E7\xEF\x80\x80|R", 
                                    "|CFF0DB5E7\xEF\x80\x80|R", "|CFF0DB5E7\xEF\x80\x80|R", 
                                    "|CFF0DB5E7\xEF\x80\x80|R", "|CFF0DB5E7\xEF\x80\x80|R", 
                                    "|CFF0DB5E7\xEF\x80\x80|R", "|CFF0DB5E7\xEF\x80\x80|R"};
  GetMapByIndexing("AutoRefresh/NameMap", 0, 12, asDefaultNameMap);
  //GetString("AutoRefresh/Name", "|CFF0042FF\xEF\x80\x80|R");
  /*char *aszNames[] = {"|CFFFF0303\xEF\x80\x80|R",
                      "|CFF0042FF\xEF\x80\x80|R",
                      "|CFF1Ce6B9\xEF\x80\x80|R",
                      "|CFF540081\xEF\x80\x80|R",
                      "|CFFFFFC01\xEF\x80\x80|R",
                      "|CFFFEBA0E\xEF\x80\x80|R",
                      "|CFF20C000\xEF\x80\x80|R",
                      "|CFFE55BB0\xEF\x80\x80|R",
                      "|CFF959697\xEF\x80\x80|R",
                      "|CFF7EBFF1\xEF\x80\x80|R",
                      "|CFF00FF00\xEF\x80\x80|R",
                      "|CFF4E2A04\xEF\x80\x80|R"};*/
  
  GetString("Netclip/Host", "192.168.0.2");
  GetInt("Netclip/Port", 45643);
  
  GetString("Speech/PlayerLeft", "Leaver detected");
  GetString("Speech/GameStarted", /*"Game started"*/"");
  GetString("Speech/GameLoaded", /*"Game loaded"*/"");
  
  GetArrayString("Realm/Hosts", "Azeroth:useast.battle.net,Lordaeron:uswest.battle.net,Northrend:europe.battle.net,Kalimdor:asia.battle.net");
  
  GetString("Ip2Country/DbFile", "local.db");

  const char *asDefaultColorMap[] = {"red", "blue", "teal", "purple", "yellow",
                                     "orange", "green", "pink", "grey", "light blue",
                                     "dark green", "brown"};
  GetMapByIndexing("Client/ColorMap", 0, 12, asDefaultColorMap);

  const char *asDefaultTeamMap[] = {"Sentinel", "Scourge"};
  GetMapByIndexing("Client/TeamMap", 0, 2, asDefaultTeamMap);

  const char *asDefaultDifficultyMap[] = {"Easy", "Normal", "Insane"};
  GetMapByIndexing("Client/DifficultyMap", 0, 3, asDefaultDifficultyMap);

  GetString("Interface/Client/LoadedFormat", "</D/24>");  

  const char *asDefaultSlotTextMap[] = {"00", "01", "02", "03", "04", "05",
                                        "06", "07", "08", "09", "10", "11"};
  GetMapByIndexing("Interface/Client/Slot/TextMap", 0, 12, asDefaultSlotTextMap);

  const char *asDefaultSlotFormatMap[] = {"</L></B/16>", "</L></N/40>", "</L></N/56>", "</L></N/48>", "</L></B/32>",
                                          "</L></B/16>", "</L></B/24>", "</L></B/48>", "</L></D/08>", "</L></B/40>",
                                          "</L></D/24>", "</L></N/32>"};
  GetMapByIndexing("Interface/Client/Slot/FormatMap", 0, 12, asDefaultSlotFormatMap);
}

//===============================
bool Config::Reload()
{
  m_pConfig.reset();

  Load(m_sFileName);
}

//===============================
bool Config::GetBool(const wxString &sPath, bool bDefault)
{
  if (!m_pConfig)
    return bDefault;

  long lDefault = bDefault == true ? 1 : 0;
  long lValue = m_pConfig->Read(sPath, lDefault);
  
  bool bRet = lValue == 1 ? true : false;
  m_map[sPath] = bRet;
  return bRet;
}

//===============================
int Config::GetInt(const wxString &sPath, int nDefault)
{
  if (!m_pConfig)
    return nDefault;

  long lDefault = static_cast<long>(nDefault);
  long lValue = m_pConfig->Read(sPath, lDefault);

  int nRet = static_cast<int>(lValue);
  m_map[sPath] = nRet;
  return nRet;
}

//===============================
wxString Config::GetString(const wxString &sPath, const wxString &sDefault)
{
  if (!m_pConfig)
    return sDefault;

  wxString sRet = m_pConfig->Read(sPath, sDefault);
  m_map[sPath] = sRet;
  return sRet;
}

//===============================
ArrayInt Config::GetArrayInt(const wxString &sPath, const wxString &sDefault)
{
  ArrayInt array;
  wxString sValue = m_pConfig->Read(sPath, sDefault);
  wxStringTokenizer tokenizer(sValue, ',', wxTOKEN_STRTOK);
  while (tokenizer.HasMoreTokens()) {
    long lValue = 0;
    tokenizer.GetNextToken().ToLong(&lValue);

    array.push_back(static_cast<int>(lValue));
  }

  m_map[sPath] = array;
  return array;
}

//===============================
ArrayInt Config::GetArrayIntHexToDec(const wxString &sPath, const wxString &sDefault)
{
  ArrayInt array;
  wxString sValue = m_pConfig->Read(sPath, sDefault);
  wxStringTokenizer tokenizer(sValue, ',', wxTOKEN_STRTOK);
  while (tokenizer.HasMoreTokens()) {
    array.push_back(ConvertHexToDec(tokenizer.GetNextToken()));
  }

  m_map[sPath] = array;
  return array;
}

//===============================
ArrayString Config::GetArrayString(const wxString &sPath, const wxString &sDefault)
{
  ArrayString array;
  wxString sValue = m_pConfig->Read(sPath, sDefault);
  wxStringTokenizer tokenizer(sValue, ',', wxTOKEN_STRTOK);
  while (tokenizer.HasMoreTokens()) {
    array.push_back(tokenizer.GetNextToken());
  }

  m_map[sPath] = array;
  return array;
}

//===============================
IntStrMap Config::GetMapByIndexing(const wxString &sPath, int nStartNumber, int nSize, const char *asDefault[])
{
  IntStrMap map;
  int nNumber = nStartNumber;
  for (int x = 0; x < nSize; x++, nNumber++) {
    wxString s = m_pConfig->Read(sPath + wxString::Format("/%d", nNumber), asDefault == NULL ? "" : asDefault[x]);
    map[nNumber] = s;
  }

  m_map[sPath] = map;
  return map;
}

//===============================
StrStrMap Config::GetMapByStrings(const wxString &sPath, int nSize, const char *asDefaultKeys[], const char *asDefaultValues[])
{
  StrStrMap map;
  wxString sKey;
  long dummy;
  
  //Set all the defaults in the map
  for (int x = 0; x < nSize; x++) {
    map[asDefaultKeys[x]] = asDefaultValues[x]; 
  }

  //Now enumerate through the config, overwriting defaults.
  bool bCont = m_pConfig->GetFirstEntry(sKey, dummy);
  while (bCont) {
    wxString s = m_pConfig->Read(sPath + "/" + sKey, "");
    if (!s.IsEmpty())
      map[sKey] = s;

    bCont = m_pConfig->GetNextEntry(sKey, dummy);
  }
  
  m_map[sPath] = map;  
  return map;
}
