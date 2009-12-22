#ifndef _COMMANDHANDLER_H
#define _COMMANDHANDLER_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/arrstr.h>

#include <map>
#include <string>

#include "Object.h"

//Forward declarations
class Command;

typedef shared_ptr<Command> CommandPtr;
typedef map<string, CommandPtr> CommandList;

//-------------------------------
enum CommandHandlerType
{
  CHT_CONSOLE = 1,
  CHT_NET
};

//-------------------------------
class CommandHandler : public Object<CommandHandler>
{
  public:
    CommandHandler(CommandHandlerType type)
      : m_type(type)
    {}
    
    void Register(shared_ptr<Command> pCmd);
    void Unregister(const wxString &sCmd);
    void Unregister(const shared_ptr<Command> pCmd);

    void Process(const wxString &sFullCmd, void *pExtra = 0);
    
    CommandHandlerType GetType() const
    { return m_type; }
    
    CommandList &GetCommandList()
    { return ms_commandList; }
    
    const CommandList &GetCommandList() const
    { return ms_commandList; }

  private:
    CommandHandlerType m_type;
    
    //Shared among all instances of CommandHandler
    static CommandList ms_commandList;
};

#endif
