#ifndef _INJECTMODULE_H
#define _INJECTMODULE_H

#include "Module.h"
#include "Command.h"

//-------------------------------
class InjectModule : public Module
{
  public:
    InjectModule();    
    ~InjectModule();
    
    void Initialize(shared_ptr<Game> pGame);
    void Shutdown();
    
  private:
    shared_ptr<Game> m_pGame;
};

//-------------------------------
//Commands
DECLARE_COMMAND(InjectCmd, "inject")

#endif
