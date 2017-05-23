#include "bitstamp.h"
#include "parameters.h"
#include "curl_fun.h"
#include "utils/restapi.h"
#include "utils/base64.h"
#include "unique_json.hpp"

#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <unistd.h>
#include <math.h>
#include <sstream>
#include <iomanip>
#include <sys/time.h>
#include <iomanip>

namespace Bitstamp {

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://www.bitstamp.net",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters& params)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/api/ticker") };

  const char *quote = json_string_value(json_object_get(root.get(), "bid"));
  auto bidValue = quote ? atof(quote) : 0.0;

  quote = json_string_value(json_object_get(root.get(), "ask"));
  auto askValue = quote ? atof(quote) : 0.0;

  return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters& params, std::string currency)
{
  unique_json root { authRequest(params, "https://www.bitstamp.net/api/balance/", "") };
  while (json_object_get(root.get(), "message") != NULL)
  {
    sleep(1.0);
    *params.logFile << "<Bitstamp> Error with JSON: " << json_dumps(root.get(), 0) << ". Retrying..." << std::endl;
    root.reset(authRequest(params, "https://www.bitstamp.net/api/balance/", ""));
  }
  double availability = 0.0;
  const char* returnedText = NULL;
  if (currency == "btc")
  {
    returnedText = json_string_value(json_object_get(root.get(), "btc_balance"));
  }
  else if (currency == "usd")
  {
    returnedText = json_string_value(json_object_get(root.get(), "usd_balance"));
  }
  if (returnedText != NULL)
  {
    availability = atof(returnedText);
  }
  else
  {
    *params.logFile << "<Bitstamp> Error with the credentials." << std::endl;
    availability = 0.0;
  }

  return availability;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price)
{
  *params.logFile << "<Bitstamp> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(6) << quantity << "@$"
                  << std::setprecision(2) << price << "...\n";
  std::ostringstream oss;
  oss << "https://www.bitstamp.net/api/" << direction << "/";
  std::string url = oss.str();
  oss.clear();
  oss.str("");
  oss << "amount=" << quantity << "&price=" << std::fixed << std::setprecision(2) << price;
  std::string options = oss.str();
  unique_json root { authRequest(params, url, options) };
  auto orderId = std::to_string(json_integer_value(json_object_get(root.get(), "id")));
  if (orderId == "0")
  {
    *params.logFile << "<Bitstamp> Order ID = 0. Message: " << json_dumps(root.get(), 0) << '\n';
  }
  *params.logFile << "<Bitstamp> Done (order ID: " << orderId << ")\n" << std::endl;

  return orderId;
}

bool isOrderComplete(Parameters& params, std::string orderId)
{
  if (orderId == "0") return true;

  auto options = "id=" + orderId;
  unique_json root { authRequest(params, "https://www.bitstamp.net/api/order_status/", options) };
  std::string status = json_string_value(json_object_get(root.get(), "status"));
  return status == "Finished";
}

double getActivePos(Parameters& params) { return getAvail(params, "btc"); }

double getLimitPrice(Parameters& params, double volume, bool isBid)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/api/order_book") };
  auto orderbook = json_object_get(root.get(), isBid ? "bids" : "asks");

  // loop on volume
  *params.logFile << "<Bitstamp> Looking for a limit price to fill "
                  << std::setprecision(6) << fabs(volume) << " BTC...\n";
  double tmpVol = 0.0;
  double p = 0.0;
  double v;
  int i = 0;
  while (tmpVol < fabs(volume) * params.orderBookFactor)
  {
    p = atof(json_string_value(json_array_get(json_array_get(orderbook, i), 0)));
    v = atof(json_string_value(json_array_get(json_array_get(orderbook, i), 1)));
    *params.logFile << "<Bitstamp> order book: "
                    << std::setprecision(6) << v << "@$"
                    << std::setprecision(2) << p << std::endl;
    tmpVol += v;
    i++;
  }
  return p;
}

json_t* authRequest(Parameters& params, std::string url, std::string options)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long nonce = (tv.tv_sec * 1000.0) + (tv.tv_usec * 0.001) + 0.5;
  std::ostringstream oss;
  oss << nonce << params.bitstampClientId << params.bitstampApi;
  unsigned char* digest;
  digest = HMAC(EVP_sha256(), params.bitstampSecret.c_str(), params.bitstampSecret.size(), (unsigned char*)oss.str().data(), oss.str().size(), NULL, NULL);
  char mdString[SHA256_DIGEST_LENGTH+100];  // FIXME +100
  for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
  {
    sprintf(&mdString[i*2], "%02X", (unsigned int)digest[i]);
  }
  oss.clear();
  oss.str("");
  oss << "key=" << params.bitstampApi << "&signature=" << mdString << "&nonce=" << nonce << "&" << options;
  std::string postParams = oss.str().c_str();

  if (params.curl)
  {
    std::string readBuffer;
    curl_easy_setopt(params.curl, CURLOPT_POST,1L);
    curl_easy_setopt(params.curl, CURLOPT_POSTFIELDS, postParams.c_str());
    curl_easy_setopt(params.curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(params.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(params.curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(params.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(params.curl, CURLOPT_CONNECTTIMEOUT, 10L);
    CURLcode resCurl = curl_easy_perform(params.curl);

    while (resCurl != CURLE_OK)
    {
      *params.logFile << "<Bitstamp> Error with cURL. Retry in 2 sec..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
    }
    json_error_t error;
    json_t *root = json_loads(readBuffer.c_str(), 0, &error);
    while (!root)
    {
      *params.logFile << "<Bitstamp> Error with JSON:\n" << error.text << '\n'
                      << "<Bitstamp> Buffer:\n" << readBuffer << '\n'
                      << "<Bitstamp> Retrying..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
      while (resCurl != CURLE_OK)
      {
        *params.logFile << "<Bitstamp> Error with cURL. Retry in 2 sec..." << std::endl;
        sleep(2.0);
        readBuffer = "";
        resCurl = curl_easy_perform(params.curl);
      }
      root = json_loads(readBuffer.c_str(), 0, &error);
    }
    curl_easy_reset(params.curl);
    return root;
  }
  else
  {
    *params.logFile << "<Bitstamp> Error with cURL init." << std::endl;
    return NULL;
  }
}

}
