#include <string.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/time.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include "utils/base64.h"
#include <jansson.h>
#include "kraken.h"
#include "curl_fun.h"


// Initialise global variables
json_t *krakenTicker;
bool krakenGotTicker = false;
json_t *krakenLimPrice;
bool krakenGotLimPrice = false;

namespace Kraken {

double getQuote(Parameters& params, bool isBid) {
  bool GETRequest = false;
  json_t* root;
  if (krakenGotTicker) {
    root = krakenTicker;
    krakenGotTicker = false;
  } else {
    root = getJsonFromUrl(params, "https://api.kraken.com/0/public/Ticker", "pair=XXBTZUSD", GETRequest);
    krakenGotTicker = true;
    krakenTicker = root;
  }
  const char* quote;
  double quoteValue;
  if (isBid) {
    quote = json_string_value(json_array_get(json_object_get(json_object_get(json_object_get(root, "result"), "XXBTZUSD"), "b"), 0));
  } else {
    quote = json_string_value(json_array_get(json_object_get(json_object_get(json_object_get(root, "result"), "XXBTZUSD"), "a"), 0));
  }
  if (quote != NULL) {
    quoteValue = atof(quote);
  } else {
    quoteValue = 0.0;
  }
  if (!krakenGotTicker) {
    json_decref(root);
  }
  return quoteValue;
}

double getAvail(Parameters& params, std::string currency) {
  json_t* root = authRequest(params, "https://api.kraken.com", "/0/private/Balance");
  json_t* result = json_object_get(root, "result");
  if (json_object_size(result) == 0) {
    return 0.0;
  }
  double available = 0.0;
  if (currency.compare("usd") == 0) {
    const char * avail_str = json_string_value(json_object_get(result, "ZUSD"));
    available = avail_str ? atof(avail_str) : 0.0;
  } else if (currency.compare("btc") == 0) {
    const char * avail_str = json_string_value(json_object_get(result, "XXBT"));
    available = avail_str ? atof(avail_str) : 0.0;
  } else {
    *params.logFile << "<Kraken> Currency not supported" << std::endl;
  }
  return available;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0) {
    *params.logFile  << "<Kraken> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
    return 0;
  }
  *params.logFile << "<Kraken> Trying to send a \"" << direction << "\" limit order: " << quantity << " @ $" << price << "..." << std::endl;
  std::string pair = "XXBTZUSD";
  std::string type = direction;
  std::string ordertype = "limit";
  std::string pricelimit = std::to_string(price);
  std::string volume = std::to_string(quantity);
  std::string options = "pair=" + pair + "&type=" + type + "&ordertype=" + ordertype + "&price=" + pricelimit + "&volume=" + volume;
  json_t* res = authRequest(params, "https://api.kraken.com", "/0/private/AddOrder", options);
  json_t* root = json_object_get(res, "result");
  if (json_is_object(root) == 0) {
    *params.logFile << json_dumps(res, 0) << std::endl;
    exit(0);
  }
  std::string txid = json_string_value(json_array_get(json_object_get(root, "txid"), 0));
  *params.logFile << "<Kraken> Done (transaction ID: " << txid << ")\n" << std::endl;
  json_decref(root);
  return txid;
}

bool isOrderComplete(Parameters& params, std::string orderId) {
  json_t* root = authRequest(params, "https://api.kraken.com", "/0/private/OpenOrders");
  // no open order: return true
  root = json_object_get(json_object_get(root, "result"), "open");
  if (json_object_size(root) == 0) {
    *params.logFile << "<Kraken> No order exists" << std::endl;
    return true;
  }
  *params.logFile << json_dumps(root, 0) << std::endl;
  root = json_object_get(root, orderId.c_str());
  // open orders exist but specific order not found: return true
  if (json_object_size(root) == 0) {
    *params.logFile << "<Kraken> Order " << orderId << " does not exist" << std::endl;
    return true;
  // open orders exist and specific order was found: return false
  } else {
    *params.logFile << "<Kraken> Order " << orderId << " still exists!" << std::endl;
    return false;
  }
}

