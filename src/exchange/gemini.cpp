#include <string.h>
#include <iostream>
#include <unistd.h>
#include <math.h>
#include <sstream>
#include <sys/time.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include "utils/base64.h"
#include <jansson.h>
#include "gemini.h"
#include "curl_fun.h"

namespace Gemini {

double getQuote(Parameters& params, bool isBid) {
  bool GETRequest = false;
  json_t* root = getJsonFromUrl(params, "https://api.gemini.com/v1/book/BTCUSD", "", GETRequest);
  const char *quote;
  double quoteValue;
  if (isBid) {
    quote = json_string_value(json_object_get(json_array_get(json_object_get(root, "bids"), 0), "price"));
  } else {
    quote = json_string_value(json_object_get(json_array_get(json_object_get(root, "asks"), 0), "price"));
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
  json_t* root = authRequest(params, "https://api.gemini.com/v1/balances", "balances", "");
  while (json_object_get(root, "message") != NULL) {
    sleep(1.0);
    *params.logFile << "<Gemini> Error with JSON: " << json_dumps(root, 0) << ". Retrying..." << std::endl;
    root = authRequest(params, "https://api.gemini.com/v1/balances", "balances", "");
  }
  // go through the list
  size_t arraySize = json_array_size(root);
  double availability = 0.0;
  const char* returnedText;
  std::string currencyAllCaps;
  if (currency.compare("btc") == 0) {
    currencyAllCaps = "BTC";
  } else if (currency.compare("usd") == 0) {
    currencyAllCaps = "USD";
  }
  for (size_t i = 0; i < arraySize; i++) {
    std::string tmpCurrency = json_string_value(json_object_get(json_array_get(root, i), "currency"));
    if (tmpCurrency.compare(currencyAllCaps.c_str()) == 0) {
      returnedText = json_string_value(json_object_get(json_array_get(root, i), "amount"));
      if (returnedText != NULL) {
        availability = atof(returnedText);
      } else {
       *params.logFile << "<Gemini> Error with the credentials." << std::endl;
       availability = 0.0;
      }
    }
  }
  json_decref(root);
  return availability;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  *params.logFile << "<Gemini> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$" << price << "..." << std::endl;
  std::ostringstream oss;
  oss << "\"symbol\":\"BTCUSD\", \"amount\":\"" << quantity << "\", \"price\":\"" << price << "\", \"side\":\"" << direction << "\", \"type\":\"exchange limit\"";
  std::string options = oss.str();
  json_t* root = authRequest(params, "https://api.gemini.com/v1/order/new", "order/new", options);
  std::string orderId = json_string_value(json_object_get(root, "order_id"));
  *params.logFile << "<Gemini> Done (order ID: " << orderId << ")\n" << std::endl;
  json_decref(root);
  return orderId;
}

bool isOrderComplete(Parameters& params, std::string orderId) {
  if (orderId == "0") return true;

  auto options = "\"order_id\":" + orderId;
  json_t* root = authRequest(params, "https://api.gemini.com/v1/order/status", "order/status", options);
  bool isComplete = json_is_false(json_object_get(root, "is_live"));
  json_decref(root);
  return isComplete;
}

double getActivePos(Parameters& params) {
  return getAvail(params, "btc");
}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
  bool GETRequest = false;
  json_t* root;
  if (isBid) {
    root = json_object_get(getJsonFromUrl(params, "https://api.gemini.com/v1/book/btcusd", "", GETRequest), "bids");
  } else {
    root = json_object_get(getJsonFromUrl(params, "https://api.gemini.com/v1/book/btcusd", "", GETRequest), "asks");
  }
  // loop on volume
  *params.logFile << "<Gemini> Looking for a limit price to fill " << fabs(volume) << " BTC..." << std::endl;
  double tmpVol = 0.0;
  double p;
  double v;
  int i = 0;
  while (tmpVol < fabs(volume) * params.orderBookFactor) {
    p = atof(json_string_value(json_object_get(json_array_get(root, i), "price")));
    v = atof(json_string_value(json_object_get(json_array_get(root, i), "amount")));
    *params.logFile << "<Gemini> order book: " << v << "@$" << p << std::endl;
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
  unsigned char* digest;
  digest = HMAC(EVP_sha384(), params.geminiSecret.c_str(), strlen(params.geminiSecret.c_str()), (unsigned char*)tmpPayload.c_str(), strlen(tmpPayload.c_str()), NULL, NULL);
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
    while (resCurl != CURLE_OK) {
      *params.logFile << "<Gemini> Error with cURL. Retry in 2 sec..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);
    while (!root) {
      *params.logFile << "<Gemini> Error with JSON:\n" << error.text << std::endl;
      *params.logFile << "<Gemini> Buffer:\n" << readBuffer.c_str() << std::endl;
      *params.logFile << "<Gemini> Retrying..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
      while (resCurl != CURLE_OK) {
        *params.logFile << "<Gemini> Error with cURL. Retry in 2 sec..." << std::endl;
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
    *params.logFile << "<Gemini> Error with cURL init." << std::endl;
    return NULL;
  }
}

}

