#include "kraken.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "utils/base64.h"
#include "unique_json.hpp"

#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <iomanip>
#include <vector>
#include <array>
#include <sys/time.h>

namespace Kraken {

// Initialise internal variables
static unique_json krakenTicker = nullptr;
static bool krakenGotTicker = false;
static unique_json krakenLimPrice = nullptr;
static bool krakenGotLimPrice = false;

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://api.kraken.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{
  if (krakenGotTicker) {
    krakenGotTicker = false;
  } else {
    auto &exchange = queryHandle(params);
    krakenTicker.reset(exchange.getRequest("/0/public/Ticker?pair=XXBTZUSD"));
    krakenGotTicker = true;
  }
  json_t *root = krakenTicker.get();
  const char *quote = json_string_value(json_array_get(json_object_get(json_object_get(json_object_get(root, "result"), "XXBTZUSD"), "b"), 0));
  auto bidValue = quote ? std::stod(quote) : 0.0;

  quote = json_string_value(json_array_get(json_object_get(json_object_get(json_object_get(root, "result"), "XXBTZUSD"), "a"), 0));
  auto askValue = quote ? std::stod(quote) : 0.0;

  return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters& params, std::string currency) {
  unique_json root { authRequest(params, "/0/private/Balance") };
  json_t *result = json_object_get(root.get(), "result");
  if (json_object_size(result) == 0) {
    return 0.0;
  }
  double available = 0.0;
  if (currency.compare("usd") == 0) {
    const char * avail_str = json_string_value(json_object_get(result, "ZUSD"));
    available = avail_str ? atof(avail_str) : 0.0;
  } else if (currency.compare("btc") == 0) {
    const char * avail_str = json_string_value(json_object_get(result, "XXBT"));
    available = avail_str ? atof(avail_str) : 0.0;
  } else {
    *params.logFile << "<Kraken> Currency not supported" << std::endl;
  }
  return available;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0) {
    *params.logFile  << "<Kraken> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
    return "0";
  }
  *params.logFile << "<Kraken> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(6) << quantity << " @ $"
                  << std::setprecision(2) << price << "...\n";
  std::string pair = "XXBTZUSD";
  std::string type = direction;
  std::string ordertype = "limit";
  std::string pricelimit = std::to_string(price);
  std::string volume = std::to_string(quantity);
  std::string options = "pair=" + pair + "&type=" + type + "&ordertype=" + ordertype + "&price=" + pricelimit + "&volume=" + volume;
  unique_json root { authRequest(params, "/0/private/AddOrder", options) };
  json_t *res = json_object_get(root.get(), "result");
  if (json_is_object(res) == 0) {
    *params.logFile << json_dumps(root.get(), 0) << std::endl;
    exit(0);
  }
  std::string txid = json_string_value(json_array_get(json_object_get(res, "txid"), 0));
  *params.logFile << "<Kraken> Done (transaction ID: " << txid << ")\n" << std::endl;
  return txid;
}

bool isOrderComplete(Parameters& params, std::string orderId) {
  unique_json root { authRequest(params, "/0/private/OpenOrders") };
  // no open order: return true
  auto res = json_object_get(json_object_get(root.get(), "result"), "open");
  if (json_object_size(res) == 0) {
    *params.logFile << "<Kraken> No order exists" << std::endl;
    return true;
  }
  *params.logFile << json_dumps(res, 0) << std::endl;
  res = json_object_get(res, orderId.c_str());
  // open orders exist but specific order not found: return true
  if (json_object_size(res) == 0) {
    *params.logFile << "<Kraken> Order " << orderId << " does not exist" << std::endl;
    return true;
  // open orders exist and specific order was found: return false
  } else {
    *params.logFile << "<Kraken> Order " << orderId << " still exists!" << std::endl;
    return false;
  }
}

double getActivePos(Parameters& params) {
  return getAvail(params, "btc");
}

double getLimitPrice(Parameters &params, double volume, bool isBid)
{
  if (!krakenGotLimPrice)
  {
    auto &exchange = queryHandle(params);
    krakenLimPrice.reset(exchange.getRequest("/0/public/Depth?pair=XXBTZUSD"));
  }
  krakenGotLimPrice = !krakenGotLimPrice;
  auto root = krakenLimPrice.get();
  auto branch = json_object_get(json_object_get(root, "result"), "XXBTZUSD");
  branch = json_object_get(branch, isBid ? "bids" : "asks");

  // loop on volume
  double tmpVol = 0.0;
  int i = 0;
  // [[<price>, <volume>, <timestamp>], [<price>, <volume>, <timestamp>], ...]
  while (tmpVol < volume) {
    // volumes are added up until the requested volume is reached
    tmpVol += atof(json_string_value(json_array_get(json_array_get(branch, i), 1)));
    i++;
  }

  return atof(json_string_value(json_array_get(json_array_get(branch, i-1), 0)));
}

json_t* authRequest(Parameters& params, std::string request, std::string options)
{
  using namespace std;
  // create nonce and POST data
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long nonce = (tv.tv_sec * 1000.0) + (tv.tv_usec * 0.001) + 0.5;
  string post_data = "nonce=" + to_string(nonce);
  if (!options.empty())
    post_data += "&" + options;

  // Message signature using HMAC-SHA512 of (URI path + SHA256(nonce + POST data))
  // and base64 decoded secret API key
  auto sig_size = request.size() + SHA256_DIGEST_LENGTH;
  vector<uint8_t> sig_data(sig_size);
  copy(begin(request), end(request), begin(sig_data));

  string payload_for_signature = to_string(nonce) + post_data;
  SHA256((uint8_t *)payload_for_signature.c_str(), payload_for_signature.size(),
         &sig_data[ sig_size - SHA256_DIGEST_LENGTH ]);

  string decoded_key = base64_decode(params.krakenSecret);
  uint8_t *hmac_digest = HMAC(EVP_sha512(),
                              decoded_key.c_str(), decoded_key.length(),
                              sig_data.data(), sig_data.size(), NULL, NULL);
  string api_sign_header = base64_encode(hmac_digest, SHA512_DIGEST_LENGTH);
  // cURL header
  array<string, 2> headers
  {
    "API-KEY:"  + params.krakenApi,
    "API-Sign:" + api_sign_header,
  };

  // cURL request
  auto &exchange = queryHandle(params);
  return exchange.postRequest(request,
                              make_slist(begin(headers), end(headers)),
                              post_data);
}

}
