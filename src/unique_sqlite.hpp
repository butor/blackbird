#ifndef UNIQUE_SQLITE_HPP
#define UNIQUE_SQLITE_HPP

#include "sqlite3.h"
#include <memory>

struct sqlite_deleter
{
  void operator () (sqlite3 *S)
  {
    sqlite3_close(S);
  }
  void operator () (char *errmsg)
  {
    sqlite3_free(errmsg);
  }
};

using unique_sqlite = std::unique_ptr<sqlite3, sqlite_deleter>;
using unique_sqlerr = std::unique_ptr<char, sqlite_deleter>;

#endif
