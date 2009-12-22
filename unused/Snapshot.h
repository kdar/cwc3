#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H

#include "Object.h"

//-------------------------------
//A class that defines the snapshot of our current game status.
class Snapshot : public Object<Snapshot>
{
  public:
    Snapshot()
    { Reset(); }

    void Reset()
    {
      bCreatingGame = false;
      bCreatedGame  = false;
      bStartingGame = false;
      bStartedGame  = false;
      bLoadedGame   = false;
      bExitedGame   = false;
      bEndedGame    = false;
    }

  public:
    bool bCreatingGame;
    bool bCreatedGame;
    bool bStartingGame;
    bool bStartedGame;
    bool bLoadedGame;
    bool bExitedGame;
    bool bEndedGame;
};

#endif
