#ifndef RESULT_H
#define RESULT_H

#include <ostream>
#include <ctime>
#include <string>
#include <list>

// stores the information of a complete long/short trade (2 entry trades, 2 exit trades)
struct Result {
    unsigned id;
    unsigned idExchLong;
    unsigned idExchShort;
    double exposure;
    double feesLong;
    double feesShort;
    std::time_t entryTime;
    std::time_t exitTime;
    std::string exchNameLong;
    std::string exchNameShort;
    double priceLongIn;
    double priceShortIn;
    double priceLongOut;
    double priceShortOut;
    double spreadIn;
    double spreadOut;
    double exitTarget;
    // FIXME: the arrays should have a dynamic size
    double minSpread[10][10];
    double maxSpread[10][10];
    double trailing[10][10];
    unsigned trailingWaitCount[10][10];
    std::list<double> volatility[10][10];
    double usdTotBalanceBefore;
    double usdTotBalanceAfter;

    double targetPerfLong();

    double targetPerfShort();

    double actualPerf();

    double getTradeLengthInMinute();

    void printEntryInfo(std::ostream &logFile);

    void printExitInfo(std::ostream &logFile);

    void reset();
};

#endif
