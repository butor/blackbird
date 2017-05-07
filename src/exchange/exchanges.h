#ifndef EXCHANGES_H
#define EXCHANGES_H

#include "parameters.h"
#include "quote_t.h"
#include "db_fun.h"
#include <string>

typedef quote_t (*getQuoteType) (Parameters& params);
typedef double (*getAvailType) (Parameters& params, std::string currency);
typedef std::string (*sendOrderType) (Parameters& params, std::string direction, double quantity, double price);
typedef bool (*isOrderCompleteType) (Parameters& params, std::string orderId);
typedef double (*getActivePosType) (Parameters& params);
typedef double (*getLimitPriceType) (Parameters& params, double volume, bool isBid);

// function arrays containing all the exchanges functions
extern getQuoteType getQuote[10];
extern getAvailType getAvail[10];
extern sendOrderType sendLongOrder[10];
extern sendOrderType sendShortOrder[10];
extern isOrderCompleteType isOrderComplete[10];
extern getActivePosType getActivePos[10];
extern getLimitPriceType getLimitPrice[10];
extern std::string dbTableName[10];

int initExchanges(Parameters& params);

#endif