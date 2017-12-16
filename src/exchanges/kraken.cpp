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
#include <ctime>

namespace Kraken {

// Initialise internal variables
static unique_json krakenTicker = nullptr;
static bool krakenGotTicker = false;
static unique_json krakenLimPrice = nullptr;
static bool krakenGotLimPrice = false;

std::string tickerMapping(const std::string& input){
  std::string canonicalAsset;
  std::transform(input.begin(), input.end(), canonicalAsset
          .begin(), ::toupper);
  // TODO make sure the mapping is complete
  if (canonicalAsset == "BTC") {
    return "XBT";
  }
  else {
    return canonicalAsset;
  }
}

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
    std::string ccyPair = tickerMapping(params.leg1) + tickerMapping(params.leg2);
    krakenTicker.reset(exchange.getRequest("/0/public/Ticker?pair=" + ccyPair));
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
  auto dump = json_dumps(res, 0);
  *params.logFile << dump << std::endl;
  free(dump);
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
  double totVol = 0.0;
  double currPrice = 0;
  double currVol = 0;
  unsigned int i;
  // [[<price>, <volume>, <timestamp>], [<price>, <volume>, <timestamp>], ...]
  for(i = 0; i < json_array_size(branch); i++)
  {
    // volumes are added up until the requested volume is reached
    currVol = atof(json_string_value(json_array_get(json_array_get(branch, i), 1)));
    currPrice = atof(json_string_value(json_array_get(json_array_get(branch, i), 0)));
    totVol += currVol;
    if(totVol >=  volume * params.orderBookFactor)
        break;
  }

  return currPrice;
}

json_t* authRequest(Parameters& params, std::string request, std::string options)
{
  // create nonce and POST data
  static uint64_t nonce = time(nullptr) * 4;
  std::string post_data = "nonce=" + std::to_string(++nonce);
  if (!options.empty())
    post_data += "&" + options;

  // Message signature using HMAC-SHA512 of (URI path + SHA256(nonce + POST data))
  // and base64 decoded secret API key
  auto sig_size = request.size() + SHA256_DIGEST_LENGTH;
  std::vector<uint8_t> sig_data(sig_size);
  copy(std::begin(request), std::end(request), std::begin(sig_data));

  std::string payload_for_signature = std::to_string(nonce) + post_data;
  SHA256((uint8_t *)payload_for_signature.c_str(), payload_for_signature.size(),
         &sig_data[ sig_size - SHA256_DIGEST_LENGTH ]);

  std::string decoded_key = base64_decode(params.krakenSecret);
  uint8_t *hmac_digest = HMAC(EVP_sha512(),
                              decoded_key.c_str(), decoded_key.length(),
                              sig_data.data(), sig_data.size(), NULL, NULL);
  std::string api_sign_header = base64_encode(hmac_digest, SHA512_DIGEST_LENGTH);
  // cURL header
  std::array<std::string, 2> headers
  {
    "API-KEY:"  + params.krakenApi,
    "API-Sign:" + api_sign_header,
  };

  // cURL request
  auto &exchange = queryHandle(params);
  return exchange.postRequest(request,
                              make_slist(std::begin(headers), std::end(headers)),
                              post_data);
}

}
