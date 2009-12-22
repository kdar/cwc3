#ifndef _OBJECT_H
#define _OBJECT_H

#include "Lib.h"

//Note on shared_from_this, you can only obtain it if you have
//created a shared_ptr to this object first. So for example, if you have
//a class named Caca that inherits Object<Caca>, then somewhere you must do:
//shared_ptr<Caca> caca(new Caca()); before SharedPtr() will work.

//-------------------------------
template <class T>
class Object : public enable_shared_from_this<T>
{
  public:
    Object() {}
    virtual ~Object() {}
    
    virtual shared_ptr<T> SharedPtr()
    { return enable_shared_from_this<T>::shared_from_this(); }
};

#endif
