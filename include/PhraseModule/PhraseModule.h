#ifndef _PHRASEMODULE_H
#define _PHRASEMODULE_H

#include "Module.h"

//Forward declarations
class Client;

//-------------------------------
class PhraseModule : public Module
{
  public:
    PhraseModule()
    : Module()
    {}
    
    void Initialize(shared_ptr<Game> pGame);
    void Shutdown();
    
    static wxString Parse(const wxString &sPhrase, const wxString &sParam, shared_ptr<Client> pClient = shared_ptr<Client>());
    
  private:
    shared_ptr<Game> m_pGame;
};

#endif
