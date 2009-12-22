#include <wx/tokenzr.h>

#include "CoreCommands.h"
#include "CommandHandler.h"

#include "ClientManager.h"
#include "Info.h"
#include "SlotManager.h"
#include "Interface.h"
#include "AutoRefresh.h"
#include "Main.h"
#include "Config.h"
#include "Functions.h"
#include "Netclip.h"

//===============================
void RegisterCoreCommands(shared_ptr<CommandHandler> pHandler)
{ 
  BEGIN_REGISTER_COMMANDS(pHandler);
  
  REGISTER_COMMAND(HelpCmd);
  REGISTER_COMMAND(ExitCmd);
  REGISTER_COMMAND(RefreshUICmd);
  REGISTER_COMMAND(RefreshCmd);
  REGISTER_COMMAND(ListSnapshotCmd);
  REGISTER_COMMAND(InfoCmd);
  REGISTER_COMMAND(ListClientsCmd);
  REGISTER_COMMAND(LoadTimeCmd);
  REGISTER_COMMAND(LoadMinMaxCmd);
  REGISTER_COMMAND(ListLoadTimeCmd);
  REGISTER_COMMAND(ConfigCmd);
  REGISTER_COMMAND(ListInfoCmd);
  REGISTER_COMMAND(SpeakCmd);
  
  END_REGISTER_COMMANDS();
}

//===============================
DEFINE_COMMAND(HelpCmd)
{
  const CommandList &cmdList = pHandler->GetCommandList();
  for (CommandList::const_iterator i = cmdList.begin(); i != cmdList.end(); i++) {
    Interface::Get()->Output(i->first);
  }
}

//===============================
DEFINE_COMMAND(ExitCmd)
{
  if (pHandler->GetType() == CHT_CONSOLE)
    Main::Get()->Shutdown();
}

//===============================
DEFINE_COMMAND(RefreshUICmd)
{
  Interface::Get()->RefreshClientAll();
  Interface::Get()->RefreshMapInfo();
}

//===============================
DEFINE_COMMAND(RefreshCmd)
{  
}

//===============================
DEFINE_COMMAND(ListSnapshotCmd)
{
  const GameInfo &gameInfo = Info::Get()->GetGameInfo();
  Interface::Get()->Output(wxString::Format("Snapshot:"));
  Interface::Get()->Output(wxString::Format("bCreatingGame: %s", gameInfo.bCreatingGame ? "true" : "false"));
  Interface::Get()->Output(wxString::Format("bCreatedGame:  %s", gameInfo.bCreatedGame  ? "true" : "false"));
  Interface::Get()->Output(wxString::Format("bStartingGame: %s", gameInfo.bStartingGame ? "true" : "false"));
  Interface::Get()->Output(wxString::Format("bStartedGame:  %s", gameInfo.bStartedGame  ? "true" : "false"));
  Interface::Get()->Output(wxString::Format("bExitedGame:   %s", gameInfo.bExitedGame   ? "true" : "false"));
  Interface::Get()->Output(wxString::Format("bEndedGame:    %s", gameInfo.bEndedGame    ? "true" : "false"));
}

//===============================
DEFINE_COMMAND(InfoCmd)
{
  wxString sName;
  if (asCmd.size() > 1)
    sName = asCmd[1];
    
  bool bFakeInfo = asCmd[0].IsSameAs("finfo", false);

  wxString sMessage = "s {appactivate Warcraft III}+{DELAY 100}{ENTER}";

  bool bSend = false;

  if (sName.IsEmpty()) {
    int nPrinted = 0;
    const SlotTable &table = SlotManager::Get()->GetSlotTable();
    for (int x = 0; x < table.size(); x++) {
      shared_ptr<Client> pClient = table[x]->pClient;
      if (pClient && !pClient->sCountry.IsEmpty() && !pClient->ping.IsNull()) {
        bSend = true;
        nPrinted++;
        sMessage += pClient->sName + ": {LEFTPAREN}";
        sMessage += pClient->sCountry + "{RIGHTPAREN}[";
        int nPing = pClient->GetPing();
        //Make this percentage configurable.
        if (bFakeInfo)
        	nPing = static_cast<int>(nPing * .70);
        sMessage += wxString::Format("%d", nPing);

        if (x + 1 < table.size() && table[x+1]->pClient)
          sMessage += "], ";
        else
          sMessage += "]";
      }

      //If we loaded the game, then blizzard limits the amount of text
      //we can write on a single line. So we break it up every five entries.
      if (Info::Get()->GetGameInfo().bLoadedGame) {
        if (nPrinted == 4) {
          if (table.size() - x > 0)
            sMessage += "{BS}{BS}";
          sMessage += "{ENTER}+{DELAY 100}{ENTER}";
        }
      }
    }
  } else {
    shared_ptr<Client> pClient = ClientManager::Get()->GetClientByNameMatch(sName);
    if (pClient) {
      bSend = true;

      sMessage += pClient->sName + ": {LEFTPAREN}";
      sMessage += pClient->sCountry + "{RIGHTPAREN}[";
      int nPing = pClient->GetPing();
      //Make this percentage configurable.
      if (bFakeInfo)
      	nPing = static_cast<int>(nPing * .70);
      sMessage += wxString::Format("%d", nPing);
      sMessage += "]";
    }
  }

  sMessage += "{ENTER}\n";

  if (bSend) {
    Netclip nc;
    nc.Send(sMessage);
  }
}

