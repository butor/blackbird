#ifndef DB_FUN_H
#define DB_FUN_H

#include <string>

struct Parameters;

int createDbConnection(Parameters& params);

int createTable(std::string exchangeName, Parameters& params);

int addBidAskToDb(std::string exchangeName, std::string datetime, double bid, double ask, Parameters& params);

int createTradeTable(Parameters& params);

int addTradesToDb(Result& trade, Parameters& params, int closer);

int closeTradeInDb(Result& trade, Parameters& params);

int getNumTradesOutstanding(Parameters &params);

int getTradesFromDb(Parameters &params, std::vector<Result> &trade);

#endif
