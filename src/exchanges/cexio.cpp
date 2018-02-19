#include "cexio.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "utils/base64.h"
#include "unique_json.hpp"
#include "hex_str.hpp"

#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <vector>
#include <iomanip>
#include <array>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace Cexio {
static json_t* authRequest(Parameters &, std::string, std::string);

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://cex.io/api",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

static json_t* checkResponse(std::ostream &logFile, json_t *root)
{
  auto errstatus = json_object_get(root, "error");

  if (errstatus)
  {
    auto errmsg = json_dumps(errstatus, JSON_ENCODE_ANY);
    logFile << "<Cexio> Error with response: "
            << errmsg << '\n';
    free(errmsg);
  }

  return root;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params); 
  unique_json root { exchange.getRequest("/ticker/BTC/USD") };

  double bidValue = json_number_value(json_object_get(root.get(), "bid"));
  double askValue = json_number_value(json_object_get(root.get(), "ask"));

  return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters& params, std::string currency)
{
  double available = 0.0;
  currency = symbolTransform(params, currency);
  const char * curr_ = currency.c_str();

  unique_json root { authRequest(params, "/balance/","") };

  const char * avail_str = json_string_value(json_object_get(json_object_get(root.get(), curr_), "available"));
  available = avail_str ? atof(avail_str) : 0.0;

  return available;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price)
{
  return sendOrder(params, direction, quantity, price);
}

std::string sendShortOrder(Parameters& params, std::string direction, double quantity, double price)
{
  return sendOrder(params, direction, quantity, price);
}

std::string sendOrder(Parameters& params, std::string direction, double quantity, double price)
{
  using namespace std;
  string pair = "btc_usd";
  string orderId = "";
  *params.logFile << "<Cexio> Trying to send a " << pair << " " << direction << " limit order: " << quantity << "@" << price << endl;

  ostringstream oss;
  oss << "type=" << direction << "&amount=" << quantity << "&price=" << fixed << setprecision(2) << price;
  string options = oss.str();

  unique_json root { authRequest(params, "/place_order/BTC/USD/", options) };
  auto error = json_string_value(json_object_get(root.get(), "error"));
  if (error){
    // auto dump = json_dumps(root.get(), 0);
    // *params.logFile << "<Cexio> Error placing order: " << dump << ")\n" << endl;
    // free(dump);
    orderId = "0";
  } else {
    orderId = json_string_value(json_object_get(root.get(), "id"));
    *params.logFile << "<Cexio> Done (order ID): " << orderId << ")\n" << endl;
  }
  return orderId;
}

bool isOrderComplete(Parameters& params, std::string orderId)
{
  using namespace std;

  bool tmp = true;
  unique_json root { authRequest(params, "/open_orders/","") };
  size_t arraySize = json_array_size(root.get());
  for (size_t i = 0; i < arraySize; ++i){
    string tmpid = json_string_value(json_object_get(json_array_get(root.get(),i),"id"));
    if (tmpid.compare(orderId.c_str())==0){
      *params.logFile << "<Cexio> Order Still open (ID: " << orderId << " )" << " Remaining: " 
      << json_string_value(json_object_get(json_array_get(root.get(),i),"pending"))
      << endl;
      return tmp = false;
    }
  }
  if (tmp == true){ *params.logFile << "<Cexio> Order Complete (ID: " << orderId << " )" << endl;}
  return tmp;
}

double getActivePos(Parameters& params, std::string orderId) { 
  std::string options = "id="+ orderId;
  unique_json root { authRequest(params, "/get_order/", options) };
  double res = atof(json_string_value(json_object_get(root.get(),"amount")));
  
  return res; }

double getLimitPrice(Parameters &params, double volume, bool isBid)
{
  auto &exchange = queryHandle(params);
  auto root = unique_json(exchange.getRequest("/order_book/BTC/USD/"));
  auto branch = json_object_get(root.get(), isBid ? "bids" : "asks");

  // loop on volume
  double totVol = 0.0;
  double currPrice = 0.0;
  double currVol = 0.0;
  unsigned int i = 0;
  // // [[<price>, <volume>], [<price>, <volume>], ...]
  for(i = 0; i < (json_array_size(branch)); i++)
  {
    // volumes are added up until the requested volume is reached
    currVol = json_number_value(json_array_get(json_array_get(branch, i), 1));
    currPrice = json_number_value(json_array_get(json_array_get(branch, i), 0));
    totVol += currVol;
    if(totVol >= volume * params.orderBookFactor){
        break;
    }
  }

  return currPrice;
}

