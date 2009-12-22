#include "Command.h"

//===============================
Command::Command(int nCount, ...)
{ 
  va_list arguments;

  va_start(arguments, nCount);

  for (u_int x = 0; x < nCount; x++) {
    m_asStrings.Add(va_arg(arguments, char*));
  }

  va_end(arguments);
}
