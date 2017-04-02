#include "bitfinex.h"
#include "parameters.h"
#include "utils/base64.h"
#include "curl_fun.h"
#include "hex_str.hpp"

#include "jansson.h"
#include "openssl/sha.h"
#include "openssl/hmac.h"

#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <math.h>
#include <sys/time.h>

namespace Bitfinex {

double getQuote(Parameters& params, bool isBid) {
  bool GETRequest = true;
  json_t* root = getJsonFromUrl(params, "https://api.bitfinex.com/v1/ticker/btcusd", "", GETRequest);
  const char* quote;
  double quoteValue;
  if (isBid) {
    quote = json_string_value(json_object_get(root, "bid"));
  } else {
    quote = json_string_value(json_object_get(root, "ask"));
  }
  if (quote != NULL) {
    quoteValue = atof(quote);
  } else {
    quoteValue = 0.0;
  }
  json_decref(root);
  return quoteValue;
}

double getAvail(Parameters& params, std::string currency) {
  json_t* root = authRequest(params, "https://api.bitfinex.com/v1/balances", "balances", "");
  while (json_object_get(root, "message") != NULL) {
    sleep(1.0);
    *params.logFile << "<Bitfinex> Error with JSON: " << json_dumps(root, 0) << ". Retrying..." << std::endl;
    root = authRequest(params, "https://api.bitfinex.com/v1/balances", "balances", "");
  }
  size_t arraySize = json_array_size(root);
  double availability = 0.0;
  const char* returnedText;
  for (size_t i = 0; i < arraySize; i++) {
    std::string tmpType = json_string_value(json_object_get(json_array_get(root, i), "type"));
    std::string tmpCurrency = json_string_value(json_object_get(json_array_get(root, i), "currency"));
    if (tmpType.compare("trading") == 0 && tmpCurrency.compare(currency.c_str()) == 0) {
      returnedText = json_string_value(json_object_get(json_array_get(root, i), "amount"));
      if (returnedText != NULL) {
        availability = atof(returnedText);
      } else {
        *params.logFile << "<Bitfinex> Error with the credentials." << std::endl;
        availability = 0.0;
      }
    }
  }
  json_decref(root);
  return availability;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  return sendOrder(params, direction, quantity, price);
}

std::string sendShortOrder(Parameters& params, std::string direction, double quantity, double price) {
  return sendOrder(params, direction, quantity, price);
}

std::string sendOrder(Parameters& params, std::string direction, double quantity, double price) {
  *params.logFile << "<Bitfinex> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$" << price << "..." << std::endl;
  std::ostringstream oss;
  oss << "\"symbol\":\"btcusd\", \"amount\":\"" << quantity << "\", \"price\":\"" << price << "\", \"exchange\":\"bitfinex\", \"side\":\"" << direction << "\", \"type\":\"limit\"";
  std::string options = oss.str();
  json_t* root = authRequest(params, "https://api.bitfinex.com/v1/order/new", "order/new", options);
  auto orderId = std::to_string(json_integer_value(json_object_get(root, "order_id")));
  *params.logFile << "<Bitfinex> Done (order ID: " << orderId << ")\n" << std::endl;
  json_decref(root);
  return orderId;
}

bool isOrderComplete(Parameters& params, std::string orderId) {
  if (orderId == "0") return true;

  auto options =  "\"order_id\":" + orderId;
  json_t* root = authRequest(params, "https://api.bitfinex.com/v1/order/status", "order/status", options);
  bool isComplete = json_is_false(json_object_get(root, "is_live"));
  json_decref(root);
  return isComplete;
}

double getActivePos(Parameters& params) {
  json_t* root = authRequest(params, "https://api.bitfinex.com/v1/positions", "positions", "");
  double position;
  if (json_array_size(root) == 0) {
    *params.logFile << "<Bitfinex> WARNING: BTC position not available, return 0.0" << std::endl;
    position = 0.0;
  } else {
    position = atof(json_string_value(json_object_get(json_array_get(root, 0), "amount")));
  }
  json_decref(root);
  return position;
}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
  json_t* root;
  bool GETRequest = true;
  if (isBid) {
    root = json_object_get(getJsonFromUrl(params, "https://api.bitfinex.com/v1/book/btcusd", "", GETRequest), "bids");
  } else {
    root = json_object_get(getJsonFromUrl(params, "https://api.bitfinex.com/v1/book/btcusd", "", GETRequest), "asks");
  }
  *params.logFile << "<Bitfinex> Looking for a limit price to fill " << fabs(volume) << " BTC..." << std::endl;
  double tmpVol = 0.0;
  double p;
  double v;
  int i = 0;
    // loop on volume
  while (tmpVol < fabs(volume) * params.orderBookFactor) {
    p = atof(json_string_value(json_object_get(json_array_get(root, i), "price")));
    v = atof(json_string_value(json_object_get(json_array_get(root, i), "amount")));
    *params.logFile << "<Bitfinex> order book: " << v << "@$" << p << std::endl;
    tmpVol += v;
    i++;
  }
  double limPrice = 0.0;
  limPrice = atof(json_string_value(json_object_get(json_array_get(root, i-1), "price")));
  json_decref(root);
  return limPrice;
}

json_t* authRequest(Parameters& params, std::string url, std::string request, std::string options) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long nonce = (tv.tv_sec * 1000.0) + (tv.tv_usec * 0.001) + 0.5;
  std::ostringstream oss;
  if (options.empty()) {
    oss << "{\"request\":\"/v1/" << request << "\",\"nonce\":\"" << nonce << "\"}";
  } else {
    oss << "{\"request\":\"/v1/" << request << "\",\"nonce\":\"" << nonce << "\", " << options << "}";
  }
  std::string tmpPayload = base64_encode(reinterpret_cast<const uint8_t *>(oss.str().c_str()), oss.str().length());
  oss.clear();
  oss.str("");
  oss << "X-BFX-PAYLOAD:" << tmpPayload;
  std::string payload;
  payload = oss.str();
  oss.clear();
  oss.str("");
  // signature
  uint8_t *digest = HMAC(EVP_sha384(), params.bitfinexSecret.c_str(), params.bitfinexSecret.length(), (uint8_t *)tmpPayload.c_str(), tmpPayload.length(), NULL, NULL);

  oss << "X-BFX-SIGNATURE:" << hex_str(digest, digest + SHA384_DIGEST_LENGTH);
  struct curl_slist *headers = NULL;
  std::string api = "X-BFX-APIKEY:" + std::string(params.bitfinexApi);
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
    json_t* root;
    json_error_t error;
    while (resCurl != CURLE_OK) {
      *params.logFile << "<Bitfinex> Error with cURL. Retry in 2 sec..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);
    while (!root) {
      *params.logFile << "<Bitfinex> Error with JSON:\n" << error.text << std::endl;
      *params.logFile << "<Bitfinex> Buffer:\n" << readBuffer.c_str() << std::endl;
      *params.logFile << "<Bitfinex> Retrying..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
      while (resCurl != CURLE_OK) {
        *params.logFile << "<Bitfinex> Error with cURL. Retry in 2 sec..." << std::endl;
        sleep(2.0);
        readBuffer = "";
        resCurl = curl_easy_perform(params.curl);
      }
      root = json_loads(readBuffer.c_str(), 0, &error);
    }
    curl_slist_free_all(headers);
    curl_easy_reset(params.curl);
    return root;
  } else {
    *params.logFile << "<Bitfinex> Error with cURL init." << std::endl;
    return NULL;
  }
}

}
