#ifndef BITFINEX_H
#define BITFINEX_H


#include <string>

struct json_t;
struct Parameters;

namespace Bitfinex {

double getQuote(Parameters& params, bool isBid);

double getAvail(Parameters& params, std::string currency);

long sendLongOrder(Parameters& params, std::string direction, double quantity, double price);

long sendShortOrder(Parameters& params, std::string direction, double quantity, double price);

long sendOrder(Parameters& params, std::string direction, double quantity, double price);

bool isOrderComplete(Parameters& params, long orderId);

double getActivePos(Parameters& params);

double getLimitPrice(Parameters& params, double volume, bool isBid);

json_t* authRequest(Parameters& params, std::string url, std::string request, std::string options);

}

#endif
