#ifndef _GAMEINFO_H
#define _GAMEINFO_H

#include "Object.h"

//-------------------------------
//Contains information pertaining to the game being played
class GameInfo : public Object<GameInfo>
{
  public:
    GameInfo()
    { Reset(); }

    void Reset()
    {
      sMapName = "";
      sGameName = "";
      bHosted = false;
      bPrivate = false;
      nHostedCount = 0;
    }

  public:
    wxDateTime startTime;
    wxDateTime endTime;
    wxTimeSpan gameDuration;

    wxString sMapName;
    wxString sGameName;
    
    //Is this a private game?
    bool bPrivate;

    //Are we hosting this game? Or did we join one.
    bool bHosted;
    
    //This is how many games we have hosted since wc3 was started.
    //We need this for the auto refresher. Clients use it to connect
    //to games (it's almost like authentication).
    int nHostedCount;
};

#endif