json_t* authRequest(Parameters &params, std::string request, std::string options)
{
  using namespace std;
  static uint64_t nonce = time(nullptr) * 4;
  auto msg = to_string(++nonce) + params.cexioClientId + params.cexioApi;
  uint8_t *digest = HMAC (EVP_sha256(),
                          params.cexioSecret.c_str(), params.cexioSecret.size(),
                          reinterpret_cast<const uint8_t *>(msg.data()), msg.size(),
                          nullptr, nullptr);

  string postParams = "key=" + params.cexioApi +
                           "&signature=" + hex_str<upperhex>(digest, digest + SHA256_DIGEST_LENGTH) +
                           "&nonce=" + to_string(nonce);
  
  if (!options.empty())
  {
    postParams += "&";
    postParams += options;
  }

  auto &exchange = queryHandle(params);
  return checkResponse(*params.logFile, exchange.postRequest(request, postParams));
}

std::string symbolTransform(Parameters& params, std::string leg){
  std::transform(leg.begin(),leg.end(), leg.begin(), ::toupper);
  if (leg.compare("BTC")==0){
    return "BTC";
  } else if (leg.compare("USD")==0){
    return "USD";
  } else {
    *params.logFile << "<Cexio> WARNING: Currency not supported." << std::endl;
    return "";
  }
}
void testCexio() {
  using namespace std;
  Parameters params("test.conf");
  params.logFile = new ofstream("./test.log" , ofstream::trunc);

  string orderId;

  cout << "Current value BTC_USD bid: " << getQuote(params).bid() << endl;
  cout << "Current value BTC_USD ask: " << getQuote(params).ask() << endl;
  cout << "Current balance BTC: " << getAvail(params, "BTC") << endl;
  cout << "Current balance BCH: " << getAvail(params, "BCH") << endl;
  cout << "Current balance ETH: " << getAvail(params, "ETH") << endl;
  cout << "Current balance LTC: " << getAvail(params, "LTC") << endl;
  cout << "Current balance DASH: " << getAvail(params, "DASH") << endl;
  cout << "Current balance ZEC: " << getAvail(params, "ZEC") << endl;
  cout << "Current balance USD: " << getAvail(params, "USD") << endl;
  cout << "Current balance EUR: " << getAvail(params, "EUR")<< endl;
  cout << "Current balance GBP: " << getAvail(params, "GBP") << endl;
  cout << "Current balance RUB: " << getAvail(params, "RUB") << endl;
  cout << "Current balance GHS: " << getAvail(params, "GHS") << endl;
  cout << "Current bid limit price for 10 units: " << getLimitPrice(params, 10.0, true) << endl;
  cout << "Current ask limit price for 10 units: " << getLimitPrice(params, 10.0, false) << endl;

  // cout << "Sending buy order - TXID: " ;
   orderId = "5689142503";
   cout << orderId << endl;
   cout << "Buy order is complete: " << isOrderComplete(params, orderId) << endl;
  // cout << "Active positions (BTC): " << getActivePos(params) << endl;

  // cout << "Sending sell order - TXID: " ;
  // orderId = sendLongOrder(params, "sell", 0.002, 8800);
  // cout << orderId << endl;
  // cout << "Sell order is complete: " << isOrderComplete(params, orderId) << endl;

  // cout << "Active positions (BTC): " << getActivePos(params) << endl;

  // cout << "Sending short order - TXID: " ;
  // orderId = sendShortOrder(params, "sell", 0.002, 8800);
  // cout << orderId << endl;
  // cout << "Sell order is complete: " << isOrderComplete(params, orderId) << endl;

  // cout << "Active positions: " << getActivePos(params) << endl;

  /******* Cancel order functionality - for testing *******/
  // long oId = 0; //Add orderId here
  // ostringstream oss;
  // oss << "id=" << oId;
  // string options = oss.str();

  // unique_json root { authRequest(params, "/cancel_order/", options) };
  // auto dump = json_dumps(root.get(), 0);
  // *params.logFile << "<Cexio> Error canceling order: " << dump << ")\n" << endl;
  // free(dump);
  /*********************************************************/
  }

}
