#include "cexio.h"
#include "parameters.h"
#include "curl_fun.h"
#include "utils/restapi.h"
#include "unique_json.hpp"


namespace CEXIO {

std::string tickerMapping(const std::string& input){
    std::string data = input;
    // TODO
    return data;
}

static RestApi& queryHandle(Parameters &params)
{
    static RestApi query ("https://cex.io",
                          params.cacert.c_str(), *params.logFile);
    return query;
}

quote_t getQuote(Parameters &params)
{
    auto &exchange = queryHandle(params);
    std::string url("/api/ticker/");
    url += (tickerMapping(params.leg1) + "/" + tickerMapping(params.leg2));
    unique_json root { exchange.getRequest(url) };
    
    double bidValue = json_real_value(json_object_get(root.get(), "bid"));

    double askValue = json_real_value(json_object_get(root.get(), "ask"));

    return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters& params, std::string currency) {
    // TODO
    return 0.0;
}

double getActivePos(Parameters& params) {
    // TODO
    return 0.0;
}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
    // TODO
    return 0.0;
}

}
