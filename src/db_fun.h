#ifndef DB_FUN_H
#define DB_FUN_H

#include <string>
#include "parameters.h"

int createDbConnection(Parameters &params);

int createTable(std::string exchangeName, Parameters &params);

int addBidAskToDb(std::string exchangeName, std::string datetime, double bid, double ask, Parameters &params);

#endif

