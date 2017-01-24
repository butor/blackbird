#include <string.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <math.h>
#include <sstream>
#include <sys/time.h>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <jansson.h>
#include "bitstamp.h"
#include "curl_fun.h"

namespace Bitstamp {

    double getQuote(Parameters &params, bool isBid) {
        bool GETRequest = false;
        json_t *root = getJsonFromUrl(params, "https://www.bitstamp.net/api/ticker/", "", GETRequest);
        const char *quote;
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

    double getAvail(Parameters &params, std::string currency) {
        json_t *root = authRequest(params, "https://www.bitstamp.net/api/balance/", "");
        while (json_object_get(root, "message") != NULL) {
            sleep(1.0);
            *params.logFile << "<Bitstamp> Error with JSON: " << json_dumps(root, 0) << ". Retrying..." << std::endl;
            root = authRequest(params, "https://www.bitstamp.net/api/balance/", "");
        }
        double availability = 0.0;
        const char *returnedText = NULL;
        if (currency.compare("btc") == 0) {
            returnedText = json_string_value(json_object_get(root, "btc_balance"));
        } else if (currency.compare("usd") == 0) {
            returnedText = json_string_value(json_object_get(root, "usd_balance"));
        }
        if (returnedText != NULL) {
            availability = atof(returnedText);
        } else {
            *params.logFile << "<Bitstamp> Error with the credentials." << std::endl;
            availability = 0.0;
        }
        json_decref(root);
        return availability;
    }

    int sendLongOrder(Parameters &params, std::string direction, double quantity, double price) {
        *params.logFile << "<Bitstamp> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$"
                        << price << "..." << std::endl;
        std::ostringstream oss;
        oss << "https://www.bitstamp.net/api/" << direction << "/";
        std::string url = oss.str();
        oss.clear();
        oss.str("");
        oss << "amount=" << quantity << "&price=" << std::fixed << std::setprecision(2) << price;
        std::string options = oss.str();
        json_t *root = authRequest(params, url, options);
        int orderId = json_integer_value(json_object_get(root, "id"));
        if (orderId == 0) {
            *params.logFile << "<Bitstamp> Order ID = 0. Message: " << json_dumps(root, 0) << std::endl;
        }
        *params.logFile << "<Bitstamp> Done (order ID: " << orderId << ")\n" << std::endl;
        json_decref(root);
        return orderId;
    }

    bool isOrderComplete(Parameters &params, int orderId) {
        if (orderId == 0) {
            return true;
        }
        std::ostringstream oss;
        oss << "id=" << orderId;
        std::string options = oss.str();
        json_t *root = authRequest(params, "https://www.bitstamp.net/api/order_status/", options);
        std::string status = json_string_value(json_object_get(root, "status"));
        json_decref(root);
        if (status.compare("Finished") == 0) {
            return true;
        } else {
            return false;
        }
    }

    double getActivePos(Parameters &params) {
        return getAvail(params, "btc");
    }

    double getLimitPrice(Parameters &params, double volume, bool isBid) {
        bool GETRequest = false;
        json_t *root;
        if (isBid) {
            root = json_object_get(getJsonFromUrl(params, "https://www.bitstamp.net/api/order_book/", "", GETRequest),
                                   "bids");
        } else {
            root = json_object_get(getJsonFromUrl(params, "https://www.bitstamp.net/api/order_book/", "", GETRequest),
                                   "asks");
        }
        // loop on volume
        *params.logFile << "<Bitstamp> Looking for a limit price to fill " << fabs(volume) << " BTC..." << std::endl;
        double tmpVol = 0.0;
        double p;
        double v;
        int i = 0;
        while (tmpVol < fabs(volume) * params.orderBookFactor) {
            p = atof(json_string_value(json_array_get(json_array_get(root, i), 0)));
            v = atof(json_string_value(json_array_get(json_array_get(root, i), 1)));
            *params.logFile << "<Bitstamp> order book: " << v << "@$" << p << std::endl;
            tmpVol += v;
            i++;
        }
        double limPrice = 0.0;
        limPrice = atof(json_string_value(json_array_get(json_array_get(root, i - 1), 0)));
        json_decref(root);
        return limPrice;
    }

    json_t *authRequest(Parameters &params, std::string url, std::string options) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        unsigned long long nonce = (tv.tv_sec * 1000.0) + (tv.tv_usec * 0.001) + 0.5;
        std::ostringstream oss;
        oss << nonce << params.bitstampClientId << params.bitstampApi;
        unsigned char *digest;
        digest = HMAC(EVP_sha256(), params.bitstampSecret.c_str(), strlen(params.bitstampSecret.c_str()),
                      (unsigned char *) oss.str().c_str(), strlen(oss.str().c_str()), NULL, NULL);
        char mdString[SHA256_DIGEST_LENGTH + 100];  // FIXME +100
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            sprintf(&mdString[i * 2], "%02X", (unsigned int) digest[i]);
        }
        oss.clear();
        oss.str("");
        oss << "key=" << params.bitstampApi << "&signature=" << mdString << "&nonce=" << nonce << "&" << options;
        std::string postParams = oss.str().c_str();
        CURLcode resCurl;
        if (params.curl) {
            std::string readBuffer;
            curl_easy_setopt(params.curl, CURLOPT_POST, 1L);
            curl_easy_setopt(params.curl, CURLOPT_POSTFIELDS, postParams.c_str());
            curl_easy_setopt(params.curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(params.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(params.curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(params.curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(params.curl, CURLOPT_CONNECTTIMEOUT, 10L);
            resCurl = curl_easy_perform(params.curl);
            json_t *root;
            json_error_t error;
            while (resCurl != CURLE_OK) {
                *params.logFile << "<Bitstamp> Error with cURL. Retry in 2 sec..." << std::endl;
                sleep(2.0);
                readBuffer = "";
                resCurl = curl_easy_perform(params.curl);
            }
            root = json_loads(readBuffer.c_str(), 0, &error);
            while (!root) {
                *params.logFile << "<Bitstamp> Error with JSON:\n" << error.text << std::endl;
                *params.logFile << "<Bitstamp> Buffer:\n" << readBuffer.c_str() << std::endl;
                *params.logFile << "<Bitstamp> Retrying..." << std::endl;
                sleep(2.0);
                readBuffer = "";
                resCurl = curl_easy_perform(params.curl);
                while (resCurl != CURLE_OK) {
                    *params.logFile << "<Bitstamp> Error with cURL. Retry in 2 sec..." << std::endl;
                    sleep(2.0);
                    readBuffer = "";
                    resCurl = curl_easy_perform(params.curl);
                }
                root = json_loads(readBuffer.c_str(), 0, &error);
            }
            curl_easy_reset(params.curl);
            return root;
        } else {
            *params.logFile << "<Bitstamp> Error with cURL init." << std::endl;
            return NULL;
        }
    }

}

