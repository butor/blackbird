#include "gdax.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "unique_json.hpp"
#include "utils/base64.h"
#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <iomanip>
#include <vector>
#include <array>
#include <ctime>
#include <cmath>

namespace GDAX
{

static RestApi &queryHandle(Parameters &params)
{
  static RestApi query("https://api.gdax.com",
                       params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{

  auto &exchange = queryHandle(params);
  std::string pair;
  pair = "/products/";
  //pair += "BTC";
  //pair += "-";
  //pair += "USD";
  pair += pairTransform(params);
  pair += "/ticker";
  unique_json root{exchange.getRequest(pair)};
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
  unique_json root{authRequest(params, "GET", "/accounts", "")};
  currency = symbolTransform(params, currency);
  size_t arraySize = json_array_size(root.get());
  double available = 0.0;
  const char *currstr;
  for (size_t i = 0; i < arraySize; i++)
  {
    std::string tmpCurrency = json_string_value(json_object_get(json_array_get(root.get(), i), "currency"));
    if (tmpCurrency.compare(currency.c_str()) == 0)
    {
      currstr = json_string_value(json_object_get(json_array_get(root.get(), i), "available"));
      if (currstr != NULL)
      {
        available = atof(currstr);
        break;
      }
      else
      {
        *params.logFile << "<GDAX> Error with currency string" << std::endl;
        available = 0.0;
      }
    }
  }
  return available;
}

double getActivePos(Parameters &params, std::string orderId)
{
  double activeSize = 0.0;
  if (!orderId.empty())
  {
    std::string uri = "/orders/";
    uri += orderId.c_str();
    unique_json root{authRequest(params, "GET", uri, "")};
    activeSize = atof(json_string_value(json_object_get(root.get(), "size")));
  }
  return activeSize;
}

double getLimitPrice(Parameters &params, double volume, bool isBid)
{
  auto &exchange = queryHandle(params);
  // TODO: Build a real URL with leg1 leg2 and auth post it
  // FIXME: using level 2 order book - has aggregated data but should be sufficient for now.
  unique_json root{exchange.getRequest("/products/BTC-USD/book?level=2")};
  auto bidask = json_object_get(root.get(), isBid ? "bids" : "asks");
  *params.logFile << "<GDAX> Looking for a limit price to fill "
                  << std::setprecision(8) << fabs(volume) << " Legx...\n";
  double tmpVol = 0.0;
  double p = 0.0;
  double v;
  int i = 0;
  while (tmpVol < fabs(volume) * params.orderBookFactor)
  {
    p = atof(json_string_value(json_array_get(json_array_get(bidask, i), 0)));
    v = atof(json_string_value(json_array_get(json_array_get(bidask, i), 1)));
    *params.logFile << "<GDAX> order book: "
                    << std::setprecision(8) << v << " @$"
                    << std::setprecision(8) << p << std::endl;
    tmpVol += v;
    i++;
  }
  return p;
}

std::string sendLongOrder(Parameters &params, std::string direction, double quantity, double price)
{
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0)
  {
    *params.logFile << "<GDAX> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
    return "0";
  }
  *params.logFile << "<GDAX> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(8) << quantity << " @ $"
                  << std::setprecision(8) << price << "...\n";
  std::string pair = "BTC-USD";
  std::string type = direction;
  char buff[300];
  snprintf(buff, 300, "{\"size\":\"%.8f\",\"price\":\"%.8f\",\"side\":\"%s\",\"product_id\": \"%s\"}", quantity, price, type.c_str(), pair.c_str());
  unique_json root{authRequest(params, "POST", "/orders", buff)};
  auto txid = json_string_value(json_object_get(root.get(), "id"));

  *params.logFile << "<GDAX> Done (transaction ID: " << txid << ")\n"
                  << std::endl;
  return txid;
}

bool isOrderComplete(Parameters &params, std::string orderId)
{

  unique_json root{authRequest(params, "GET", "/orders", "")};
  size_t arraySize = json_array_size(root.get());
  bool complete = true;
  const char *idstr;
  for (size_t i = 0; i < arraySize; i++)
  {
    std::string tmpId = json_string_value(json_object_get(json_array_get(root.get(), i), "id"));
    if (tmpId.compare(orderId.c_str()) == 0)
    {
      idstr = json_string_value(json_object_get(json_array_get(root.get(), i), "status"));
      *params.logFile << "<GDAX> Order still open (Status:" << idstr << ")" << std::endl;
      complete = false;
    }
  }
  return complete;
}

json_t *authRequest(Parameters &params, std::string method, std::string request, const std::string &options)
{
  // create timestamp

  //static uint64_t nonce = time(nullptr);
  std::string nonce = gettime();
  // create data string

  std::string post_data = nonce + method + request + options;

  //if (!options.empty())
  //  post_data += options;

  // create decoded key

  std::string decoded_key = base64_decode(params.gdaxSecret);

  // Message Signature using HMAC-SHA256 of (NONCE+ METHOD??? + PATH + body)

  uint8_t *hmac_digest = HMAC(EVP_sha256(),
                              decoded_key.c_str(), decoded_key.length(),
                              reinterpret_cast<const uint8_t *>(post_data.data()), post_data.size(), NULL, NULL);

  // encode the HMAC to base64
  std::string api_sign_header = base64_encode(hmac_digest, SHA256_DIGEST_LENGTH);

  // cURL header

  std::array<std::string, 5> headers{
      "CB-ACCESS-KEY:" + params.gdaxApi,
      "CB-ACCESS-SIGN:" + api_sign_header,
      "CB-ACCESS-TIMESTAMP:" + nonce,
      "CB-ACCESS-PASSPHRASE:" + params.gdaxPhrase,
      "Content-Type: application/json; charset=utf-8",
  };

  // cURL request
  auto &exchange = queryHandle(params);

  //TODO: this sucks please do something better
  if (method.compare("GET") == 0)
  {
    return exchange.getRequest(request, make_slist(std::begin(headers), std::end(headers)));
  }
  else if (method.compare("POST") == 0)
  {
    return exchange.postRequest(request, make_slist(std::begin(headers), std::end(headers)), options);
  }
  else
  {
    std::cout << "Error With Auth method. Exiting with code 0" << std::endl;
    exit(0);
  }
}
std::string gettime()
{

  timeval curTime;
  gettimeofday(&curTime, NULL);
  int milli = curTime.tv_usec / 1000;
  char buffer[80];
  strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S", gmtime(&curTime.tv_sec));
  char time_buffer2[200];
  snprintf(time_buffer2, 200, "%s.%d000Z", buffer, milli);
  snprintf(time_buffer2, 200, "%sZ", buffer);

  //return time_buffer2;

  time_t result = time(NULL);
  long long result2 = result;
  char buff4[40];
  snprintf(buff4, 40, "%lld", result2);

  char buff5[40];
  snprintf(buff5, 40, "%lld.%d000", result2, milli);
  return buff5;
}

std::string pairTransform(Parameters& params){
  std::string leg1 = params.leg1.c_str();
  std::string leg2 = params.leg2.c_str();
  std::transform(leg1.begin(), leg1.end(), leg1.begin(), ::toupper);
  std::transform(leg2.begin(),leg2.end(), leg2.begin(), ::toupper);
  std::string marketurl = leg1 + "-" +leg2;
  return marketurl;
}
std::string symbolTransform(Parameters& params, std::string leg){
  std::transform(leg.begin(),leg.end(), leg.begin(), ::toupper);
  if (leg.compare("BTC")==0){
    return "BTC";
  } else if (leg.compare("USD")==0 || leg.compare("USDT")==0){
    return "USD"; //WARNING: hard transform usd-> USDT not appropriate for all users
  } else {
    *params.logFile << "<GDAX> WARNING: Currency not supported." << std::endl;
    return "";
  }
}


void testGDAX()
{

  Parameters params("bird.conf");
  params.logFile = new std::ofstream("./test.log", std::ofstream::trunc);

  std::string orderId;

  //std::cout << "Current value LEG1_LEG2 bid: " << getQuote(params).bid() << std::endl;
  //std::cout << "Current value LEG1_LEG2 ask: " << getQuote(params).ask() << std::endl;
  //std::cout << "Current balance BTC: " << getAvail(params, "BTC") << std::endl;
  //std::cout << "Current balance USD: " << getAvail(params, "USD")<< std::endl;
  //std::cout << "Current balance ETH: " << getAvail(params, "ETH")<< std::endl;
  //std::cout << "Current balance BTC: " << getAvail(params, "BCH")<< std::endl;
  //std::cout << "current bid limit price for 10 units: " << getLimitPrice(params, 10 , true) << std::endl;
  //std::cout << "Current ask limit price for .09 units: " << getLimitPrice(params, 0.09, false) << std::endl;
  //std::cout << "Sending buy order for 0.005 BTC @ ASK! USD - TXID: " << std::endl;
  //orderId = sendLongOrder(params, "buy", 0.005, getLimitPrice(params,.005,false));
  //std::cout << orderId << std::endl;
  //std::cout << "Buy Order is complete: " << isOrderComplete(params, orderId) << std::endl;

  //std::cout << "Sending sell order for 0.02 BTC @ 10000 USD - TXID: " << std::endl;
  //orderId = sendLongOrder(params, "sell", 0.02, 10000);
  //std::cout << orderId << std::endl;
  //std::cout << "Sell order is complete: " << isOrderComplete(params, orderId) << std::endl;
  //std::cout << "Active Position: " << getActivePos(params);
}
}
