//
// Created by ychen607 on 12/10/17.
//

#ifndef BLACKBIRD_CEXIO_H
#define BLACKBIRD_CEXIO_H

#include "quote_t.h"

#include <string>

struct json_t;
struct Parameters;

namespace CEXIO {

quote_t getQuote(Parameters& params);

double getAvail(Parameters& params, std::string currency);

double getActivePos(Parameters& params);

double getLimitPrice(Parameters& params, double volume, bool isBid);

}

#endif //BLACKBIRD_CEXIO_H