//===============================
DEFINE_COMMAND(ListClientsCmd)
{
  int nCount = 0;
  const ClientArray &array = ClientManager::Get()->GetClientArray();
  for (int x = 0; x < array.size(); x++) {
    Interface::Get()->Output("------------------------");

    wxStringTokenizer tokenizer(array[x]->ToString(), "\n", wxTOKEN_STRTOK);
    while (tokenizer.HasMoreTokens()) {
      wxString sToken = tokenizer.GetNextToken();
      Interface::Get()->Output(sToken);
    }

    nCount++;
  }

  Interface::Get()->Output("------------------------");
  Interface::Get()->Output(wxString::Format("Clients Printed: %d", nCount));
  //Printing out all this information can sometimes screw up the screen. So refresh it.
  Interface::Get()->RefreshScreen();
}

//===============================
DEFINE_COMMAND(LoadTimeCmd)
{
  wxString sName;
  if (asCmd.size() > 1)
    sName = asCmd[1];

  wxString sMessage = "s {appactivate Warcraft III}+{DELAY 100}{ENTER}Load times:{ENTER}+{DELAY 100}{ENTER}";

  bool bSend = false;

  if (sName.IsEmpty()) {
    int nPrinted = 0;
    const SlotTable &table = SlotManager::Get()->GetSlotTable();
    for (int x = 0; x < table.size(); x++) {
      shared_ptr<Client> pClient = table[x]->pClient;
      if (pClient && !pClient->loadTime.IsNull()) {
        bSend = true;
        nPrinted++;
        sMessage += pClient->sName + ": {LEFTPAREN}";
        sMessage += pClient->loadTime.Format("%Mm%Ss");            

        if (x + 1 < table.size() && table[x+1]->pClient)
          sMessage += "{RIGHTPAREN}, ";
        else
          sMessage += "{RIGHTPAREN}";
      }

      //If we loaded the game, then blizzard limits the amount of text
      //we can write on a single line. So we break it up every 3 entries
      if (Info::Get()->GetGameInfo().bLoadedGame) {
        if (nPrinted % 3 == 0) {
          if (table.size() - x > 0)
            sMessage += "{BS}{BS}";
          sMessage += "{ENTER}+{DELAY 100}{ENTER}";
        }
      }
    }
  } else {
    shared_ptr<Client> pClient = ClientManager::Get()->GetClientByNameMatch(sName);
    if (pClient) {
      bSend = true;

      sMessage += pClient->sName + ": {LEFTPAREN}";
      sMessage += pClient->loadTime.Format("%Mm%Ss") + "{RIGHTPAREN}";
    }
  }

  sMessage += "{ENTER}\n";

  if (bSend) {
    Netclip nc;
    nc.Send(sMessage);
  }
}

//===============================
DEFINE_COMMAND(LoadMinMaxCmd)
{
  wxTimeSpan max;
  wxTimeSpan min;
  
  shared_ptr<Client> pClientMax;
  shared_ptr<Client> pClientMin;

  const SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (SlotTable::const_iterator i = table.begin(); i != table.end(); i++) {
    shared_ptr<Client> pClient = (*i)->pClient;
    if (!pClient) continue;
    
    if (min.IsNull() || pClient->loadTime.IsShorterThan(min)) {
      min = pClient->loadTime;
      pClientMin = pClient;
    }
    
    if (max.IsNull() || pClient->loadTime.IsLongerThan(max)) {
      max = pClient->loadTime;
      pClientMax = pClient;
    }
  }
  
  wxString s;
  bool bSend = false;
  if (pClientMin) {
    s += wxString::Format("Fastest loader: %s {LEFTPAREN}%s{RIGHTPAREN}", pClientMin->sName.c_str(), min.Format("%Mm %Ss").c_str());
    bSend = true;
  }
  
  if (pClientMax) {
    s += wxString::Format("%sSlowest loader: %s {LEFTPAREN}%s{RIGHTPAREN}", bSend ? ", " : "", pClientMax->sName.c_str(), max.Format("%Mm %Ss").c_str());
    bSend = true;
  }

  if (bSend) {
    wxString sMessage = "s {appactivate Warcraft III}+{DELAY 100}{ENTER}";
    sMessage += s;
    sMessage += "{ENTER}\n";
    Netclip nc;
    nc.Send(sMessage);
  }
}

