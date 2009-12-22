#ifndef _CONFIG_H
#define _CONFIG_H

//This should move into a configuration file and a Config class
//wxFileConfig can help with this.

#define SNIFF_DEVICE "eth1"
#define INTERNET_DEVICE "ppp0"
#define GAME_HOST "192.168.0.2"
#define GAME_PORT 6112
#define BATTLE_PORT 6112
#define DATA_PREFIXES "FF,F7"
#define ALLOWED_COUNTRIES "US,CA"
#define MAX_PING 225
#define AUTOKICK_ON_INVALID_COUNTRY true
#define AUTOKICK_ON_INVALID_PING true
#define AUTOKICK_ON_MULTIPLE_IP true
#define WARN_ON_INVALID_COUNTRY false
#define WARN_ON_INVALID_PING false
#define WARN_ON_MULTIPLE_IP false

//There is an autokick list that is checked everytime someone joins. If their ip
//is in that list, they will be automatically kicked. What these two options mean
//is if they were kicked for two clients having the same IP or they were slot kicked,
//then they are added to the auto kick list so they can't join again.
#define ADD_AUTOKICK_ON_MULTIPLE_IP_KICK true
//If we slot kicked someone, then continually kick them if they try to join again.
#define ADD_AUTOKICK_ON_SLOT_KICK true

#define ANNOUNCE_ON_KLINE false

#define AUTOKICK_REASON true
#define AUTOKICK_EXCEPTIONS "tehsemi,megadaffy,thedarth,klutchzilla,hotnix,jimmydeano,moll420ll305ll,eltee"
//The amount of time it takes from when the client stops responding, to when we pronounce them as disconnected.
#define CLIENT_TIMEOUT 55

static const char *g_asColorMap[]               = {"red", "blue", "teal", "purple", "yellow", "orange",
                                                   "green", "pink", "grey", "light blue", "dark green", "brown"};

static const char *g_asTeamMap[]                = {"Sentinel", "Scourge"};

static const char *g_asClientDisplayDefault[]   = {"Computer - Sentinel", "Open", "Open", "Open", "Open", "Open", 
                                                   "Computer - Scourge", "Open", "Open", "Open", "Open", "Open"};
static const char *g_asClientSlotDisplay[]      = {"00", "01", "02", "03", "04", "05", 
                                                   "06", "07", "08", "09", "10", "11"};
static const char *g_asClientDisplayFormatMap[] = {"<L></B/16>", "<L>", "<L>", "<L>", "<L>", "<L>", 
                                                   "<L></B/24>", "<L>", "<L>", "<L>", "<L>", "<L>"};
static const char *g_asClientSlotFormatMap[]    = {"<L></B/16>", "<L></N/40>", "<L></N/56>", "<L></N/48>", "<L></B/32>", "<L></B/16>", 
                                                   "<L></B/24>", "<L></B/48>", "<L></D/08>", "<L></B/40>", "<L></D/24>", "<L></N/32>"};

#endif
