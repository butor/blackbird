#ifndef BTCE_H
#define BTCE_H

#include <curl/curl.h>
#include <string>
#include "parameters.h"

namespace BTCe {

    double getQuote(Parameters &params, bool isBid);

    double getAvail(Parameters &params, std::string currency);

    double getActivePos(Parameters &params);

    double getLimitPrice(Parameters &params, double volume, bool isBid);

}

#endif

