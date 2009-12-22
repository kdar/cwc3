#include <stdio.h>

#ifdef PLATFORM_UNIX
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <signal.h>
  #include <unistd.h>
#endif

#include "RunMax.h"

//===============================
RunMax::RunMax ()
  : wxThread (wxTHREAD_DETACHED)
{
}

//===============================
RunMax::~RunMax ()
{
}

//===============================
bool RunMax::Execute (const int nSeconds, const wxString &sCommand)
{
  m_nSeconds = nSeconds;
  m_sCommand = sCommand;

  if (Create () != wxTHREAD_NO_ERROR)
    return false;

  Run ();
  return true;
}

//===============================
#ifdef PLATFORM_UNIX
void *RunMax::Entry ()
{
  Exit ();
}
#endif

//===============================
#ifdef PLATFORM_WIN32
void *RunMax::Entry ()
{
  Exit ();
}
#endif
