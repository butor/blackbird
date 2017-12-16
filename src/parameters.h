#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "unique_sqlite.hpp"

#include <fstream>
#include <string>
#include <vector>
#include <curl/curl.h>

// Stores all the parameters defined
// in the configuration file.
struct Parameters {

    std::vector <std::string> exchName;
    std::vector<double> fees;
    std::vector<bool> canShort;
    std::vector<bool> isImplemented;
    std::vector <std::string> tickerUrl;

    CURL *curl;
    double venueSpreadTh;
    double spreadEntry;
    double spreadTarget;
    unsigned maxLength;
    double priceDeltaLim;
    double trailingLim;
    unsigned trailingCount;
    double orderBookFactor;
    bool demoMode;
    std::string leg1;
    std::string leg2;
    bool verbose;
    std::ofstream *logFile;
    unsigned interval;
    unsigned debugMaxIteration;
    bool useFullExposure;
    double testedExposure;
    double maxExposure;
    bool useVolatility;
    unsigned volatilityPeriod;
    std::string cacert;

    std::string bitfinexApi;
    std::string bitfinexSecret;
    double bitfinexFees;
    bool bitfinexEnable;
    std::string okcoinApi;
    std::string okcoinSecret;
    double okcoinFees;
    bool okcoinEnable;
    std::string bitstampClientId;
    std::string bitstampApi;
    std::string bitstampSecret;
    double bitstampFees;
    bool bitstampEnable;
    std::string geminiApi;
    std::string geminiSecret;
    double geminiFees;
    bool geminiEnable;
    std::string krakenApi;

    std::string krakenSecret;
    double krakenFees;
    bool krakenEnable;
    std::string itbitApi;
    std::string itbitSecret;
    double itbitFees;
    bool itbitEnable;
    std::string btceApi;
    std::string btceSecret;
    double btceFees;
    bool btceEnable;
    std::string poloniexApi;
    std::string poloniexSecret;
    double poloniexFees;
    bool poloniexEnable;
    std::string gdaxApi;
    std::string gdaxSecret;
    double gdaxFees;
    bool gdaxEnable;
    std::string quadrigaApi;
    std::string quadrigaSecret;
    std::string quadrigaClientId;
    double quadrigaFees;
    bool quadrigaEnable;

    std::string cexioApi;
    std::string cexioSecret;
    std::string cexioClientId;
    double cexioFees;
    bool cexioEnable;
    bool sendEmail;

    std::string senderAddress;
    std::string senderUsername;
    std::string senderPassword;
    std::string smtpServerAddress;
    std::string receiverAddress;

    std::string dbFile;
    unique_sqlite dbConn;

    Parameters(std::string fileName);

    void addExchange(std::string n, double f, bool h, bool m);

    int nbExch() const;
};

// Copies the parameters from the configuration file
// to the Parameter structure.
std::string getParameter(std::string parameter, std::ifstream& configFile);

bool getBool(std::string value);

double getDouble(std::string value);

unsigned getUnsigned(std::string value);

#endif
