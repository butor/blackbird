#include "okcoin.h"
#include "parameters.h"
#include "curl_fun.h"
#include "utils/restapi.h"
#include "hex_str.hpp"
#include "unique_json.hpp"

#include "openssl/md5.h"
#include <sstream>
#include <iomanip>
#include <unistd.h> // sleep
#include <math.h>   // fabs

namespace OKCoin {

static RestApi& queryHandle(Parameters &params)
{
  static RestApi query ("https://www.okcoin.com",
                        params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/api/ticker.do?ok=1") };
  const char *quote = json_string_value(json_object_get(json_object_get(root.get(), "ticker"), "buy"));
  auto bidValue = quote ? std::stod(quote) : 0.0;

  quote = json_string_value(json_object_get(json_object_get(root.get(), "ticker"), "sell"));
  auto askValue = quote ? std::stod(quote) : 0.0;

  return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters& params, std::string currency)
{
  std::ostringstream oss;
  oss << "api_key=" << params.okcoinApi << "&secret_key=" << params.okcoinSecret;
  std::string signature(oss.str());
  oss.clear();
  oss.str("");
  oss << "api_key=" << params.okcoinApi;
  std::string content(oss.str());
  unique_json root { authRequest(params, "https://www.okcoin.com/api/v1/userinfo.do", signature, content) };
  double availability = 0.0;
  const char* returnedText;
  if (currency == "usd")
  {
    returnedText = json_string_value(json_object_get(json_object_get(json_object_get(json_object_get(root.get(), "info"), "funds"), "free"), "usd"));
  }
  else if (currency == "btc")
  {
    returnedText = json_string_value(json_object_get(json_object_get(json_object_get(json_object_get(root.get(), "info"), "funds"), "free"), "btc"));
  }
  else returnedText = "0.0";

  if (returnedText != NULL) {
    availability = atof(returnedText);
  } else {
    *params.logFile << "<OKCoin> Error with the credentials." << std::endl;
    availability = 0.0;
  }
  return availability;
}

std::string sendLongOrder(Parameters& params, std::string direction, double quantity, double price)
{
  // signature
  std::ostringstream oss;
  oss << "amount=" << quantity << "&api_key=" << params.okcoinApi << "&price=" << price << "&symbol=btc_usd&type=" << direction << "&secret_key=" << params.okcoinSecret;
  std::string signature = oss.str();
  oss.clear();
  oss.str("");
  // content
  oss << "amount=" << quantity << "&api_key=" << params.okcoinApi << "&price=" << price << "&symbol=btc_usd&type=" << direction;
  std::string content = oss.str();
  *params.logFile << "<OKCoin> Trying to send a \"" << direction << "\" limit order: "
                  << std::setprecision(6) << quantity << "@$"
                  << std::setprecision(2) << price << "...\n";
  unique_json root { authRequest(params, "https://www.okcoin.com/api/v1/trade.do", signature, content) };
  auto orderId = std::to_string(json_integer_value(json_object_get(root.get(), "order_id")));
  *params.logFile << "<OKCoin> Done (order ID: " << orderId << ")\n" << std::endl;
  return orderId;
}

std::string sendShortOrder(Parameters& params, std::string direction, double quantity, double price) {
  // TODO
  // Unlike Bitfinex and Poloniex, on OKCoin the borrowing phase has to be done
  // as a separated step before being able to short sell.
  // Here are the steps:
  // Step                                     | Function
  // -----------------------------------------|----------------------
  //  1. ask to borrow bitcoins               | borrowBtc(amount) FIXME bug "10007: Signature does not match"
  //  2. sell the bitcoins on the market      | sendShortOrder("sell")
  //  3. <wait for the spread to close>       |
  //  4. buy back the bitcoins on the market  | sendShortOrder("buy")
  //  5. repay the bitcoins to the lender     | repayBtc(borrowId)
  return "0";
}

bool isOrderComplete(Parameters& params, std::string orderId)
{
  if (orderId == "0") return true;

  // signature
  std::ostringstream oss;
  oss << "api_key=" << params.okcoinApi << "&order_id=" << orderId << "&symbol=btc_usd" << "&secret_key=" << params.okcoinSecret;
  std::string signature = oss.str();
  oss.clear();
  oss.str("");
  // content
  oss << "api_key=" << params.okcoinApi << "&order_id=" << orderId << "&symbol=btc_usd";
  std::string content = oss.str();
  unique_json root { authRequest(params, "https://www.okcoin.com/api/v1/order_info.do", signature, content) };
  int status = json_integer_value(json_object_get(json_array_get(json_object_get(root.get(), "orders"), 0), "status"));

  return status == 2;
}

double getActivePos(Parameters& params) { return getAvail(params, "btc"); }

double getLimitPrice(Parameters& params, double volume, bool isBid)
{
  auto &exchange = queryHandle(params);
  unique_json root { exchange.getRequest("/api/v1/depth.do") };
  auto bidask = json_object_get(root.get(), isBid ? "bids" : "asks");

  // loop on volume
  *params.logFile << "<OKCoin> Looking for a limit price to fill "
                  << std::setprecision(6) << fabs(volume) << " BTC...\n";
  double tmpVol = 0.0;
  double p = 0.0;
  double v;
  size_t i = isBid ? 0 : json_array_size(bidask) - 1;
  size_t step = isBid ? 1 : -1;
  while (tmpVol < fabs(volume) * params.orderBookFactor)
  {
    p = json_number_value(json_array_get(json_array_get(bidask, i), 0));
    v = json_number_value(json_array_get(json_array_get(bidask, i), 1));
    *params.logFile << "<OKCoin> order book: "
                    << std::setprecision(6) << v << "@$"
                    << std::setprecision(2) << p << std::endl;
    tmpVol += v;
    i += step;
  }

  return p;
}

json_t* authRequest(Parameters& params, std::string url, std::string signature, std::string content)
{
  uint8_t digest[MD5_DIGEST_LENGTH];
  MD5((uint8_t *)signature.data(), signature.length(), (uint8_t *)&digest);

  std::ostringstream oss;
  oss << content << "&sign=" << hex_str<upperhex>(digest, digest + MD5_DIGEST_LENGTH);
  std::string postParameters = oss.str().c_str();
  curl_slist *headers = curl_slist_append(nullptr, "contentType: application/x-www-form-urlencoded");
  CURLcode resCurl;
  if (params.curl) {
    curl_easy_setopt(params.curl, CURLOPT_POST,1L);
    curl_easy_setopt(params.curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(params.curl, CURLOPT_POSTFIELDS, postParameters.c_str());
    curl_easy_setopt(params.curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(params.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    std::string readBuffer;
    curl_easy_setopt(params.curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(params.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(params.curl, CURLOPT_CONNECTTIMEOUT, 10L);
    resCurl = curl_easy_perform(params.curl);

    while (resCurl != CURLE_OK) {
      *params.logFile << "<OKCoin> Error with cURL. Retry in 2 sec..." << std::endl;
      sleep(4.0);
      resCurl = curl_easy_perform(params.curl);
    }
    json_error_t error;
    json_t *root = json_loads(readBuffer.c_str(), 0, &error);
    while (!root) {
      *params.logFile << "<OKCoin> Error with JSON:\n" << error.text << std::endl;
      *params.logFile << "<OKCoin> Buffer:\n" << readBuffer.c_str() << std::endl;
      *params.logFile << "<OKCoin> Retrying..." << std::endl;
      sleep(4.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
      while (resCurl != CURLE_OK) {
        *params.logFile << "<OKCoin> Error with cURL. Retry in 2 sec..." << std::endl;
        sleep(4.0);
        readBuffer = "";
        resCurl = curl_easy_perform(params.curl);
      }
      root = json_loads(readBuffer.c_str(), 0, &error);
    }
    curl_slist_free_all(headers);
    curl_easy_reset(params.curl);
    return root;
  } else {
    *params.logFile << "<OKCoin> Error with cURL init." << std::endl;
    return NULL;
  }
}

void getBorrowInfo(Parameters& params)
{
  std::ostringstream oss;
  oss << "api_key=" << params.okcoinApi << "&symbol=btc_usd&secret_key=" << params.okcoinSecret;
  std::string signature = oss.str();
  oss.clear();
  oss.str("");
  oss << "api_key=" << params.okcoinApi << "&symbol=btc_usd";
  std::string content = oss.str();
  unique_json root { authRequest(params, "https://www.okcoin.com/api/v1/borrows_info.do", signature, content) };
  *params.logFile << "<OKCoin> Borrow info:\n" << json_dumps(root.get(), 0) << std::endl;
}

int borrowBtc(Parameters& params, double amount)
{
  std::ostringstream oss;
  oss << "api_key=" << params.okcoinApi << "&symbol=btc_usd&days=fifteen&amount=" << 1 << "&rate=0.0001&secret_key=" << params.okcoinSecret;
  std::string signature = oss.str();
  oss.clear();
  oss.str("");
  oss << "api_key=" << params.okcoinApi << "&symbol=btc_usd&days=fifteen&amount=" << 1 << "&rate=0.0001";
  std::string content = oss.str();
  unique_json root { authRequest(params, "https://www.okcoin.com/api/v1/borrow_money.do", signature, content) };
  *params.logFile << "<OKCoin> Borrow "
                  << std::setprecision(6) << amount
                  << " BTC:\n" << json_dumps(root.get(), 0) << std::endl;

  bool isBorrowAccepted = json_is_true(json_object_get(root.get(), "result"));
  return isBorrowAccepted ?
         json_integer_value(json_object_get(root.get(), "borrow_id")) :
         0;
}

void repayBtc(Parameters& params, int borrowId)
{
  std::ostringstream oss;
  oss << "api_key=" << params.okcoinApi << "&borrow_id=" << borrowId << "&secret_key=" << params.okcoinSecret;
  std::string signature = oss.str();
  oss.clear();
  oss.str("");
  oss << "api_key=" << params.okcoinApi << "&borrow_id=" << borrowId;
  std::string content = oss.str();
  unique_json root { authRequest(params, "https://www.okcoin.com/api/v1/repayment.do", signature, content) };
  *params.logFile << "<OKCoin> Repay borrowed BTC:\n" << json_dumps(root.get(), 0) << std::endl;
}

}
