//
// Created by Macbook on 6/29/17.
//

#ifndef BLACKBIRD_UTILS_H
#define BLACKBIRD_UTILS_H

#include "../parameters.h"
#include "../quote_t.h"
#include "iostream"

using namespace std;

// The 'typedef' declarations needed for the function arrays
// These functions contain everything needed to communicate with
// the exchanges, like getting the quotes or the active positions.
// Each function is implemented in the files located in the 'exchanges' folder.
typedef quote_t     (*getQuoteType)(Parameters &params);

typedef double      (*getAvailType)(Parameters &params, std::__1::string currency);

typedef std::string (*sendOrderType)(Parameters &params, std::__1::string direction, double quantity, double price);

typedef bool        (*isOrderCompleteType)(Parameters &params, std::__1::string orderId);

typedef double      (*getActivePosType)(Parameters &params);

typedef double      (*getLimitPriceType)(Parameters &params, double volume, bool isBid);

// This structure contains the balance of both exchanges,
// *before* and *after* an arbitrage trade.
// This is used to compute the performance of the trade,
// by comparing the balance before and after the trade.
struct Balance {
    double leg1, leg2;
    double leg1After, leg2After;
};
#endif //BLACKBIRD_UTILS_H
