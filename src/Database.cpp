//Inspired by http://freshmeat.net/articles/view/1428/

#include "Database.h"

//===============================
Database::Database(const string &sDbName)
{
  int rc = sqlite3_open(sDbName.c_str(), &m_pDb);
  if (rc) {
    //fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(m_pDb);
  }
}

//===============================
int Database::Exec(const string &sExe)
{
  char **result;
  int nrow;
  int ncol;
  char *szErrMsg;
  int rc;
    
  rc = sqlite3_get_table(
  	m_pDb,           /* An open database */
  	sExe.c_str(),    /* SQL to be executed */
  	&result,         /* Result written to a char *[]  that this points to */
  	&nrow,           /* Number of result rows written here */
  	&ncol,           /* Number of result columns written here */
  	&szErrMsg        /* Error msg written here */
	);

  if (vcol.size() > 0) 
    vcol.clear();
  if (vdata.size() > 0)
    vdata.clear();

  if (rc == SQLITE_OK) {
    for (int i=0; i < ncol; ++i) {
      vcol.push_back(result[i]); // First row heading
    }
    for (int i=0; i < ncol*nrow; ++i) {
      vdata.push_back(result[ncol+i]);
    }
  }
 
  sqlite3_free_table(result);
  return rc;
}
