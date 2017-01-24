#include <string.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <jansson.h>
#include "itbit.h"
#include "curl_fun.h"

namespace ItBit {

    double getQuote(Parameters &params, bool isBid) {
        bool GETRequest = false;
        json_t *root = getJsonFromUrl(params, "https://api.itbit.com/v1/markets/XBTUSD/ticker", "", GETRequest);
        const char *quote;
        double quoteValue;
        if (isBid) {
            quote = json_string_value(json_object_get(root, "bid"));
        } else {
            quote = json_string_value(json_object_get(root, "ask"));
        }
        if (quote != NULL) {
            quoteValue = atof(quote);
        } else {
            quoteValue = 0.0;
        }
        json_decref(root);
        return quoteValue;
    }

    double getAvail(Parameters &params, std::string currency) {
        // TODO
        return 0.0;
    }

    double getActivePos(Parameters &params) {
        // TODO
        return 0.0;
    }

    double getLimitPrice(Parameters &params, double volume, bool isBid) {
        // TODO
        return 0.0;
    }

}

