#include "exmo.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "utils/base64.h"
#include "unique_json.hpp"
#include "hex_str.hpp"
#include "utils/hmac_sha512.hpp"

#include <vector>
#include <iomanip>
#include <array>
#include <chrono>

namespace Exmo {
// Forward declarations
static json_t* authRequest(Parameters &, const char* URL_Request, std::string URL_Options = "");
static std::string getSignature(Parameters &, std::string);

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://api.exmo.com/v1",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params); 
  auto root = unique_json(exchange.getRequest("/order_book/?pair=BTC_USD"));

  auto quote = json_string_value(json_object_get(json_object_get(root.get(), "BTC_USD"), "bid_top"));
  auto bidValue = quote ? std::stod(quote) : 0.0;

  quote = json_string_value(json_object_get(json_object_get(root.get(), "BTC_USD"), "ask_top"));
  auto askValue = quote ? std::stod(quote) : 0.0;

  return std::make_pair(bidValue, askValue);
}


double getAvail(Parameters& params, std::string currency)
{
  double available = 0.0;
  currency = symbolTransform(params, currency);
  const char * curr_ = currency.c_str();
  
  unique_json root { authRequest(params, "/user_info") };
  const char * avail_str = json_string_value(json_object_get(json_object_get(root.get(), "balances"), curr_));
  available = avail_str ? atof(avail_str) : 0.0;
  return available;
}

// TODO multi currency support
//std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price, std::string pair) {
std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  using namespace std;
  string pair = "btc_usd"; // TODO remove when multi currency support
  *params.logFile << "<Exmo> Trying to send a " << pair << " " << direction << " limit order: " << quantity << "@" << price << endl;
  transform(pair.begin(), pair.end(), pair.begin(), ::toupper);

  string options;
  options  = "pair=" + pair;
  options += "&quantity=" + to_string(quantity);
  options += "&price=" + to_string(price);
  options += "&type=" + direction;

  unique_json root { authRequest(params, "/order_create", options) };
  string orderId = to_string(json_integer_value(json_object_get(root.get(), "order_id")));
  if (orderId == "0") {
    auto dump = json_dumps(root.get(), 0);
    *params.logFile << "<Exmo> Failed, Message: " << dump << endl;
    free(dump);
  }
  else {
    *params.logFile << "<Exmo> Done, order ID: " << orderId << endl;
  } 
  return orderId;
}


// TODO multi currency support
//bool isOrderComplete(Parameters& params, std::string orderId, std::string pair) 
bool isOrderComplete(Parameters& params, std::string orderId) {
  using namespace std;
  string pair = "btc_usd"; // TODO remove when multi currency support
  transform(pair.begin(), pair.end(), pair.begin(), ::toupper);
  
  unique_json rootOrd { authRequest(params, "/user_open_orders") };
  
  int orders  = json_array_size(json_object_get(rootOrd.get(), pair.c_str()));
  string order_id;

  for (int i=0; i<orders; i++){
    order_id = json_string_value(json_object_get(json_array_get(json_object_get(rootOrd.get(), pair.c_str()), i), "order_id"));
    if (orderId.compare(order_id) == 0)
      return false;
  }
  
  string options;
  options  = "pair=" + pair;
  options  += "&limit=1";

  unique_json rootTr { authRequest(params, "/user_trades", options) };
  order_id = to_string(json_integer_value(json_object_get(json_array_get(json_object_get(rootTr.get(), pair.c_str()), 0), "order_id")));
  if (orderId.compare(order_id) == 0) 
    return true;
  else {
    auto dump = json_dumps(rootTr.get(), 0);
    *params.logFile << "<Exmo> Failed, Server Return Message: " << dump << endl;
    free(dump);
    return false;
  }
}


double getActivePos(Parameters& params, std::string orderId) {
  return getAvail(params, "btc");
}


