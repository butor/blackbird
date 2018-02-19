#include "binance.h"
#include "parameters.h"
#include "utils/restapi.h"
#include "unique_json.hpp"
#include "hex_str.hpp"
#include "time_fun.h"
#include "openssl/sha.h"
#include "openssl/hmac.h"
#include <array>
#include <algorithm>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <iomanip>


namespace Binance
{

static json_t *authRequest(Parameters &, std::string, std::string, std::string);

static std::string getSignature(Parameters &params, std::string payload);

static RestApi &queryHandle(Parameters &params)
{
    static RestApi query("https://api.binance.com",
                         params.cacert.c_str(), *params.logFile);
    return query;
}

quote_t getQuote(Parameters &params)
{
    auto &exchange = queryHandle(params);
    std::string x;
    //TODO: build real currency string
    x += "/api/v3/ticker/bookTicker?symbol=";
    x += "BTCUSDT";
    //params.leg2.c_str();
    unique_json root{exchange.getRequest(x)};
    double quote = atof(json_string_value(json_object_get(root.get(), "bidPrice")));
    auto bidValue = quote ? quote : 0.0;
    quote = atof(json_string_value(json_object_get(root.get(), "askPrice")));
    auto askValue = quote ? quote : 0.0;

    return std::make_pair(bidValue, askValue);
}

double getAvail(Parameters &params, std::string currency)
{

    currency = symbolTransform(params, currency);


    unique_json root{authRequest(params, "GET", "/api/v3/account", "")};
    size_t arraySize = json_array_size(json_object_get(root.get(), "balances"));
    double available = 0.0;
    const char *currstr;
    auto balances = json_object_get(root.get(), "balances");

    // this loops 151 times....
    for (size_t i = 0; i < arraySize; i++)
    {
        std::string tmpCurrency = json_string_value(json_object_get(json_array_get(balances, i), "asset"));
        if (tmpCurrency.compare(currency) == 0)

        {
            currstr = json_string_value(json_object_get(json_array_get(balances, i), "free"));
            if (currstr != NULL)
            {
                available = atof(currstr);

                break;
            }
            else
            {
                *params.logFile << "<Binance> Error with currency string" << std::endl;
                available = 0.0;
            }
        }
    }
    return available;
}
//TODO: Currency String here
std::string sendLongOrder(Parameters &params, std::string direction, double quantity, double price)
{
    if (direction.compare("buy") != 0 && direction.compare("sell") != 0)
    {
        *params.logFile << "<Binance> Error: Neither \"buy\" nor \"sell\" selected" << std::endl;
        return "0";
    }
    *params.logFile << "<Binance> Trying to send a \"" << direction << "\" limit order: "
                    << std::setprecision(8) << quantity << " @ $"
                    << std::setprecision(8) << price << "...\n";
    std::string symbol = "BTCUSDT";
    std::transform(direction.begin(), direction.end(), direction.begin(), toupper);
    std::string type = "LIMIT";
    std::string tif = "GTC";
    std::string pricelimit = std::to_string(price);
    std::string volume = std::to_string(quantity);
    std::string options = "symbol=" + symbol + "&side=" + direction + "&type=" + type + "&timeInForce=" + tif + "&price=" + pricelimit + "&quantity=" + volume;
    unique_json root{authRequest(params, "POST", "/api/v3/order", options)};
    long txid = json_integer_value(json_object_get(root.get(), "orderId"));
    std::string order = std::to_string(txid);
    *params.logFile << "<Binance> Done (transaction ID: " << order << ")\n"
                    << std::endl;
    return order;
}

//TODO: probably not necessary
std::string sendShortOrder(Parameters &params, std::string direction, double quantity, double price)
{
    return "0";
}

bool isOrderComplete(Parameters &params, std::string orderId)
{
    unique_json root{authRequest(params, "GET", "/api/v3/openOrders", "")};
    size_t arraySize = json_array_size(root.get());
    bool complete = true;
    const char *idstr;
    for (size_t i = 0; i < arraySize; i++)
    {
        //SUGGEST: this is sort of messy
        long tmpInt = json_integer_value(json_object_get(json_array_get(root.get(), i), "orderId"));
        std::string tmpId = std::to_string(tmpInt);
        if (tmpId.compare(orderId.c_str()) == 0)
        {
            idstr = json_string_value(json_object_get(json_array_get(root.get(), i), "status"));
            *params.logFile << "<Binance> Order still open (Status:" << idstr << ")" << std::endl;
            complete = false;
        }
    }
    return complete;
}
//TODO: Currency

double getActivePos(Parameters &params, std::string orderId)
{
  double activeSize = 0.0;
  if (!orderId.empty()){
    std::string builder = "symbol=BTCUSDT&orderId=";
    builder += orderId.c_str();
    unique_json root { authRequest(params, "GET", "/api/v3/order",builder) };
    activeSize = atof(json_string_value(json_object_get(root.get(),"origQty")));
  }

  return activeSize;  

}

double getLimitPrice(Parameters &params, double volume, bool isBid)
{
    auto &exchange = queryHandle(params);
    //TODO build a real URI string here
    unique_json root{exchange.getRequest("/api/v1/depth?symbol=BTCUSDT")};
    auto bidask = json_object_get(root.get(), isBid ? "bids" : "asks");
    *params.logFile << "<Binance Looking for a limit price to fill "
                    << std::setprecision(8) << fabs(volume) << " Legx...\n";
    double tmpVol = 0.0;
    double p = 0.0;
    double v;
    int i = 0;
    while (tmpVol < fabs(volume) * params.orderBookFactor)
    {
        p = atof(json_string_value(json_array_get(json_array_get(bidask, i), 0)));
        v = atof(json_string_value(json_array_get(json_array_get(bidask, i), 1)));
        *params.logFile << "<Binance> order book: "
                        << std::setprecision(8) << v << "@$"
                        << std::setprecision(8) << p << std::endl;
        tmpVol += v;
        i++;
    }
    return p;
}

json_t *authRequest(Parameters &params, std::string method, std::string request, std::string options)
{
    //create timestamp Binance is annoying and requires their servertime
    auto &exchange = queryHandle(params);
    unique_json stamper{exchange.getRequest("/api/v1/time")};
    long stamp = json_integer_value(json_object_get(stamper.get(), "serverTime"));
    std::string timestamp = std::to_string(stamp);
    // create empty payload
    std::string payload;
    std::string uri;
    std::string sig;
    //our headers, might want to edit later to go into options check
    std::array<std::string, 1> headers{
        "X-MBX-APIKEY:" + params.binanceApi,
    };
    if (method.compare("POST") == 0)
    {
        payload += options + "&timestamp=" + timestamp;
        sig += getSignature(params, payload);
        uri += request + "?" + options + "&timestamp=" + timestamp + "&signature=" + sig;
        return exchange.postRequest(uri, make_slist(std::begin(headers), std::end(headers)));
    }
    else
    {
        if (options.empty())
        {
            payload += "timestamp=" + timestamp;
            sig += getSignature(params, payload);
            uri += request + "?timestamp=" + timestamp + "&signature=" + sig;
        }
        else
        {
            payload += options + "&timestamp=" + timestamp;
            sig += getSignature(params, payload);
            uri += request + "?" + options + "&timestamp=" + timestamp + "&signature=" + sig;
        }
        return exchange.getRequest(uri, make_slist(std::begin(headers), std::end(headers)));
    }
}

static std::string getSignature(Parameters &params, std::string payload)
{
    uint8_t *hmac_digest = HMAC(EVP_sha256(),
                                params.binanceSecret.c_str(), params.binanceSecret.size(),
                                reinterpret_cast<const uint8_t *>(payload.data()), payload.size(),
                                NULL, NULL);
    std::string api_sign_header = hex_str(hmac_digest, hmac_digest + SHA256_DIGEST_LENGTH);
    return api_sign_header;
}


std::string symbolTransform(Parameters& params, std::string leg){
    std::transform(leg.begin(),leg.end(), leg.begin(), ::toupper);
    if (leg.compare("USD")==0 || leg.compare("USDT")==0){
        return "USDT"; //WARNING: allowing hardswap of USDT, not applicable to all users
    } else if (leg.compare("BTC")==0){
        return "BTC";
    } else {
        *params.logFile << "<Binance> WARNING: Currency not supported" << std::endl;
        return "";
    }
}

void testBinance()
{

    Parameters params("blackbird.conf");
    params.logFile = new std::ofstream("./test.log", std::ofstream::trunc);

    std::string orderId;

    //std::cout << "Current value LEG1_LEG2 bid: " << getQuote(params).bid() << std::endl;
    //std::cout << "Current value LEG1_LEG2 ask: " << getQuote(params).ask() << std::endl;
    //std::cout << "Current balance BTC: " << getAvail(params, "BTC") << std::endl;
    //std::cout << "Current balance USD: " << getAvail(params, "USD")<< std::endl;
    //std::cout << "Current balance ETH: " << getAvail(params, "ETH")<< std::endl;
    //std::cout << "Current balance BNB: " << getAvail(params, "BNB")<< std::endl;
    //std::cout << "current bid limit price for .04 units: " << getLimitPrice(params, 0.04, true) << std::endl;
    //std::cout << "Current ask limit price for 10 units: " << getLimitPrice(params, 10.0, false) << std::endl;
    //std::cout << "Sending buy order for 0.003603 BTC @ BID! - TXID: " << std::endl;
    //orderId = sendLongOrder(params, "buy", 0.003603, getLimitPrice(params,0.003603,true));
    //std::cout << orderId << std::endl;
    //std::cout << "Buy Order is complete: " << isOrderComplete(params, orderId) << std::endl;

    //std::cout << "Sending sell order for 0.003603 BTC @ 10000 USD - TXID: " << std::endl;
    //orderId = sendLongOrder(params, "sell", 0.003603, 10000);
    //std::cout << orderId << std::endl;
    //std::cout << "Sell order is complete: " << isOrderComplete(params, orderId) << std::endl;
    //std::cout << "Active Position: " << getActivePos(params,orderId.c_str());
}
}
