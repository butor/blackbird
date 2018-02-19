#ifndef EXMO_H
#define EXMO_H

#include "quote_t.h"
#include <string>
#include <sstream>
#include <algorithm>

struct json_t;
struct Parameters;

namespace Exmo {

quote_t getQuote(Parameters& params);

double getAvail(Parameters& params, std::string currency);

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price);
// TODO multi currency support
//std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price, std::string pair = "btc_usd");

bool isOrderComplete(Parameters& params, std::string orderId);

double getActivePos(Parameters& params, std::string orderId="");

double getLimitPrice(Parameters& params, double volume, bool isBid);

void testExmo();

std::string symbolTransform(Parameters& params, std::string leg);

}

#endif