double getLimitPrice(Parameters &params, double volume, bool isBid)
{
  auto &exchange = queryHandle(params);
  auto root = unique_json(exchange.getRequest("/order_book?pair=BTC_USD"));
  auto branch = json_object_get(json_object_get(root.get(), "BTC_USD"), isBid ? "bid" : "ask");

  // loop on volume
  double totVol = 0.0;
  double currPrice = 0.0;
  double currVol = 0.0;
  unsigned int i = 0;
  // [[<price>, <volume>], [<price>, <volume>], ...]
  for(i = 0; i < (json_array_size(branch)); i++)
  {
    // volumes are added up until the requested volume is reached
    currVol = atof(json_string_value(json_array_get(json_array_get(branch, i), 1)));
    currPrice = atof(json_string_value(json_array_get(json_array_get(branch, i), 0)));
    totVol += currVol;
    if(totVol >= volume * params.orderBookFactor){
        break;
    }
  }

  return currPrice;
}


json_t* authRequest(Parameters &params, const char *request, std::string options) {
  using namespace std;
  static unsigned long nonce = time(nullptr);
  nonce++;

  string req = request;
  req += "?nonce=" + to_string(nonce);
  if (!options.empty())
    req += "&" + options;

  // FIXME against the API definition exmo actualy seems don't using options in signature 
  string opt = "";

  array<string, 3> headers {
    "Content-Type:application/x-www-form-urlencoded",
    "Key:"   + params.exmoApi,
    "Sign:"  + getSignature(params, opt),
  };
  
  // cURL request
  auto &exchange = queryHandle(params);
  auto ret = exchange.postRequest(req,
                                  make_slist(begin(headers), end(headers)));
  // debug
  //auto dump = json_dumps(ret, 0);
  //*params.logFile << "<Exmo> Debug, Server Return Message: " << dump << std::endl << std::endl;
  //free(dump);
  
  return ret;
}

std::string getSignature(Parameters& params, std::string msg) {
  
  HMAC_SHA512 hmac_sha512(params.exmoSecret.c_str(), msg);
  return hmac_sha512.hex_digest();
}

std::string symbolTransform(Parameters& params, std::string leg){
  std::transform(leg.begin(),leg.end(), leg.begin(), ::toupper);
  if (leg.compare("BTC")==0){
    return "BTC";
  } else if (leg.compare("USD")==0){
    return "USD";
  } else {
    *params.logFile << "<Exmo> WARNING: Currency not supported." << std::endl;
    return "";
  }
}

void testExmo() {

  using namespace std;
  Parameters params("blackbird.conf");
  //params.exmoSecret = "";
  //params.exmoClientId = "";
  //params.exmoApi = "";
  params.logFile = new ofstream("./test.log" , ofstream::trunc);

  string orderId;

  cout << "Current value BTC_USD bid: " << getQuote(params).bid() << endl;
  cout << "Current value BTC_USD ask: " << getQuote(params).ask() << endl;
  cout << "Current balance BTC: " << getAvail(params, "btc") << endl;
  cout << "Current balance USD: " << getAvail(params, "usd") << endl;
  cout << "Current balance XMR: " << getAvail(params, "xmr")<< endl;
  cout << "Current balance EUR: " << getAvail(params, "eur")<< endl;
  cout << "Current bid limit price for 10 units: " << getLimitPrice(params, 10.0, true) << endl;
  cout << "Current ask limit price for 10 units: " << getLimitPrice(params, 10.0, false) << endl;

  //cout << "Sending buy order - TXID: " ;
  //orderId = sendLongOrder(params, "buy", 0.005, 1000);
  //cout << orderId << endl;
  //cout << "Buy order is complete: " << isOrderComplete(params, orderId) << endl;

  //cout << "Sending sell order - TXID: " ;
  //orderId = sendLongOrder(params, "sell", 0.5, 338);
  //if (orderId == "0") {
  //  cout << "failed" << endl;
  //}
  //else {
  //  cout << orderId << endl;
    cout << "Sell order is complete: " << isOrderComplete(params, "404591373") << endl;
  //}
}

}
