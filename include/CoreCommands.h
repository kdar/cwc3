#ifndef _CORECOMMANDS_H
#define _CORECOMMANDS_H

#include "Command.h"
#include "Lib.h"

//Forward declarations.
class CommandHandler;

void RegisterCoreCommands(shared_ptr<CommandHandler> pHandler);

DECLARE_COMMAND(HelpCmd, "help")
DECLARE_MULTI_COMMAND(ExitCmd, 2, "exit", "quit")
DECLARE_COMMAND(RefreshUICmd, "refreshui")
DECLARE_COMMAND(RefreshCmd, "refresh")
DECLARE_COMMAND(ListSnapshotCmd, "listsnapshot")
DECLARE_MULTI_COMMAND(InfoCmd, 2, "info", "finfo")
DECLARE_COMMAND(ListClientsCmd, "listclients")
DECLARE_COMMAND(LoadTimeCmd, "loadtime")
DECLARE_COMMAND(LoadMinMaxCmd, "loadminmax")
DECLARE_COMMAND(ListLoadTimeCmd, "listloadtime")
DECLARE_COMMAND(ConfigCmd, "config")
DECLARE_COMMAND(ListInfoCmd, "listinfo")
DECLARE_COMMAND(SpeakCmd, "speak")

#endif
