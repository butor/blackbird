#include "poloniex.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "unique_json.hpp"
#include "hex_str.hpp"

#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <array>
#include <algorithm>
#include <ctime>
#include <cctype>

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
  std::string options = "account=exchange";
  if (currency.compare("USD")==0) {
    currency += "T";
  }
  unique_json root { authRequest(params, "returnAvailableAccountBalances", options) };
  auto funds = json_string_value(json_object_get(json_object_get(root.get(), "exchange"), currency.c_str()));
  return funds ? std::stod(funds) : 0.0;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0) {
    *params.logFile  << "<Poloniex> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
    return "0";
    }
  //TODO: Real currency string
  std::string options = "currencyPair=USDT_BTC&rate=";
  std::string volume = std::to_string(quantity);
  std::string pricelimit = std::to_string(price);
  options += pricelimit + "&amount=" + volume; 
  unique_json root { authRequest(params, direction.c_str(), options) };
  std::string txid = json_string_value(json_object_get(root.get(),"orderNumber"));
  return txid;
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

double getActivePos(Parameters& params, std::string orderId) {
  double activeSize = 0.0;
  if (!orderId.empty()){
    std::string options = "orderNumber=";
    options += orderId;
    unique_json root {authRequest(params, "returnOrderTrades", options) };
    size_t arraySize = json_array_size(root.get());
    double tmpFee = 0.0;
    double tmpAmount = 0.0;
    //polo returns weird things - we must loop through all the executed trades (for partial fills)
    //the actual active size is then SUM(amount*(1-tmpFee)) fee seems to be expressed as a percent
    for (size_t i = 0; i < arraySize; i++) {
      tmpFee = atof(json_string_value(json_object_get(json_array_get(root.get(),i),"fee")));
      tmpAmount = atof(json_string_value(json_object_get(json_array_get(root.get(),i),"amount")));
      activeSize += tmpAmount*(1-tmpFee);
    }
  }
  return activeSize;
}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
  auto &exchange = queryHandle(params);
  // TODO: build real curr string
  //std::string uri = "/public?command=returnOrderBook&currencyPair=";
  unique_json root { exchange.getRequest("/public?command=returnOrderBook&currencyPair=USDT_BTC") };
  auto bidask  = json_object_get(root.get(),isBid ? "bids" : "asks");
  *params.logFile << "<Poloniex> Looking for a limit price to fill "
                  << std::setprecision(8) << fabs(volume) << " Legx...\n";
  double tmpVol = 0.0;
  double p = 0.0;
  double v;
  int i = 0;
  while (tmpVol < fabs(volume) * params.orderBookFactor)
  {
    p = atof(json_string_value(json_array_get(json_array_get(bidask,i),0)));
    v = json_number_value(json_array_get(json_array_get(bidask,i),1));
    *params.logFile << "<Poloniex> order book: "
                    << std::setprecision(8) << v << " @$"
                    << std::setprecision(8) << p << std::endl;
    tmpVol += v;
    i++;
  }
  return p;
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
void testPoloniex(){

    Parameters params("bird.conf");
    params.logFile = new std::ofstream("./test.log" , std::ofstream::trunc);

    std::string orderId;

    //std::cout << "Current value LEG1_LEG2 bid: " << getQuote(params).bid() << std::endl;
    //std::cout << "Current value LEG1_LEG2 ask: " << getQuote(params).ask() << std::endl;
    //std::cout << "Current balance BTC: " << getAvail(params, "BTC") << std::endl;
    //std::cout << "Current balance USD: " << getAvail(params, "USD")<< std::endl;
    //std::cout << "Current balance ETH: " << getAvail(params, "ETH")<< std::endl;
    //std::cout << "Current balance LTC: " << getAvail(params, "LTC")<< std::endl;
    //std::cout << "current bid limit price for .04 units: " << getLimitPrice(params, 0.04, true) << std::endl;
    //std::cout << "Current ask limit price for 10 units: " << getLimitPrice(params, 10.0, false) << std::endl;
    //std::cout << "Sending buy order for 0.0011 BTC @ ASK - TXID: " << std::endl;
    //orderId = sendLongOrder(params, "buy", 0.0011, getLimitPrice(params,0.0011,false));
    //std::cout << orderId << std::endl;
    //std::cout << "Buy Order is complete: " << isOrderComplete(params, orderId) << std::endl;
    //sleep(5);
    //std::cout << "Sending sell order for 0.003603 BTC @ 10000 USD - TXID: " << std::endl;
    //orderId = sendLongOrder(params, "sell", 0.003603, 10000);
    //std::cout << orderId << std::endl;
    //std::cout << "Sell order is complete: " << isOrderComplete(params, orderId) << std::endl;

    // THE EXECUTED ORDER NUMBER:: 146631728614  146631728614
    //std::cout << "Active Position: " << getActivePos(params,"146631728614");
}
}