#ifndef QUADRIGACX_H
#define QUADRIGACX_H

#include "quote_t.h"
#include <string>
#include <sstream>

struct json_t;
struct Parameters;

namespace QuadrigaCX {

quote_t getQuote(Parameters& params);

double getAvail(Parameters& params, std::string currency);

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price);

bool isOrderComplete(Parameters& params, std::string orderId);

double getActivePos(Parameters& params, std::string orderId="" );

double getLimitPrice(Parameters& params, double volume, bool isBid);

void testQuadriga();

std::string symbolTransform(Parameters& params, std::string leg);
}

#endif
