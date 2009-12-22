#ifdef PLATFORM_UNIX
#include <signal.h>
#endif

#include <wx/defs.h>
#include <wx/app.h>
#include <wx/utils.h>

#include "Main.h"
#include "Game.h"
#include "ClientManager.h"
#include "Interface.h"
#include "SlotManager.h"
#include "Config.h"
#include "Lib.h"

Main *Main::ms_pInstance = NULL;

//===============================
int main(int argc, char *argv[])
{
  Main main(argc, argv);

  return main.Run();
}

//===============================
/*static*/ Main *Main::Get()
{
  if (!ms_pInstance) {
    fprintf(stderr, "You forgot to use Main::Set to set the Main object.\n");
  }

  return ms_pInstance;
}

//===============================
/*static*/ Main *Main::Set(Main *pInstance)
{
  Main *pOldInstance = ms_pInstance;
  ms_pInstance = pInstance;
  return pOldInstance;
}

//===============================
int Main::Run()
{
  wxInitializer initializer;
  if (!initializer) {
    fprintf(stderr, "Failed to initialize the wxWidgets library, aborting.");
    return -1;
  }
  
  InitSignals();

  bool bLoadSuccess = true;

  shared_ptr<Config> pConfig = Config::Get();
  shared_ptr<Interface> pInterface = Interface::Get();

  if (!pInterface->Initialize())
    bLoadSuccess = false;
  pInterface->Run();

  shared_ptr<Game> pGame = Game::Get();
  if (!pGame->Initialize())
    bLoadSuccess = false;
  pGame->Run();

  shared_ptr<ClientManager> pClientManager = ClientManager::Get();
  shared_ptr<SlotManager> pSlotManager = SlotManager::Get();

  if (bLoadSuccess) {
    pInterface->Output(_T("CWc3 successfully loaded."));
  } else {
    pInterface->Output(_T("CWc3 failed to load correctly."), OUTPUT_ERROR);
  }

  //Wait until m_bShutdown is true while doing some OnInterval calls.
  while (!m_bShutdown) {
    pInterface->OnInterval();
    pGame->OnInterval();
    ::wxMilliSleep(1000);
  }
  
  pGame->Shutdown();
  pGame->Delete();
    
  //This sadly has to be done because the CDK library has blocking calls.
  //I tried my hardest to somehow to get around it, but I'm not sure how
  //to interrupt those blocking calls. So we must force a kill to this thread.
#ifdef USE_INTERFACE_CDK
  pInterface->OnExit();
  pInterface->Kill();  
#endif

  return 0;
}

//===============================
void Main::Shutdown()
{
  //The reason we set a variable instead of doing the shutdown process here is
  //that another thread may call this function. If that occurs, then calls to destroy
  //other threads would be made in that thread's context. This is a bad idea. We should
  //create and destroy threads using the main thread only.
  if (!m_bShutdown) {
    m_bShutdown = true;
    Interface::Get()->Output(_T("Shutting down..."));
  }
}

//===============================
void Main::InitSignals()
{
#ifdef PLATFORM_UNIX
  int signals[] = {SIGHUP, SIGINT, SIGQUIT, SIGUSR1, SIGTERM};
  int signals_n(sizeof(signals) / sizeof(signals[0]));

  for (int i = 0; i < signals_n; i++) {
    signal(signals[i], SignalHandler);
  }
#endif
}

//===============================
/*static*/ void Main::SignalHandler(int nSignal)
{
  Main::Get()->Shutdown();
}
