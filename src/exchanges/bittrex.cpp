#include "bittrex.h"
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
#include <iomanip>
#include <sstream>

namespace Bittrex {
static json_t* authRequest(Parameters &, std::string, std::string);
static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://bittrex.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

static json_t* checkResponse(std::ostream &logFile, json_t *root)
{
  auto errmsg = json_object_get(root, "error");
  if (errmsg)
    logFile << "<Bittrex> Error with response: "
            << json_string_value(errmsg) << '\n';

  return root;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params);
  std::string x;

  x += "/api/v1.1/public/getticker?market=";
  x += "USDT-BTC";
  //params.leg2.c_str();

  unique_json root {exchange.getRequest(x)};
  
  double quote = json_number_value(json_object_get(json_object_get(root.get(), "result"), "Bid"));
  auto bidValue = quote ? quote : 0.0;

  quote = json_number_value(json_object_get(json_object_get(root.get(), "result"), "Ask"));
  auto askValue = quote ? quote : 0.0;

  return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters &params, std::string currency)
{
  currency = symbolTransform(params, currency);
  //cur_str += "currency=";
  std::string cur_str = "currency=" + currency;

  unique_json root { authRequest(params, "/api/v1.1/account/getbalance", cur_str)};
  //FIXME: theres no error checking here
  double available = json_number_value(json_object_get(json_object_get(root.get(), "result"),"Available"));
  return available;
}
// this function name is misleading it is not a "long" order but a non margin order.
std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0) {
    *params.logFile  << "<Bittrex> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
    return "0";
  }
  *params.logFile << "<Bittrex> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(8) << quantity << " @ $"
                  << std::setprecision(8) << price << "...\n";
  std::string pair = "USDT-BTC";
  std::string type = direction;
  std::string pricelimit = std::to_string(price);
  std::string volume = std::to_string(quantity);
  std::string options = "market=" + pair + "&quantity=" + volume + "&rate=" + pricelimit;
  std::string url = "/api/v1.1/market/" + direction + "limit";
  unique_json root { authRequest(params, url, options) };
  // theres some problem here that can produce a seg fault.
  auto txid = json_string_value(json_object_get(json_object_get(root.get(), "result"), "uuid"));

  *params.logFile << "<Bittrex> Done (transaction ID: " << txid << ")\n" << std::endl;
  return txid;
}
//SUGGEST: probably not necessary
std::string sendShortOrder(Parameters& params, std::string direction, double quantity, double price) {
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0) {
    *params.logFile  << "<Bittrex> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
    return "0";
  }
  *params.logFile << "<Bittrex> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(8) << quantity << " @ $"
                  << std::setprecision(8) << price << "...\n";
  std::string pair = "USDT-BTC";
  std::string pricelimit = std::to_string(price);
  std::string volume = std::to_string(quantity);
  std::string options = "market=" + pair + "&quantity=" + volume + "&rate=" + pricelimit;
  unique_json root { authRequest(params, "/api/v1.1/market/selllimit", options) };
  //theres so
  auto txid = json_string_value(json_object_get(json_object_get(root.get(), "result"), "uuid"));

  *params.logFile << "<Bittrex> Done (transaction ID: " << txid << ")\n" << std::endl;
  return txid;
}
//This is not used at the moment, but could pull out send long/short order. Leaving as is for now
std::string sendOrder(Parameters& params, std::string direction, double quantity, double price)
{
  *params.logFile << "<Bittrex> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(6) << quantity << "@$"
                  << std::setprecision(2) << price << "...\n";
  std::ostringstream oss;
  oss << "\"symbol\":\"btcusd\", \"amount\":\"" << quantity << "\", \"price\":\"" << price << "\", \"exchange\":\"bitfinex\", \"side\":\"" << direction << "\", \"type\":\"limit\"";
  std::string options = oss.str();
  unique_json root { authRequest(params, "/v1/order/new", options) };

  auto orderId = std::to_string(json_integer_value(json_object_get(root.get(), "order_id")));
  *params.logFile << "<Bittrex> Done (order ID: " << orderId << ")\n" << std::endl;
  return orderId;
}

bool isOrderComplete(Parameters& params, std::string orderId)
{
  //TODO Build a real currency string for options here (or outside?)
  unique_json root { authRequest(params, "/api/v1.1/market/getopenorders","market=USDT-BTC")};
  auto res = json_object_get(root.get(), "result");
  // loop through the array to check orders
  std::string uuid;
  bool tmp = true;
  int i = 0;
  int size = json_array_size (res);
  if (json_array_size(res) == 0) {
    *params.logFile << "<Bittrex> No orders exist" << std::endl;
    return true;
  }
  while (i < size){
    uuid = json_string_value(json_object_get(json_array_get(res,i),"OrderUuid"));
    if (uuid.compare(orderId.c_str())==0){
      *params.logFile << "<Bittrex> Order " << orderId << " still exists" << std::endl;
      tmp = false;
    }
    i++;
  }
  if (tmp == true){
    *params.logFile << "<Bittrex> Order " << orderId << " does not exist" << std::endl;
  }
  return tmp;  
}

