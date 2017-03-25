#ifndef OKCOIN_H
#define OKCOIN_H


#include <string>

struct json_t;
struct Parameters;

namespace OKCoin {

double getQuote(Parameters& params, bool isBid);

double getAvail(Parameters& params, std::string currency);

long sendLongOrder(Parameters& params, std::string direction, double quantity, double price);

long sendShortOrder(Parameters& params, std::string direction, double quantity, double price);

bool isOrderComplete(Parameters& params, long orderId);

double getActivePos(Parameters& params);

double getLimitPrice(Parameters& params, double volume, bool isBid);

json_t* authRequest(Parameters& params, std::string url, std::string signature, std::string content);

void getBorrowInfo(Parameters& params);

int borrowBtc(Parameters& params, double amount);

void repayBtc(Parameters& params, int borrowId);

}

#endif
