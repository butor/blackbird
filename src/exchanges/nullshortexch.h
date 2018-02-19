#ifndef NULLSHORTEXCH_H
#define NULLSHORTEXCH_H

#include "quote_t.h"
#include <string>

struct json_t;
struct Parameters;


namespace NullShortExch
{

quote_t getQuote(Parameters &params);

double getAvail(Parameters &params, std::string currency);

double getActivePos(Parameters &params, std::string orderId = "");

double getLimitPrice(Parameters &params, double volume, bool isBid);

std::string sendLongOrder(Parameters &params, std::string direction, double quantity, double price);

std::string sendShortOrder(Parameters &params, std::string direction, double quantity, double price);

bool isOrderComplete(Parameters &params, std::string orderId);
}
#endif