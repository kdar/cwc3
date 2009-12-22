#ifndef _DATABASE_H
#define _DATABASE_H

#include <sqlite3.h>

#include "Object.h"

//-------------------------------
class Database : public Object<Database>
{
  public:
    Database(const string &sDbName);    
    ~Database()
    { sqlite3_close(m_pDb); }
  
    int Exec(const string &sExe);
  
  public:
    ArrayString vcol;
    ArrayString vdata;
  
  private:
    sqlite3 *m_pDb;       
};

#endif
