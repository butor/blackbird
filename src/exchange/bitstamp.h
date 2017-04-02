#ifndef BITSTAMP_H
#define BITSTAMP_H

#include <curl/curl.h>
#include <string>
#include "parameters.h"

namespace Bitstamp {

double getQuote(Parameters& params, bool isBid);

double getAvail(Parameters& params, std::string currency);

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price);

bool isOrderComplete(Parameters& params, std::string orderId);

double getActivePos(Parameters& params);

double getLimitPrice(Parameters& params, double volume, bool isBid);

json_t* authRequest(Parameters& params, std::string url, std::string options);

}

#endif
