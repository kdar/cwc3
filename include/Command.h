#ifndef _COMMAND_H
#define _COMMAND_H

#include <wx/string.h>
#include <wx/arrstr.h>

#include "Object.h"

//Forward declarations.
class CommandHandler;

//-------------------------------
class Command : public Object<Command>
{
  protected:
    Command() {}
    
    Command(wxString sCommand)
    { m_asStrings.Add(sCommand); }
    
    Command(int nCount, ...);
    
    virtual ~Command() {}

    virtual wxString GetShortHelp() const
    { return ""; }

    //A default help in case the command provides none.
    virtual wxString GetHelp() const
    { return ""; }
    
  public:    
    wxArrayString GetStrings() const
    { return m_asStrings; }
    
    virtual void Execute(shared_ptr<CommandHandler> pHandler, ArrayString asCmd, void *pExtra = 0) = 0;
    
  private:
    wxArrayString m_asStrings;
};

#define DECLARE_COMMAND(classname, cmdstr)                                               \
  class classname : public Command {                                                     \
    public:                                                                              \
      classname()                                                                        \
        : Command(cmdstr) {}                                                             \
      void Execute(shared_ptr<CommandHandler> pHandler, ArrayString asParams, void *pExtra = 0);  \
  };    

#define DECLARE_MULTI_COMMAND(classname, count, ...)                                     \
  class classname : public Command {                                                     \
    public:                                                                              \
      classname()                                                                        \
        : Command(count, __VA_ARGS__) {}                                                 \
      void Execute(shared_ptr<CommandHandler> pHandler, ArrayString asParams, void *pExtra = 0);  \
  };                                                           

#define DEFINE_COMMAND(classname) \
  void classname::Execute(shared_ptr<CommandHandler> pHandler, ArrayString asCmd, void *pExtra)

//#define BEGIN_REGISTER_COMMANDS(type, handler) \
//  type __pHandler = handler;
#define BEGIN_REGISTER_COMMANDS(handler) \
  BOOST_TYPEOF(handler) __pHandler = handler;

#define REGISTER_COMMAND(cmd) \
  (__pHandler->Register(shared_ptr<Command>(new cmd())))
  
#define END_REGISTER_COMMANDS() ;
    
#define DEFINE_HELP(classname) \
  wxString classname::GetHelp() const


#endif