//===============================
DEFINE_COMMAND(ListLoadTimeCmd)
{
  Interface::Get()->Output("------------------------");
      
  const SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (int x = 0; x < table.size(); x++) {
    shared_ptr<Client> pClient = table[x]->pClient;
    if (pClient) {
      Interface::Get()->Output(wxString::Format("%s: %s", pClient->sName.c_str(), pClient->loadTime.Format("%Mm %Ss").c_str())); 
    }
  }
  
  Interface::Get()->Output("------------------------");
}

//===============================
//FIXME: VERY INCOMPLETE
DEFINE_COMMAND(ConfigCmd)
{
  //If no arguments are supplied, print out the help.
  if (asCmd.size() == 1) {
    Interface::Get()->Output("config help:");
    Interface::Get()->Output(" set <key> <value> - set a config value");
    Interface::Get()->Output(" get <key>         - get a config value");
    Interface::Get()->Output(" list              - list all config values");
  } else {
    if (asCmd[1].IsSameAs("list", false)) {
      StrAnyMap &map = Config::Get()->GetMap();
      for (StrAnyMap::iterator i = map.begin(); i != map.end(); i++) {    
        wxString sOutput = wxString::Format("</N/48>%-50s<!N!48>", i->first.c_str());
       
        try {
          bool bValue = any_cast<bool>(i->second);
          sOutput += wxString::Format("%d", bValue);
        } catch (const bad_any_cast &) {}
        
        try {
          int nValue = any_cast<int>(i->second);
          sOutput += wxString::Format("%d", nValue);
        } catch (const bad_any_cast &) {}
        
        try {
          wxString sValue = any_cast<wxString>(i->second);
          sValue = Interface::Get()->EscapeFormatting(sValue);
          sOutput += wxString::Format("\"%s\"", sValue.c_str());
        } catch (const bad_any_cast &) {}
          
        try {
          ArrayInt value = any_cast<ArrayInt>(i->second);
          for (ArrayInt::iterator i  = value.begin(); i != value.end(); i++) {
            sOutput += wxString::Format("%d", *i);
            if (i+1 != value.end())
              sOutput += ",";
          }
        } catch (const bad_any_cast &) {}
          
        try {
          ArrayString value = any_cast<ArrayString>(i->second);
          for (ArrayString::iterator i  = value.begin(); i != value.end(); i++) {
            sOutput += wxString::Format("%s", i->c_str());
            if (i+1 != value.end())
              sOutput += ",";
          }
        } catch (const bad_any_cast &) {}
                
        Interface::Get()->Output(sOutput);
      }
    } else if (asCmd[1].IsSameAs("get", false)) {
      
    } else if (asCmd[1].IsSameAs("set", false)) {
      if (asCmd.size() < 4) {
        Interface::Get()->Output("Error, invalid amount of arguments for config set.");
        return;
      }
      
      StrAnyMap &map = Config::Get()->GetMap();
      wxString sKey = asCmd[2];
      wxString sValue = asCmd[3];
      if (sValue.IsNumber()) {
        long lNumber;
        sValue.ToLong(&lNumber);
        map[sKey] = static_cast<int>(lNumber);
      } else {
        map[sKey] = sValue; 
      }      
    }
  }
}

//===============================
DEFINE_COMMAND(ListInfoCmd)
{
  Interface::Get()->Output("------------------------");
      
  const SlotTable &table = SlotManager::Get()->GetSlotTable();
  for (int x = 0; x < table.size(); x++) {
    shared_ptr<Client> pClient = table[x]->pClient;
    if (pClient) {
      Interface::Get()->Output(wxString::Format("</B>%s<!B>: (</B/24>%s<!B!24>)[</B/32>%d<!B!32>]", pClient->sName.c_str(), pClient->sCountry.c_str(), pClient->GetPing())); 
    }
  }
  
  Interface::Get()->Output("------------------------");
}

//===============================
DEFINE_COMMAND(SpeakCmd)
{
  if (asCmd.size() < 2) {
    Interface::Get()->Output("Must provide text to say", OUTPUT_ERROR);
    return; 
  }
  
  ArrayString::const_iterator begin = asCmd.begin() + 1;
  ArrayString::const_iterator end = asCmd.end();
  wxString sText = Join(begin, end);

  Netclip::SendVoice(sText);
}
