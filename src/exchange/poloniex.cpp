#include "poloniex.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "unique_json.hpp"
#include "hex_str.hpp"

#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <array>
#include <algorithm>
#include <time.h>
#include <ctype.h>

namespace Poloniex {

static json_t* authRequest(Parameters &, const char *, const std::string & = "");

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://poloniex.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

static json_t* checkResponse(std::ostream &logFile, json_t *root)
{
  auto errmsg = json_object_get(root, "error");
  if (errmsg)
    logFile << "<Poloniex> Error with response: "
            << json_string_value(errmsg) << '\n';

  return root;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/public?command=returnTicker") };

  const char *quote = json_string_value(json_object_get(json_object_get(root.get(), "USDT_BTC"), "highestBid"));
  auto bidValue = quote ? std::stod(quote) : 0.0;

  quote = json_string_value(json_object_get(json_object_get(root.get(), "USDT_BTC"), "lowestAsk"));
  auto askValue = quote ? std::stod(quote) : 0.0;

  return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters &params, std::string currency)
{
  std::transform(begin(currency), end(currency), begin(currency), ::toupper);
  std::string options = "account=margin";
  unique_json root { authRequest(params, "returnAvailableAccountBalances", options) };
  auto funds = json_string_value(json_object_get(json_object_get(root.get(), "margin"), currency.c_str()));
  return funds ? std::stod(funds) : 0.0;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  // TODO
  return "0";
}

std::string sendShortOrder(Parameters& params, std::string direction, double quantity, double price) {
  // TODO
  return "0";
}

bool isOrderComplete(Parameters& params, std::string orderId)
{
  unique_json root { authRequest(params, "returnOpenOrders", "currencyPair=USDT_BTC") };
  auto n = json_array_size(root.get());
  while (n --> 0)
  {
    auto item = json_object_get(json_array_get(root.get(), n), "orderNumber");
    if (orderId == json_string_value(item))
      return false;
  }
  return true;
}

double getActivePos(Parameters& params) {
  // TODO
  return 0.0;
}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
  // TODO
  return 0.0;
}

json_t* authRequest(Parameters &params, const char *request, const std::string &options)
{
  using namespace std;
  static uint64_t nonce = time(nullptr) * 4;
  string post_body = "nonce="   + to_string(++nonce) +
                     "&command=" + request;
  if (!options.empty())
  {
    post_body += '&';
    post_body += options;
  }

  uint8_t *sign = HMAC (EVP_sha512(),
                        params.poloniexSecret.data(), params.poloniexSecret.size(),
                        reinterpret_cast<const uint8_t *>(post_body.data()), post_body.size(),
                        nullptr, nullptr);
  auto &exchange = queryHandle(params);
  array<string, 2> headers
  {
    "Key:"  + params.poloniexApi,
    "Sign:" + hex_str(sign, sign + SHA512_DIGEST_LENGTH),
  };
  return checkResponse (*params.logFile,
                        exchange.postRequest ("/tradingApi",
                                              make_slist(begin(headers), end(headers)),
                                              post_body));
}
  
}
