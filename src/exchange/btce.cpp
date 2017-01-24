#include <string.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <jansson.h>
#include "btce.h"
#include "curl_fun.h"

namespace BTCe {

    double getQuote(Parameters &params, bool isBid) {
        bool GETRequest = false;
        json_t *root = getJsonFromUrl(params, "https://btc-e.com/api/3/ticker/btc_usd", "", GETRequest);
        double quoteValue;
        if (isBid) {
            quoteValue = json_real_value(json_object_get(json_object_get(root, "btc_usd"), "buy"));
        } else {
            quoteValue = json_real_value(json_object_get(json_object_get(root, "btc_usd"), "sell"));
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

