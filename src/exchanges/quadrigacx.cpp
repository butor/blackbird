#include "quadrigacx.h"
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

namespace QuadrigaCX {

//forward declarations
static std::string getSignature(Parameters& params, const uint64_t nonce);
static json_t* authRequest(Parameters& params, std::string request, json_t * options = nullptr);

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://api.quadrigacx.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params); 
  auto root = unique_json(exchange.getRequest("/v2/ticker?book=btc_usd"));

  auto quote = json_string_value(json_object_get(root.get(), "bid"));
  auto bidValue = quote ? std::stod(quote) : 0.0;

  quote = json_string_value(json_object_get(root.get(), "ask"));
  auto askValue = quote ? std::stod(quote) : 0.0;

  return std::make_pair(bidValue, askValue);
}


double getAvail(Parameters& params, std::string currency)
{
  unique_json root { authRequest(params, "/v2/balance") };

  double available = 0.0;
  currency = symbolTransform(params, currency);
  currency += "_available";
  const char * avail_str = json_string_value(json_object_get(root.get(), currency.c_str()));
  available = avail_str ? atof(avail_str) : 0.0;
  return available;
}


std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price)
{
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0) {
    *params.logFile  << "<QuadrigaCX> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
    return "0";
  }
  *params.logFile << "<QuadrigaCX> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(8) << quantity << "@$"
                  << std::setprecision(2) << price << "...\n";
  
  std::ostringstream oss;
  // Quadriga don't accept amount longer that 8 digits after decimal point
  // Its a workaround, would be better to trim only digits after decimal point. 
  oss << std::fixed << std::setprecision(8) << quantity;
  std::string amount = oss.str();

  unique_json options {json_object()};
  json_object_set_new(options.get(), "book", json_string("btc_usd"));
  json_object_set_new(options.get(), "amount", json_string(amount.c_str()));
  json_object_set_new(options.get(), "price", json_real(price));

  unique_json root { authRequest(params, ("/v2/" + direction), options.get()) };
  std::string orderId = json_string_value(json_object_get(root.get(), "id"));
  if (orderId.empty()) {
    auto dump = json_dumps(root.get(), 0);
    *params.logFile << "<QuadrigaCX> Failed, Message: " << dump << std::endl;
    free(dump);
    return "0";
  }
  else {
    *params.logFile << "<QuadrigaCX> Done, order ID: " << orderId << std::endl;
    return orderId;
  } 
}


bool isOrderComplete(Parameters& params, std::string orderId) 
{
  auto ret = false;
  unique_json options {json_object()};
  json_object_set_new(options.get(), "id", json_string(orderId.c_str()));

  unique_json root { authRequest(params, "/v2/lookup_order", options.get()) };
  
  auto res = json_object_get(json_array_get(root.get(),0), "status");
  if( json_is_string(res) ){
    auto value = std::string(json_string_value(res));
    if(value.compare("2") == 0){
      ret = true;
    }
  }
  else{
    auto dump = json_dumps(root.get(), 0);
    *params.logFile << "<QuadrigaCX> Error: failed to get order status for id: " << orderId 
    << ":" << dump << std::endl;
    free(dump);
  }

  return ret;
   
}

double getActivePos(Parameters& params, std::string orderId) {
  return getAvail(params, "btc");
}


double getLimitPrice(Parameters &params, double volume, bool isBid)
{
  auto &exchange = queryHandle(params);
  auto root = unique_json(exchange.getRequest("/v2/order_book?book=btc_usd"));
  auto branch = json_object_get(root.get(), isBid ? "bids" : "asks");

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


static json_t* authRequest(Parameters& params, std::string request, json_t * options)
{
  json_int_t nonce = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  //post data is json
  unique_json payload {json_object()};
  auto root = payload.get();
  json_object_set_new(root, "key", json_string(params.quadrigaApi.c_str()));
  json_object_set_new(root, "nonce", json_integer(nonce));
  json_object_set_new(root, "signature", json_string(getSignature(params, nonce).c_str()));

  if(options != nullptr){
    const char * key;
    json_t * value;

    json_object_foreach(options, key, value){
      if(value != nullptr && key != nullptr)
        json_object_set_new(root, key, value);
    }
  }

  auto payload_string = json_dumps(root, 0);
  std::string post_data(payload_string);
  free(payload_string);

  std::string headers[1] = { 
      "Content-Type: application/json; charset=utf-8",
  };

  // cURL request
  auto &exchange = queryHandle(params);
  auto ret = exchange.postRequest(request,
                              make_slist(std::begin(headers), std::end(headers)),
                              post_data);
  return ret;
}

static std::string getSignature(Parameters& params, const uint64_t nonce)
{
  std::string sig_data_str = std::to_string(nonce) + params.quadrigaClientId + params.quadrigaApi;
  auto data_len = sig_data_str.length();
  std::vector<uint8_t> sig_data(data_len);

  copy(sig_data_str.begin(), sig_data_str.end(), sig_data.begin());

  uint8_t * hmac_digest = HMAC(EVP_sha256(),
                               params.quadrigaSecret.c_str(),
                               params.quadrigaSecret.length(),
                               sig_data.data(),
                               data_len,
                               NULL, NULL);

  return hex_str(hmac_digest, hmac_digest + SHA256_DIGEST_LENGTH);
}

std::string symbolTransform(Parameters& params, std::string leg){
  std::transform(leg.begin(),leg.end(), leg.begin(), ::tolower);
  if (leg.compare("btc")==0){
    return "btc";
  } else if (leg.compare("usd")==0){
    return "usd";
  } else if (leg.compare("cad")==0){
    return "cad";}
    else if (leg.compare("eth")==0){
    return "eth";}
  else {
    *params.logFile << "<QuadrigaCX> WARNING: Currency not supported." << std::endl;
    return "";
  }
}

void testQuadriga(){

    Parameters params("blackbird.conf");
    params.quadrigaSecret = "";
    params.quadrigaClientId = "";
    params.quadrigaApi = "";
    params.logFile = new std::ofstream("./test.log" , std::ofstream::trunc);

    std::string orderId;

    std::cout << "Current value BTC_USD bid: " << getQuote(params).bid() << std::endl;
    std::cout << "Current value BTC_USD ask: " << getQuote(params).ask() << std::endl;
    std::cout << "Current balance BTC: " << getAvail(params, "btc") << std::endl;
    std::cout << "Current balance USD: " << getAvail(params, "usd")<< std::endl;
    std::cout << "Current balance ETH: " << getAvail(params, "eth")<< std::endl;
    std::cout << "Current balance CAD: " << getAvail(params, "cad")<< std::endl;
    std::cout << "current bid limit price for 10 units: " << getLimitPrice(params, 10.0, true) << std::endl;
    std::cout << "Current ask limit price for 10 units: " << getLimitPrice(params, 10.0, false) << std::endl;

    std::cout << "Sending buy order for 0.005 BTC @ 1000 USD - TXID: " ;
    orderId = sendLongOrder(params, "buy", 0.005, 1000);
    std:: cout << orderId << std::endl;
    std::cout << "Buy order is complete: " << isOrderComplete(params, orderId) << std::endl;

    std::cout << "Sending sell order for 0.005 BTC @ 5000 USD - TXID: " ;
    orderId = sendLongOrder(params, "sell", 0.005, 5000);
    std:: cout << orderId << std::endl;
    std::cout << "Sell order is complete: " << isOrderComplete(params, orderId) << std::endl;

}

}
