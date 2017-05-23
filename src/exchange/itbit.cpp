#include "itbit.h"
#include "parameters.h"
#include "curl_fun.h"
#include "utils/restapi.h"
#include "unique_json.hpp"

#include <unistd.h>

namespace ItBit {

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://api.itbit.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/v1/markets/XBTUSD/ticker") };

  const char *quote = json_string_value(json_object_get(root.get(), "bid"));
  auto bidValue = quote ? std::stod(quote) : 0.0;

  quote = json_string_value(json_object_get(root.get(), "ask"));
  auto askValue = quote ? std::stod(quote) : 0.0;

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
