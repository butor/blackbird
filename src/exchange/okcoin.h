#ifndef OKCOIN_H
#define OKCOIN_H

#include <curl/curl.h>
#include <string>
#include "parameters.h"

namespace OKCoin {

    double getQuote(Parameters &params, bool isBid);

    double getAvail(Parameters &params, std::string currency);

    int sendLongOrder(Parameters &params, std::string direction, double quantity, double price);

    int sendShortOrder(Parameters &params, std::string direction, double quantity, double price);

    bool isOrderComplete(Parameters &params, int orderId);

    double getActivePos(Parameters &params);

    double getLimitPrice(Parameters &params, double volume, bool isBid);

    json_t *authRequest(Parameters &params, std::string url, std::string signature, std::string content);

    void getBorrowInfo(Parameters &params);

    int borrowBtc(Parameters &params, double amount);

    void repayBtc(Parameters &params, int borrowId);

}

#endif

