#ifndef OKCOIN_H
#define OKCOIN_H

#include "quote_t.h"
#include <string>

struct json_t;
struct Parameters;

namespace OKCoin {

quote_t getQuote(Parameters& params);

double getAvail(Parameters& params, std::string currency);

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price);

std::string sendShortOrder(Parameters& params, std::string direction, double quantity, double price);

bool isOrderComplete(Parameters& params, std::string orderId);

double getActivePos(Parameters& params, std::string orderId="");

double getLimitPrice(Parameters& params, double volume, bool isBid);

json_t* authRequest(Parameters& params, std::string url, std::string signature, std::string content);

void getBorrowInfo(Parameters& params);

int borrowBtc(Parameters& params, double amount);

void repayBtc(Parameters& params, int borrowId);

std::string symbolTransform(Parameters& params, std::string leg);

}

#endif
