#include <wx/tokenzr.h>

#include "CommandHandler.h"
#include "Command.h"

//FIXME: There is nothing to delete all the elements of this list
//Either we create a mutex for it and delete it in a destructor, or
//we use shared_ptr from the boost library to handle it automatically.
CommandList CommandHandler::ms_commandList;

//===============================
void CommandHandler::Register(shared_ptr<Command> pCmd)
{
  wxArrayString strings = pCmd->GetStrings();
  for (int x = 0; x < strings.GetCount(); x++) {
    ms_commandList[strings.Item(x).Lower().c_str()] = pCmd;
  }
}

//===============================
void CommandHandler::Unregister(const wxString &sCmd)
{
  ms_commandList.erase(sCmd.Lower().c_str());
}

//===============================
void CommandHandler::Unregister(const shared_ptr<Command> pCmd)
{
  wxArrayString strings = pCmd->GetStrings();
  for (int x = 0; x < strings.GetCount(); x++) {
    Unregister(strings.Item(x));
  }
}

//===============================
void CommandHandler::Process(const wxString &sFullCmd, void *pExtra)
{  
  ArrayString asCmd;
  wxStringTokenizer tokenizer(sFullCmd, " \t\r\n", wxTOKEN_STRTOK);
  while (tokenizer.HasMoreTokens()) {
    wxString sToken = tokenizer.GetNextToken();
    asCmd.push_back(sToken);
  }

  if (!asCmd.empty()) {
    //Remove 1 or 2 forward slashes from the beginning.
    if (asCmd[0].Index('/') == 0) {
      asCmd[0].Remove(0, 1);
    }
    if (asCmd[0].Index('/') == 0) {
      asCmd[0].Remove(0, 1);
    }
    
    //Get the command and remove it from the list.
    wxString sCmd = asCmd[0];
    
    //Make it lower case (case insensitive)
    sCmd.MakeLower();
    
    //Find the command and execute it.
    CommandList::iterator i = ms_commandList.find(sCmd.c_str());
    if (i != ms_commandList.end())
      i->second->Execute(SharedPtr(), asCmd, pExtra);
  }
}
