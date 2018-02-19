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

namespace Kraken
{

// Initialise internal variables
static unique_json krakenTicker = nullptr;
static bool krakenGotTicker = false;


static RestApi &queryHandle(Parameters &params)
{
  static RestApi query("https://api.kraken.com",
                       params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{
  if (krakenGotTicker)
  {
    krakenGotTicker = false;
  }
  else
  {
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

double getAvail(Parameters &params, std::string currency)
{
  double available = 0.00;
  currency = symbolTransform(params, currency);
  //kraken happens to still need an if/then because we need margin free balance
  if (currency.compare("ZUSD") == 0) {
    unique_json root { authRequest(params, "/0/private/TradeBalance", "asset="+currency)};
    json_t *result = json_object_get(root.get(), "result");
    const char * avail_str = json_string_value(json_object_get(result, "mf"));
    available = avail_str ? atof(avail_str) : 0.0;
  } else {
    unique_json root { authRequest(params, "/0/private/Balance")};
    json_t *result = json_object_get(root.get(),"result");
    const char * avail_str = json_string_value(json_object_get(result, currency.c_str()));
    available = avail_str ? atof(avail_str) : 0.0;
  } 
  return available;
}

std::string sendLongOrder(Parameters &params, std::string direction, double quantity, double price)
{
  return sendOrder(params, direction, quantity, price);
}

std::string sendOrder(Parameters &params, std::string direction, double quantity, double price)
{
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0)
  {
    *params.logFile << "<Kraken> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
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
  std::string options = "pair=" + pair + "&type=" + type + "&ordertype=" + ordertype + "&price=" + pricelimit + "&volume=" + volume + "&trading_agreement=agree";
  unique_json root{authRequest(params, "/0/private/AddOrder", options)};
  json_t *res = json_object_get(root.get(), "result");
  if (json_is_object(res) == 0)
  {
    *params.logFile << json_dumps(root.get(), 0) << std::endl;
    exit(0);
  }
  std::string txid = json_string_value(json_array_get(json_object_get(res, "txid"), 0));
  *params.logFile << "<Kraken> Done (transaction ID: " << txid << ")\n"
                  << std::endl;
  return txid;
}

std::string sendShortOrder(Parameters &params, std::string direction, double quantity, double price)
{
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0)
  {
    *params.logFile << "<Kraken> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
    return "0";
  }
  *params.logFile << "<Kraken> Trying to send a short \"" << direction << "\" limit order: "
                  << std::setprecision(6) << quantity << " @ $"
                  << std::setprecision(2) << price << "...\n";
  std::string pair = "XXBTZUSD";
  std::string type = direction;
  std::string ordertype;
  std::string options;
  std::string pricelimit = std::to_string(price);
  std::string volume = std::to_string(quantity);
  std::string leverage = "2";
  ordertype = "limit";
  options = "pair=" + pair + "&type=" + type + "&ordertype=" + ordertype + "&price=" + pricelimit + "&volume=" + volume + "&leverage=" + leverage + "&trading_agreement=agree";
  unique_json root{authRequest(params, "/0/private/AddOrder", options)};
  json_t *res = json_object_get(root.get(), "result");
  if (json_is_object(res) == 0)
  {
    *params.logFile << json_dumps(root.get(), 0) << std::endl;
    exit(0);
  }
  std::string txid = json_string_value(json_array_get(json_object_get(res, "txid"), 0));
  *params.logFile << "<Kraken> Done (transaction ID: " << txid << ")\n"
                  << std::endl;
  return txid;
}

bool isOrderComplete(Parameters &params, std::string orderId)
{
  unique_json root{authRequest(params, "/0/private/OpenOrders")};
  // no open order: return true
  auto res = json_object_get(json_object_get(root.get(), "result"), "open");
  if (json_object_size(res) == 0)
  {
    *params.logFile << "<Kraken> No order exists" << std::endl;
    return true;
  }
  res = json_object_get(res, orderId.c_str());
  // open orders exist but specific order not found: return true
  if (json_object_size(res) == 0)
  {
    *params.logFile << "<Kraken> Order " << orderId << " does not exist" << std::endl;
    return true;
    // open orders exist and specific order was found: return false
  }
  else
  {
    *params.logFile << "<Kraken> Order " << orderId << " still exists!" << std::endl;
    return false;
  }
}

double getActivePos(Parameters &params, std::string orderId)
{
  double activeSize = 0.0;
  if (!orderId.empty()){
    std::string uri = "/0/private/QueryOrders";
    std::string options = "txid=";
    options += orderId;
    unique_json root { authRequest(params, "/0/private/QueryOrders", options) };
    auto result = json_object_get(root.get(),"result");
    //SUGGEST: Pretty messy dude
    std::string tmpord = orderId.c_str();
    activeSize = atof(json_string_value(json_object_get(json_object_get(result,tmpord.c_str()),"vol")));
  }
  return activeSize;
}

double getLimitPrice(Parameters &params, double volume, bool isBid)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/0/public/Depth?pair=XXBTZUSD") };
  auto branch = json_object_get(json_object_get(root.get(), "result"), "XXBTZUSD");
  branch = json_object_get(branch, isBid ? "bids" : "asks");

  // loop on volume
  double totVol = 0.0;
  double currPrice = 0;
  double currVol = 0;
  unsigned int i;
  // [[<price>, <volume>, <timestamp>], [<price>, <volume>, <timestamp>], ...]
  for (i = 0; i < json_array_size(branch); i++)
  {
    // volumes are added up until the requested volume is reached
    currVol = atof(json_string_value(json_array_get(json_array_get(branch, i), 1)));
    currPrice = atof(json_string_value(json_array_get(json_array_get(branch, i), 0)));
    totVol += currVol;
    if (totVol >= volume * params.orderBookFactor)
      break;
  }

  return currPrice;
}

json_t *authRequest(Parameters &params, std::string request, std::string options)
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
         &sig_data[sig_size - SHA256_DIGEST_LENGTH]);

  std::string decoded_key = base64_decode(params.krakenSecret);
  uint8_t *hmac_digest = HMAC(EVP_sha512(),
                              decoded_key.c_str(), decoded_key.length(),
                              sig_data.data(), sig_data.size(), NULL, NULL);
  std::string api_sign_header = base64_encode(hmac_digest, SHA512_DIGEST_LENGTH);
  // cURL header
  std::array<std::string, 2> headers{
      "API-KEY:" + params.krakenApi,
      "API-Sign:" + api_sign_header,
  };

  // cURL request
  auto &exchange = queryHandle(params);
  return exchange.postRequest(request,
                              make_slist(std::begin(headers), std::end(headers)),
                              post_data);
}

std::string symbolTransform(Parameters& params, std::string currency){
  std::transform(currency.begin(),currency.end(), currency.begin(), ::toupper);
  if (currency.compare("BTC")==0){
    return "XXBT";
  
  } else if (currency.compare("USD")==0){
    return "ZUSD";

  } else if (currency.compare("ETH")==0){
    return "XETH";

  } else { 
    std::cout << "<Kraken> WARNING: Currency not supported." << std::endl;
    return "";
  }
}

void testKraken()
{

  Parameters params("bird.conf");
  params.logFile = new std::ofstream("./test.log", std::ofstream::trunc);

  std::string orderId;

  std::cout << "Current value LEG1_LEG2 bid: " << getQuote(params).bid() << std::endl;
  std::cout << "Current value LEG1_LEG2 ask: " << getQuote(params).ask() << std::endl;
  std::cout << "Current balance BTC: " << getAvail(params, "btc") << std::endl;
  std::cout << "Current balance USD: " << getAvail(params, "usd") << std::endl;
  std::cout << "Current balance ETH: " << getAvail(params, "eth") << std::endl;
  std::cout << "Current balance XMR: " << getAvail(params, "xmr") << std::endl;
  std::cout << "current bid limit price for .09 units: " << getLimitPrice(params, 0.09, true) << std::endl;
  std::cout << "Current ask limit price for .09 units: " << getLimitPrice(params, 0.09, false) << std::endl;
  //std::cout << "Sending buy order for 0.01 XMR @ $100 USD - TXID: " << std::endl;
  //orderId = sendLongOrder(params, "buy", 0.01, 100);
  //std::cout << orderId << std::endl;
  ///// if you don't wait bittrex won't recognize order for iscomplete
  //sleep(5);
  //std::cout << "Buy Order is complete: " << isOrderComplete(params, orderId) << std::endl;

  //std::cout << "Sending Short XMR order for 0.177 XMR @BID! USD: ";
  //orderId = sendShortOrder(params,"sell",0.133, getLimitPrice(params,0.133,true));
  //std::cout << orderId << std::endl;
  //std::cout << "Closing Short XMR order for .09 - TXID: ";
  //orderId = sendShortOrder(params, "buy", 0.046, getLimitPrice(params,0.046, false));
  //std::cout << orderId  << std::endl;

  //vanilla sell orders below
  //std::cout << "Buy order is complete: " << isOrderComplete(params, orderId) << std::endl;
  //std::cout << "Sending sell order for 0.01 XMR @ 5000 USD - TXID: " << std::endl ;
  //orderId = sendLongOrder(params, "sell", 0.01, 5000);
  //std:: cout << orderId << std::endl;
  //std::cout << "Sell order is complete: " << isOrderComplete(params, orderId) << std::endl;
  //std::cout << "Active Position: " << getActivePos(params);
}
}
