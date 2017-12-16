#include "gdax.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "unique_json.hpp"

namespace GDAX {

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://api.gdax.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params);
  std::string url("/products/");
  url += params.leg1 + "-" + params.leg2 + "/ticker";
  unique_json root { exchange.getRequest(url) };

  const char *bid, *ask;
  int unpack_fail = json_unpack(root.get(), "{s:s, s:s}", "bid", &bid, "ask", &ask);
  if (unpack_fail)
  {
    bid = "0";
    ask = "0";
  }

  return std::make_pair(std::stod(bid), std::stod(ask));
}

double getAvail(Parameters &params, std::string currency)
{
  // TODO
  return 0.0;
}

double getActivePos(Parameters &params)
{
  // TODO
  return 0.0;
}

double getLimitPrice(Parameters &params, double volume, bool isBid)
{
  // TODO
  return 0.0;
}

}