double getActivePos(Parameters& params, std::string orderId) {
  double activeSize = 0.0;
  if (!orderId.empty()){
    std::string uri = "/api/v1.1/account/getorder";
    std::string builder = "uuid=";
    builder += orderId.c_str();
    unique_json root {authRequest(params, uri, builder) };
    activeSize = json_number_value(json_object_get(json_object_get(root.get(), "result"),"Quantity"));
  }
  return activeSize;

}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
  // takes a quantity we want and if its a bid or not
  auto &exchange  = queryHandle(params);
  //TODO build a real URI string here
  unique_json root { exchange.getRequest("/api/v1.1/public/getorderbook?market=USDT-BTC&type=both") };
  auto bidask  = json_object_get(json_object_get(root.get(),"result"),isBid ? "buy" : "sell");
    // loop on volume
  *params.logFile << "<Bittrex> Looking for a limit price to fill "
                  << std::setprecision(8) << fabs(volume) << " Legx...\n";
  double tmpVol = 0.0;
  double p = 0.0;
  double v;
  int i = 0;
  while (tmpVol < fabs(volume) * params.orderBookFactor)
  {
    p = json_number_value(json_object_get(json_array_get(bidask, i), "Rate"));
    v = json_number_value(json_object_get(json_array_get(bidask, i), "Quantity"));
    *params.logFile << "<Bittrex> order book: "
                    << std::setprecision(8) << v << "@$"
                    << std::setprecision(8) << p << std::endl;
    tmpVol += v;
    i++;
  }

  return p;
}

std::string symbolTransform(Parameters& params, std::string leg){
  std::transform(leg.begin(),leg.end(), leg.begin(), ::toupper);
  if (leg.compare("BTC")==0){
    return "BTC";
  } else if (leg.compare("USD")==0 || leg.compare("USDT")==0){
    return "USDT"; //WARNING: hard transform usd-> USDT not appropriate for all users
  } else {
    *params.logFile << "<Bittrex> WARNING: Currency not supported." << std::endl;
    return "";
  }
}

// build auth request - needs to append apikey, nonce, and calculate HMAC 512 hash and include it under api sign header
json_t* authRequest(Parameters &params, std::string request, std:: string options)
{
  //TODO: create nonce NOT respected right now so not calculating but could be added with  std::to_string(++nonce)
  //static uint64_t nonce = time(nullptr) * 4;
  // this message is the full uri for sig signing. 
  auto msg = "https://bittrex.com" + request +"?apikey=" + params.bittrexApi.c_str() +"&nonce=0";
  //append options to full uri and partial URI
  std::string uri = request + "?apikey=" + params.bittrexApi + "&nonce=0";
  if (!options.empty()){
    msg += "&";
    msg += options;
    uri += "&";
    uri += options;
  }
  // SHA512 of URI and API SECRET 
  // this function grabs the HMAC hash (using sha512) of the secret and the full URI
  uint8_t *hmac_digest = HMAC(EVP_sha512(),
                              params.bittrexSecret.c_str(), params.bittrexSecret.size(),
                              reinterpret_cast<const uint8_t *>(msg.data()),msg.size(), 
                              NULL, NULL);
  // creates a hex string of the hmac hash
  std::string api_sign_header = hex_str(hmac_digest, hmac_digest + SHA512_DIGEST_LENGTH);
  // create a base for appending the initial request domain
  std::string postParams = "?apikey=" + params.bittrexApi +
                           "&nonce=0";
  //once again append the extra options 
  if (!options.empty())
  {
    postParams += "&";
    postParams += options;
  }
  std::array<std::string,1> headers
  {
    "apisign:" + api_sign_header
  };
  //curl the request
  auto &exchange = queryHandle(params);
  return checkResponse(*params.logFile, exchange.postRequest(uri, make_slist(std::begin(headers), std::end(headers)),
                       postParams));
}



void testBittrex(){

    Parameters params("blackbird.conf");
    params.logFile = new std::ofstream("./test.log" , std::ofstream::trunc);

    std::string orderId;

    //std::cout << "Current value LEG1_LEG2 bid: " << getQuote(params).bid() << std::endl;
    //std::cout << "Current value LEG1_LEG2 ask: " << getQuote(params).ask() << std::endl;
    //std::cout << "Current balance XMR: " << getAvail(params, "XMR") << std::endl;
    //std::cout << "Current balance USD: " << getAvail(params, "USD")<< std::endl;
    //std::cout << "Current balance ETH: " << getAvail(params, "ETH")<< std::endl;
    //std::cout << "Current balance BTC: " << getAvail(params, "BTC")<< std::endl;
    //std::cout << "current bid limit price for 10 units: " << getLimitPrice(params, 10.0, true) << std::endl;
    //std::cout << "Current ask limit price for 10 units: " << getLimitPrice(params, 10.0, false) << std::endl;
    //std::cout << "Sending buy order for 0.01 XMR @ $100 USD - TXID: " << std::endl;
    //orderId = sendLongOrder(params, "buy", 0.01, 100);
    //std::cout << orderId << std::endl;
    ///// if you don't wait bittrex won't recognize order for iscomplete
    //sleep(5);
    //std::cout << "Buy Order is complete: " << isOrderComplete(params, orderId) << std::endl;
  
    // TODO: Test sell orders, really should work though.
    //std::cout << orderId << std::endl;
    //std::cout << "Buy order is complete: " << isOrderComplete(params, orderId) << std::endl;
    //std::cout << "Sending sell order for 0.01 XMR @ 5000 USD - TXID: " << std::endl ;
    //orderId = sendLongOrder(params, "sell", 0.01, 5000);
    //std:: cout << orderId << std::endl;
    //std::cout << "Sell order is complete: " << isOrderComplete(params, orderId) << std::endl;
    //std::cout << "Active Position: " << getActivePos(params, orderId);
}
}