#include "parameters.h"
#include "unique_sqlite.hpp"
#include "result.h"
#include <iostream>
#include <string>


// Defines some helper overloads to ease sqlite resource management
namespace {
template <typename UNIQUE_T>
class sqlite_proxy {
  typename UNIQUE_T::pointer S;
  UNIQUE_T &owner;

public:
  sqlite_proxy(UNIQUE_T &owner) : S(nullptr), owner(owner)
  {}
  ~sqlite_proxy()                           { owner.reset(S); }
  operator typename UNIQUE_T::pointer* ()   { return &S;      }
};

template <typename T, typename deleter>
sqlite_proxy< std::unique_ptr<T, deleter> >
acquire(std::unique_ptr<T, deleter> &owner) { return owner;   }
}

int createDbConnection(Parameters& params) {
  int res = sqlite3_open(params.dbFile.c_str(), acquire(params.dbConn));
  
  if (res != SQLITE_OK)
    std::cerr << sqlite3_errmsg(params.dbConn.get()) << std::endl;

  return res;
}

int createTable(std::string exchangeName, Parameters& params) {
  
  std::string query = "CREATE TABLE IF NOT EXISTS `" + exchangeName +
                      "` (Datetime DATETIME NOT NULL, bid DECIMAL(8, 2), ask DECIMAL(8, 2));";
  unique_sqlerr errmsg;
  int res = sqlite3_exec(params.dbConn.get(), query.c_str(), nullptr, nullptr, acquire(errmsg));
  if (res != SQLITE_OK)
    std::cerr << errmsg.get() << std::endl;

  return res;
}

int addBidAskToDb(std::string exchangeName, std::string datetime, double bid, double ask, Parameters& params) {
  std::string query = "INSERT INTO `" + exchangeName +
                      "` VALUES ('"   + datetime +
                      "'," + std::to_string(bid) +
                      "," + std::to_string(ask) + ");";
  unique_sqlerr errmsg;
  int res = sqlite3_exec(params.dbConn.get(), query.c_str(), nullptr, nullptr, acquire(errmsg));
  if (res != SQLITE_OK)
    std::cerr << errmsg.get() << std::endl;

  return res;
}

int createTradeTable(Parameters &params)
{
  std::string query = "CREATE TABLE IF NOT EXISTS trades ("
                      "idexchlong INT, idexchshort INT,"
                      "exchnamelong TEXT, exchnameshort TEXT, exposure REAL,"
                      "feeslong REAL, feesshort REAL, entryTime DATETIME, spreadin REAL,"
                      "pricelongin REAL, priceshortin REAL, leg2bal REAL, exittarget REAL,"
                      "longexchtradeid TEXT, shortexchtradeid TEXT, maxspread REAL, minspread REAL,"
                      "trailing REAL, trailingwaitcount REAL, isclosed INT);";
  unique_sqlerr errmsg;
  int res = sqlite3_exec(params.dbConn.get(), query.c_str(), nullptr, nullptr, acquire(errmsg));
  if (res != SQLITE_OK)
    std::cerr << errmsg.get() << std::endl;

  return res;
}
// TODO: Clean this shit and make closing optional in addtrades
int addTradesToDb(Result &trade, Parameters &params, int closer)
{
  std::stringstream ss;
  ss << "INSERT INTO trades"
     << " (idexchlong, idexchshort, exchnamelong, exchnameshort,"
     << " exposure, feeslong, feesshort, entryTime, spreadin, pricelongin, priceshortin, "
     << "leg2bal, exittarget, longexchtradeid, shortexchtradeid, maxspread, minspread, trailing, "
     << "trailingwaitcount, isclosed) "
     << "VALUES ("
     << trade.idExchLong << " ," << trade.idExchShort
     << " ,'" << trade.exchNameLong << "' ,'" << trade.exchNameShort
     << "' ," << trade.exposure
     << " ," << trade.feesLong
     << " ," << trade.feesShort
     << " ," << trade.entryTime
     << " ," << trade.spreadIn
     << " ," << trade.priceLongIn
     << " ," << trade.priceShortIn
     << " ," << trade.leg2TotBalanceBefore
     << " ," << trade.exitTarget
     << " ,'" << trade.longExchTradeId.c_str()
     << "' ,'" << trade.shortExchTradeId.c_str()
     << "' ," << trade.maxSpread
     << " ," << trade.minSpread
     << " ," << trade.trailing
     << " ," << trade.trailingWaitCount
     << " ," << closer << ");";
  std::string s = ss.str();
  std::string concat = s;
  unique_sqlerr errmsg;
  int res = sqlite3_exec(params.dbConn.get(), concat.c_str(), nullptr, nullptr, acquire(errmsg));
  if (res != SQLITE_OK)
    std::cerr << errmsg.get() << std::endl;
  return res;
}
int closeTradeInDb(Result &trade, Parameters &params)
{
  std::string query = "UPDATE trades SET isclosed='1' WHERE rowid=(SELECT rowid from trades WHERE (longExchTradeId='" + trade.longExchTradeId + "') OR (shortExchTradeId='" + trade.shortExchTradeId + "'));";
  unique_sqlerr errmsg;
  int res = sqlite3_exec(params.dbConn.get(), query.c_str(), nullptr, nullptr, acquire(errmsg));
  if (res != SQLITE_OK)
    std::cerr << errmsg.get() << std::endl;
  return res;
}

