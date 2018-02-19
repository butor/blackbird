#ifndef GDAX_H
#define GDAX_H

#include "quote_t.h"
#include <string>

struct json_t;
struct Parameters;

namespace GDAX {

quote_t getQuote(Parameters& params);

double getAvail(Parameters& params, std::string currency);

double getActivePos(Parameters& params, std::string orderId="");

double getLimitPrice(Parameters& params, double volume, bool isBid);

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price);

bool isOrderComplete(Parameters& params, std::string orderId);

json_t* authRequest(Parameters& params, std::string method, std::string request,const std::string &options);

std::string gettime();

std::string pairTransform(Parameters& params);

void testGDAX();

std::string symbolTransform(Parameters& params, std::string leg);

}

#endif
