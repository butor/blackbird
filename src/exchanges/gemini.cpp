#include "gemini.h"
#include "parameters.h"
#include "curl_fun.h"
#include "utils/restapi.h"
#include "utils/base64.h"
#include "unique_json.hpp"

#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace Gemini {

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://api.gemini.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params);
  std::string url;
  url = "/v1/book/";
  url += params.leg1 + params.leg2;
  
  unique_json root { exchange.getRequest(url) };
  const char *quote = json_string_value(json_object_get(json_array_get(json_object_get(root.get(), "bids"), 0), "price"));
  auto bidValue = quote ? std::stod(quote) : 0.0;

  quote = json_string_value(json_object_get(json_array_get(json_object_get(root.get(), "asks"), 0), "price"));
  auto askValue = quote ? std::stod(quote) : 0.0;

  return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters& params, std::string currency) {
  unique_json root { authRequest(params, "https://api.gemini.com/v1/balances", "balances", "") };
  while (json_object_get(root.get(), "message") != NULL) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto dump = json_dumps(root.get(), 0);
    *params.logFile << "<Gemini> Error with JSON: " << dump << ". Retrying..." << std::endl;
    free(dump);
    root.reset(authRequest(params, "https://api.gemini.com/v1/balances", "balances", ""));
  }
  // go through the list
  size_t arraySize = json_array_size(root.get());
  double availability = 0.0;
  const char* returnedText;
  std::string currencyAllCaps;
  if (currency.compare("btc") == 0) {
    currencyAllCaps = "BTC";
  } else if (currency.compare("usd") == 0) {
    currencyAllCaps = "USD";
  }
  for (size_t i = 0; i < arraySize; i++) {
    std::string tmpCurrency = json_string_value(json_object_get(json_array_get(root.get(), i), "currency"));
    if (tmpCurrency.compare(currencyAllCaps.c_str()) == 0) {
      returnedText = json_string_value(json_object_get(json_array_get(root.get(), i), "amount"));
      if (returnedText != NULL) {
        availability = atof(returnedText);
      } else {
       *params.logFile << "<Gemini> Error with the credentials." << std::endl;
       availability = 0.0;
      }
    }
  }

  return availability;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  *params.logFile << "<Gemini> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(6) << quantity << "@$"
                  << std::setprecision(2) << price << "...\n";
  std::ostringstream oss;
  oss << "\"symbol\":\"BTCUSD\", \"amount\":\"" << quantity << "\", \"price\":\"" << price << "\", \"side\":\"" << direction << "\", \"type\":\"exchange limit\"";
  std::string options = oss.str();
  unique_json root { authRequest(params, "https://api.gemini.com/v1/order/new", "order/new", options) };
  std::string orderId = json_string_value(json_object_get(root.get(), "order_id"));
  *params.logFile << "<Gemini> Done (order ID: " << orderId << ")\n" << std::endl;
  return orderId;
}

bool isOrderComplete(Parameters& params, std::string orderId) {
  if (orderId == "0") return true;

  auto options = "\"order_id\":" + orderId;
  unique_json root { authRequest(params, "https://api.gemini.com/v1/order/status", "order/status", options) };
  return json_is_false(json_object_get(root.get(), "is_live"));
}

double getActivePos(Parameters& params) {
  return getAvail(params, "btc");
}

double getLimitPrice(Parameters& params, double volume, bool isBid)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/v1/book/btcusd") };
  auto bidask = json_object_get(root.get(), isBid ? "bids" : "asks");
  
  // loop on volume
  *params.logFile << "<Gemini> Looking for a limit price to fill "
                  << std::setprecision(6) << fabs(volume) << " BTC...\n";
  double tmpVol = 0.0;
  double p = 0.0;
  double v;
  int i = 0;
  while (tmpVol < fabs(volume) * params.orderBookFactor)
  {
    p = atof(json_string_value(json_object_get(json_array_get(bidask, i), "price")));
    v = atof(json_string_value(json_object_get(json_array_get(bidask, i), "amount")));
    *params.logFile << "<Gemini> order book: "
                    << std::setprecision(6) << v << "@$"
                    << std::setprecision(2) << p << std::endl;
    tmpVol += v;
    i++;
  }

  return p;
}

json_t* authRequest(Parameters& params, std::string url, std::string request, std::string options) {
  static uint64_t nonce = time(nullptr) * 4;
  ++nonce;
  // check if options parameter is empty
  std::ostringstream oss;
  if (options.empty()) {
    oss << "{\"request\":\"/v1/" << request << "\",\"nonce\":\"" << nonce << "\"}";
  } else {
    oss << "{\"request\":\"/v1/" << request << "\",\"nonce\":\"" << nonce << "\", " << options << "}";
  }
  std::string tmpPayload = base64_encode(reinterpret_cast<const unsigned char*>(oss.str().c_str()), oss.str().length());
  oss.clear();
  oss.str("");
  oss << "X-GEMINI-PAYLOAD:" << tmpPayload;
  std::string payload;
  payload = oss.str();
  oss.clear();
  oss.str("");
  // build the signature
  uint8_t *digest = HMAC(EVP_sha384(), params.geminiSecret.c_str(), params.geminiSecret.size(), (uint8_t*)tmpPayload.c_str(), tmpPayload.size(), NULL, NULL);
  char mdString[SHA384_DIGEST_LENGTH+100];   // FIXME +100
  for (int i = 0; i < SHA384_DIGEST_LENGTH; ++i) {
    sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
  }
  oss.clear();
  oss.str("");
  oss << "X-GEMINI-SIGNATURE:" << mdString;
  struct curl_slist *headers = NULL;
  std::string api = "X-GEMINI-APIKEY:" + std::string(params.geminiApi);
  headers = curl_slist_append(headers, api.c_str());
  headers = curl_slist_append(headers, payload.c_str());
  headers = curl_slist_append(headers, oss.str().c_str());
  CURLcode resCurl;
  if (params.curl) {
    std::string readBuffer;
    curl_easy_setopt(params.curl, CURLOPT_POST, 1L);
    curl_easy_setopt(params.curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(params.curl, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(params.curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(params.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(params.curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(params.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(params.curl, CURLOPT_CONNECTTIMEOUT, 10L);
    resCurl = curl_easy_perform(params.curl);
    json_t *root;
    json_error_t error;
    using std::this_thread::sleep_for;
    using secs = std::chrono::seconds;
    while (resCurl != CURLE_OK) {
      *params.logFile << "<Gemini> Error with cURL. Retry in 2 sec..." << std::endl;
      sleep_for(secs(2));
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);
    while (!root) {
      *params.logFile << "<Gemini> Error with JSON:\n" << error.text << std::endl;
      *params.logFile << "<Gemini> Buffer:\n" << readBuffer.c_str() << std::endl;
      *params.logFile << "<Gemini> Retrying..." << std::endl;
      sleep_for(secs(2));
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
      while (resCurl != CURLE_OK) {
        *params.logFile << "<Gemini> Error with cURL. Retry in 2 sec..." << std::endl;
        sleep_for(secs(2));
        readBuffer = "";
        resCurl = curl_easy_perform(params.curl);
      }
      root = json_loads(readBuffer.c_str(), 0, &error);
    }
    curl_slist_free_all(headers);
    curl_easy_reset(params.curl);
    return root;
  } else {
    *params.logFile << "<Gemini> Error with cURL init." << std::endl;
    return NULL;
  }
}

}
