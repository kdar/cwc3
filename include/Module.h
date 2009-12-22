#ifndef _MODULE_H
#define _MODULE_H

#include "Object.h"

//Forward declarations.
class Game;

//-------------------------------
class Module : public Object<Module>
{
  protected:
    Module() {}
    virtual ~Module() {}
  
  public:
    virtual void Initialize(shared_ptr<Game> pGame) = 0;
    virtual void Shutdown() = 0;
};

#endif
