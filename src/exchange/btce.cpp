#include "btce.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "unique_json.hpp"
#include "hex_str.hpp"

#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <array>
#include <time.h>
#include <assert.h>

namespace BTCe {

static json_t* authRequest(Parameters &, const char *, const std::string & = "");
static json_t* adjustResponse(json_t *);

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://btc-e.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

static json_t* checkResponse(std::ostream &logFile, json_t *root)
{
  unique_json own { root };
  auto success = json_object_get(root, "success");
  if (json_integer_value(success) == 0)
  {
    auto errmsg = json_object_get(root, "error");
    logFile << "<BTC-e> Error with response: "
            << json_string_value(errmsg) << '\n';
  }

  auto result = json_object_get(root, "return");
  json_incref(result);
  return result;
}

quote_t getQuote(Parameters& params)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/api/3/ticker/btc_usd") };

  double bidValue = json_number_value(json_object_get(json_object_get(root.get(), "btc_usd"), "sell"));
  double askValue = json_number_value(json_object_get(json_object_get(root.get(), "btc_usd"), "buy"));

  return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters &params, std::string currency)
{
  unique_json root { authRequest(params, "getInfo") };
  auto funds = json_object_get(json_object_get(root.get(), "funds"), currency.c_str());
  return json_number_value(funds);
}

bool isOrderComplete(Parameters& params, std::string orderId)
{
  if (orderId == "0") return true;
  unique_json root { authRequest(params, "ActiveOrders", "pair=btc_usd") };

  return json_object_get(root.get(), orderId.c_str()) == nullptr;
}

double getActivePos(Parameters& params) {
  // TODO
  return 0.0;
}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
  // TODO
  return 0.0;
}

/*
 * This is here to handle annoying inconsistences in btce's api.
 * For example, if there are no open orders, the 'ActiveOrders'
 * method returns an *error* instead of an empty object/array.
 * This function turns that error into an empty object for sake
 * of regularity.
 */
json_t* adjustResponse(json_t *root)
{
  auto errmsg = json_object_get(root, "error");
  if (!errmsg) return root;

  if (json_string_value(errmsg) == std::string("no orders"))
  {
    int err = 0;
    err += json_integer_set(json_object_get(root, "success"), 1);
    err += json_object_set_new(root, "return", json_object());
    err += json_object_del(root, "error");
    assert (err == 0);
  }

  return root;
}

json_t* authRequest(Parameters &params, const char *request, const std::string &options)
{
  using namespace std;
  static uint64_t nonce = time(nullptr) * 4;
  string post_body = "nonce="   + to_string(++nonce) +
                     "&method=" + request;
  if (!options.empty())
  {
    post_body += '&';
    post_body += options;
  }

  uint8_t *sign = HMAC (EVP_sha512(),
                        params.btceSecret.data(), params.btceSecret.size(),
                        reinterpret_cast<const uint8_t *>(post_body.data()), post_body.size(),
                        nullptr, nullptr);
  auto &exchange = queryHandle(params);
  array<string, 2> headers
  {
    "Key:"  + params.btceApi,
    "Sign:" + hex_str(sign, sign + SHA512_DIGEST_LENGTH),
  };
  auto result = exchange.postRequest ("/tapi",
                                      make_slist(begin(headers), end(headers)),
                                      post_body);
  return checkResponse(*params.logFile, adjustResponse(result));
}

}
