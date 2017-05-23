#ifndef BTCE_H
#define BTCE_H

#include "quote_t.h"
#include <string>

struct Parameters;

namespace BTCe {

quote_t getQuote(Parameters& params);

double getAvail(Parameters& params, std::string currency);

bool isOrderComplete(Parameters& params, std::string orderId);

double getActivePos(Parameters& params);

double getLimitPrice(Parameters& params, double volume, bool isBid);

}

#endif
