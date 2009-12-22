#ifndef _MAIN_H
#define _MAIN_H

#include "Object.h"

//-------------------------------
class Main : public Object<Main>
{
  public:
    Main(int nArgc, char **pszArgv)
        : m_bShutdown(false), m_nArgc(nArgc), m_pszArgv(pszArgv)
    { Main::Set(this); }

    static Main *Get();
    static Main *Set(Main *pInstance);

    int Run();
    void Shutdown();

    static void SignalHandler(int nSignal);

  protected:
    void InitSignals();

  private:
    int m_nArgc;
    char **m_pszArgv;

    static Main *ms_pInstance;

    volatile bool m_bShutdown;
};

#endif