int count_cb(void *data, int count, char **rows, char **)
{
  if (count == 1 && rows)
  {
    *static_cast<int *>(data) = atoi(rows[0]);
    return 0;
  }
  return 1;
}
int getNumTradesOutstanding(Parameters &params)
{
  int count = 0;
  std::string query = "SELECT count(*) FROM trades WHERE isclosed = 0";
  unique_sqlerr errmsg;
  int res = sqlite3_exec(params.dbConn.get(), query.c_str(), count_cb, &count, acquire(errmsg));
  if (res != SQLITE_OK)
    std::cerr << errmsg.get() << std::endl;
  return count;
}

static int trades_cb(void *data, int argc, char **argv, char **azColName)
{
  std::vector<Result> *list = reinterpret_cast<std::vector<Result> *>(data);
  Result d;
  d.id = atoi(argv[0]);
  d.idExchLong = atoi(argv[1]);
  d.idExchShort = atoi(argv[2]);
  d.exchNameLong = argv[3];
  d.exchNameShort = argv[4];
  d.exposure = atof(argv[5]);
  d.feesLong = atof(argv[6]);
  d.feesShort = atof(argv[7]);
  d.entryTime = atof(argv[8]);
  d.spreadIn = atof(argv[9]);
  d.priceLongIn = atof(argv[10]);
  d.priceShortIn = atof(argv[11]);
  d.leg2TotBalanceBefore = atof(argv[12]);
  d.exitTarget = atof(argv[13]);
  d.longExchTradeId = argv[14];
  d.shortExchTradeId = argv[15];
  d.maxSpread = atof(argv[16]);
  d.minSpread = atof(argv[17]);
  d.trailing = atof(argv[18]);
  d.trailingWaitCount = atof(argv[19]);
  list->push_back(d);

  return 0;
}
int getTradesFromDb(Parameters &params, std::vector<Result> &trade)
{
  std::string query = "SELECT rowid, idexchlong, idexchshort, exchnamelong, exchnameshort,"
                      "exposure, feeslong, feesshort, entryTime, spreadin, pricelongin, priceshortin, "
                      "leg2bal, exittarget, longexchtradeid, shortexchtradeid, maxspread, minspread, trailing, "
                      "trailingwaitcount FROM trades WHERE isclosed = 0";
  unique_sqlerr errmsg;

  int res = sqlite3_exec(params.dbConn.get(), query.c_str(), trades_cb, &trade, acquire(errmsg));
  if (res != SQLITE_OK)
  {
    std::cerr << errmsg.get() << std::endl;
  }
  return res;
}