double getActivePos(Parameters& params) {
  return getAvail(params, "btc");
}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
  bool GETRequest = false;
  json_t* root;
  if (krakenGotLimPrice) {
    root = krakenLimPrice;
    krakenGotLimPrice = false;
  } else {
    root = json_object_get(json_object_get(getJsonFromUrl(params, "https://api.kraken.com/0/public/Depth", "pair=XXBTZUSD", GETRequest), "result"), "XXBTZUSD");
    krakenGotLimPrice = true;
    krakenLimPrice = root;
  }
  if (isBid) {
    root = json_object_get(root, "bids");
  } else {
    root = json_object_get(root, "asks");
  }
  // loop on volume
  double tmpVol = 0.0;
  int i = 0;
  // [[<price>, <volume>, <timestamp>], [<price>, <volume>, <timestamp>], ...]
  while (tmpVol < volume) {
    // volumes are added up until the requested volume is reached
    tmpVol += atof(json_string_value(json_array_get(json_array_get(root, i), 1)));
    i++;
  }
  double limPrice = 0.0;
  limPrice = atof(json_string_value(json_array_get(json_array_get(root, i-1), 0)));
  if (!krakenGotLimPrice) {
    json_decref(root);
  }
  return limPrice;
}

json_t* authRequest(Parameters& params, std::string url, std::string request, std::string options) {
  // create nonce and POST data
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long nonce = (tv.tv_sec * 1000.0) + (tv.tv_usec * 0.001) + 0.5;
  std::string post_data = "";
  if (options.empty()) {
    post_data = "nonce=" + std::to_string(nonce);
  } else {
    post_data = "nonce=" + std::to_string(nonce) + "&" + options;
  }
  // Message signature using HMAC-SHA512 of (URI path + SHA256(nonce + POST data))
  // and base64 decoded secret API key
  std::string payload_for_signature = std::to_string(nonce) + post_data;
  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256((unsigned char*)payload_for_signature.c_str(), strlen(payload_for_signature.c_str()), (unsigned char*)&digest);
  int signature_data_length = request.length() + SHA256_DIGEST_LENGTH;
  std::vector<unsigned char> signature_data(signature_data_length);
  auto signature_digest = std::copy(begin(request), end(request), begin(signature_data));
  std::copy(std::begin(digest), std::end(digest), signature_digest);
  std::string decoded_key = base64_decode(params.krakenSecret);
  unsigned char* hmac_digest;
  hmac_digest = HMAC(EVP_sha512(), decoded_key.c_str(), decoded_key.length(), signature_data.data(), signature_data.size(), NULL, NULL);
  std::string api_sign_header = base64_encode(reinterpret_cast<const unsigned char*>(hmac_digest), SHA512_DIGEST_LENGTH);
  // cURL header
  struct curl_slist* headers = NULL;
  std::string api = "API-KEY:" + std::string(params.krakenApi);
  std::string api_sig = "API-Sign:" + api_sign_header;
  headers = curl_slist_append(headers, api.c_str());
  headers = curl_slist_append(headers, api_sig.c_str());
  // cURL request
  CURLcode resCurl;
  if (params.curl) {
    std::string readBuffer;
    curl_easy_setopt(params.curl, CURLOPT_POST, 1L);
    curl_easy_setopt(params.curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(params.curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(params.curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(params.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(params.curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(params.curl, CURLOPT_URL, (url+request).c_str());
    curl_easy_setopt(params.curl, CURLOPT_CONNECTTIMEOUT, 10L);
    resCurl = curl_easy_perform(params.curl);
    json_t* root;
    json_error_t error;
    while (resCurl != CURLE_OK) {
      *params.logFile << "<Kraken> Error with cURL. Retry in 2 sec...\n" << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);
    while (!root) {
      *params.logFile << "<Kraken> Error with JSON:\n" << error.text << std::endl;
      *params.logFile << "<Kraken> Buffer:\n" << readBuffer.c_str() << std::endl;
      *params.logFile << "<Kraken> Retrying..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
      while (resCurl != CURLE_OK) {
        *params.logFile << "<Kraken> Error with cURL. Retry in 2 sec...\n" << std::endl;
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
    *params.logFile << "<Kraken> Error with cURL init." << std::endl;
    return NULL;
  }
  return NULL;
}

}
