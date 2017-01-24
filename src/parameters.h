#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <fstream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include <mysql.h>

struct Parameters {

    std::vector<std::string> exchName;
    std::vector<double> fees;
    std::vector<bool> canShort;
    std::vector<bool> isImplemented;
    std::vector<std::string> tickerUrl;

    CURL *curl;
    double spreadEntry;
    double spreadTarget;
    unsigned maxLength;
    double priceDeltaLim;
    double trailingLim;
    unsigned trailingCount;
    double orderBookFactor;
    bool demoMode;
    bool verbose;
    std::ofstream *logFile;
    unsigned gapSec;
    unsigned debugMaxIteration;
    bool useFullCash;
    double cashForTesting;
    double maxExposure;
    bool useVolatility;
    unsigned volatilityPeriod;

    std::string bitfinexApi;
    std::string bitfinexSecret;
    double bitfinexFees;
    std::string okcoinApi;
    std::string okcoinSecret;
    double okcoinFees;
    std::string bitstampClientId;
    std::string bitstampApi;
    std::string bitstampSecret;
    double bitstampFees;
    std::string geminiApi;
    std::string geminiSecret;
    double geminiFees;
    std::string krakenApi;
    std::string krakenSecret;
    double krakenFees;
    std::string itbitApi;
    std::string itbitSecret;
    double itbitFees;
    std::string btceApi;
    std::string btceSecret;
    double btceFees;
    std::string poloniexApi;
    std::string poloniexSecret;
    double poloniexFees;

    bool sendEmail;
    std::string senderAddress;
    std::string senderUsername;
    std::string senderPassword;
    std::string smtpServerAddress;
    std::string receiverAddress;
    bool useDatabase;
    std::string dbHost;
    std::string dbName;
    std::string dbUser;
    std::string dbPassword;
    MYSQL *dbConn;

    Parameters(std::string fileName);

    void addExchange(std::string n, double f, bool h, bool m);

    int nbExch() const;
};

std::string getParameter(std::string parameter, std::ifstream &configFile);

bool getBool(std::string value);

double getDouble(std::string value);

unsigned getUnsigned(std::string value);

#endif
