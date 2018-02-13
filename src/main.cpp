#include "bitcoin.h"
#include "result.h"
#include "time_fun.h"
#include "curl_fun.h"
#include "db_fun.h"
#include "parameters.h"
#include "check_entry_exit.h"
#include "exchanges/bitfinex.h"
#include "exchanges/bittrex.h"
#include "exchanges/gemini.h"
#include "exchanges/kraken.h"
#include "exchanges/gdax.h"
#include "exchanges/binance.h"
#include "exchanges/poloniex.h"
#include "utils/send_email.h"
#include "getpid.h"
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>

// The 'typedef' declarations needed for the function arrays
// These functions contain everything needed to communicate with
// the exchanges, like getting the quotes or the active positions.
// Each function is implemented in the files located in the 'exchanges' folder.
typedef quote_t (*getQuoteType)(Parameters &params);
typedef double (*getAvailType)(Parameters &params, std::string currency);
typedef std::string (*sendOrderType)(Parameters &params, std::string direction, double quantity, double price);
typedef bool (*isOrderCompleteType)(Parameters &params, std::string orderId);
typedef double (*getActivePosType)(Parameters &params, std::string orderId);
typedef double (*getLimitPriceType)(Parameters &params, double volume, bool isBid);

// This structure contains the balance of both exchanges,
// *before* and *after* an arbitrage trade.
// This is used to compute the performance of the trade,
// by comparing the balance before and after the trade.
struct Balance
{
  double leg1, leg2;
  double leg1After, leg2After;
};

// 'main' function.
// Blackbird doesn't require any arguments for now.
int main(int argc, char **argv)
{
  std::cout << "Hunting..." << std::endl;
  // Replaces the C++ global locale with the user-preferred locale
  std::locale mylocale("");
  // Loads all the parameters
  Parameters params("bird.conf");
  // Does some verifications about the parameters
  if (!params.demoMode)
  {
    if (!params.useFullExposure)
    {
      if (params.testedExposure < 10.0 && params.leg2.compare("USD") == 0)
      {
        // TODO do the same check for other currencies. Is there a limi?
        std::cout << "ERROR: Minimum USD needed: $10.00" << std::endl;
        exit(EXIT_FAILURE);
      }
      if (params.testedExposure > params.maxExposure)
      {
        std::cout << "ERROR: Test exposure (" << params.testedExposure << ") is above max exposure (" << params.maxExposure << ")\n"
                  << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  // Connects to the SQLite3 database.
  // This database is used to collect bid and ask information
  // from the exchanges. Not really used for the moment, but
  // would be useful to collect historical bid/ask data.
  if (createDbConnection(params) != 0)
  {
    std::cerr << "ERROR: cannot connect to the database \'" << params.dbFile << "\'\n"
              << std::endl;
    exit(EXIT_FAILURE);
  }
  if (createTradeTable(params) != 0)
  {
    std::cerr << "ERROR: problems with database \'" << params.dbFile << "\'\n"
              << std::endl;
    exit(EXIT_FAILURE);
  };

  // Function arrays containing all the exchanges functions
  // using the 'typedef' declarations from above.
  getQuoteType getQuote[7];
  getAvailType getAvail[7];
  sendOrderType sendLongOrder[7];
  sendOrderType sendShortOrder[7];
  isOrderCompleteType isOrderComplete[7];
  getActivePosType getActivePos[7];
  getLimitPriceType getLimitPrice[7];
  std::string dbTableName[7];

  // Adds the exchange functions to the arrays for all the defined exchanges
  int index = 0;
  if (params.bitfinexEnable &&
      (params.bitfinexApi.empty() == false || params.demoMode == true))
  {
    params.addExchange(1, "Bitfinex", params.bitfinexFees, true, true);
    getQuote[index] = Bitfinex::getQuote;
    getAvail[index] = Bitfinex::getAvail;
    sendLongOrder[index] = Bitfinex::sendLongOrder;
    sendShortOrder[index] = Bitfinex::sendShortOrder;
    isOrderComplete[index] = Bitfinex::isOrderComplete;
    getActivePos[index] = Bitfinex::getActivePos;
    getLimitPrice[index] = Bitfinex::getLimitPrice;

    dbTableName[index] = "bitfinex";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.geminiEnable &&
      (params.geminiApi.empty() == false || params.demoMode == true))
  {
    params.addExchange(2, "Gemini", params.geminiFees, false, true);
    getQuote[index] = Gemini::getQuote;
    getAvail[index] = Gemini::getAvail;
    sendLongOrder[index] = Gemini::sendLongOrder;
    isOrderComplete[index] = Gemini::isOrderComplete;
    getActivePos[index] = Gemini::getActivePos;
    getLimitPrice[index] = Gemini::getLimitPrice;

    dbTableName[index] = "gemini";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.krakenEnable &&
      (params.krakenApi.empty() == false || params.demoMode == true))
  {
    params.addExchange(3, "Kraken", params.krakenFees, true, true);
    getQuote[index] = Kraken::getQuote;
    getAvail[index] = Kraken::getAvail;
    sendLongOrder[index] = Kraken::sendLongOrder;
    sendShortOrder[index] = Kraken::sendShortOrder;
    isOrderComplete[index] = Kraken::isOrderComplete;
    getActivePos[index] = Kraken::getActivePos;
    getLimitPrice[index] = Kraken::getLimitPrice;

    dbTableName[index] = "kraken";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.bittrexEnable &&
      (params.bittrexApi.empty() == false || params.demoMode == true))
  {
    params.addExchange(4, "Bittrex", params.bittrexFees, false, true);
    getQuote[index] = Bittrex::getQuote;
    getAvail[index] = Bittrex::getAvail;
    sendLongOrder[index] = Bittrex::sendLongOrder;
    sendShortOrder[index] = Bittrex::sendShortOrder;
    isOrderComplete[index] = Bittrex::isOrderComplete;
    getActivePos[index] = Bittrex::getActivePos;
    getLimitPrice[index] = Bittrex::getLimitPrice;
    dbTableName[index] = "bittrex";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.gdaxEnable &&
      (params.gdaxApi.empty() == false || params.demoMode == true))
  {
    params.addExchange(5, "GDAX", params.gdaxFees, false, true);
    getQuote[index] = GDAX::getQuote;
    getAvail[index] = GDAX::getAvail;
    getActivePos[index] = GDAX::getActivePos;
    getLimitPrice[index] = GDAX::getLimitPrice;
    sendLongOrder[index] = GDAX::sendLongOrder;
    isOrderComplete[index] = GDAX::isOrderComplete;
    dbTableName[index] = "gdax";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.binanceEnable &&
      (params.binanceApi.empty() == false || params.demoMode == true))
  {
    params.addExchange(6, "Binance", params.binanceFees, false, true);
    getQuote[index] = Binance::getQuote;
    getAvail[index] = Binance::getAvail;
    sendLongOrder[index] = Binance::sendLongOrder;
    sendShortOrder[index] = Binance::sendShortOrder;
    isOrderComplete[index] = Binance::isOrderComplete;
    getActivePos[index] = Binance::getActivePos;
    getLimitPrice[index] = Binance::getLimitPrice;
    dbTableName[index] = "binance";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.poloniexEnable &&
      (params.poloniexApi.empty() == false || params.demoMode == true))
  {
    params.addExchange(7, "Poloniex", params.poloniexFees, false, true);
    getQuote[index] = Poloniex::getQuote;
    getAvail[index] = Poloniex::getAvail;
    sendLongOrder[index] = Poloniex::sendLongOrder;
    sendShortOrder[index] = Poloniex::sendShortOrder;
    isOrderComplete[index] = Poloniex::isOrderComplete;
    getActivePos[index] = Poloniex::getActivePos;
    getLimitPrice[index] = Poloniex::getLimitPrice;
    dbTableName[index] = "poloniex";
    createTable(dbTableName[index], params);

    index++;
  }

  // We need at least two exchanges to run
  if (index < 2)
  {
    std::cout << "ERROR: Bird needs at least two Bitcoin exchanges. Please edit the config.json file to add new exchanges\n"
              << std::endl;
    exit(EXIT_FAILURE);
  }
  // Creates the CSV file that will collect the trade results
  std::string currDateTime = printDateTimeFileName();
  std::string csvFileName = "output/bird_result_" + currDateTime + ".csv";
  std::ofstream csvFile(csvFileName, std::ofstream::trunc);
  csvFile << "TRADE_ID,EXCHANGE_LONG,EXHANGE_SHORT,ENTRY_TIME,EXIT_TIME,DURATION,"
          << "TOTAL_EXPOSURE,BALANCE_BEFORE,BALANCE_AFTER,RETURN"
          << std::endl;
  // Creates the log file where all events will be saved
  std::string logFileName = "output/bird_log_" + currDateTime + ".log";
  std::ofstream logFile(logFileName, std::ofstream::trunc);
  logFile.imbue(mylocale);
  logFile.precision(2);
  logFile << std::fixed;
  params.logFile = &logFile;
  // Log file header
  logFile << "--------------------------------------------" << std::endl;
  logFile << "|             Arbitrage Log File            |" << std::endl;
  logFile << "--------------------------------------------\n"
          << std::endl;
  logFile << "Arb started on " << printDateTime() << "\n"
          << std::endl;

  logFile << "Connected to database \'" << params.dbFile << "\'\n"
          << std::endl;

  if (params.demoMode)
  {
    logFile << "Demo mode: trades won't be generated\n"
            << std::endl;
  }

  // Shows which pair we are trading
  logFile << "Pair traded: " << params.leg1 << "/" << params.leg2 << "\n"
          << std::endl;

  std::cout << "Log file generated: " << logFileName << "\nHunting... (pid " << getpid() << ")\n"
            << std::endl;
  int numExch = params.nbExch();
  int comboExch = 0;

  // The btcVec vector contains details about every exchange,
  // like fees, as specified in bitcoin.h
  std::vector<Bitcoin> btcVec;
  btcVec.reserve(numExch);
  // Creates a new Bitcoin structure within btcVec for every exchange we want to trade on
  for (int i = 0; i < numExch; ++i)
  {
    btcVec.push_back(Bitcoin(params.exchId[i], params.exchName[i], params.fees[i], params.canShort[i], params.isImplemented[i]));
    if (params.canShort[i])
    {
      comboExch += 1;
    }
  }
  // this creates the number of possible trades
  int realNum = (comboExch - 1) * comboExch + (numExch - comboExch) * comboExch;
  // Inits cURL connections
  params.curl = curl_easy_init();
  // Shows the spreads
  logFile << "[ Targets ]\n"
          << std::setprecision(2)
          << "   Spread Entry:  " << params.spreadEntry * 100.0 << "%\n"
          << "   Spread Target: " << params.spreadTarget * 100.0 << "%\n";

  // SpreadEntry and SpreadTarget have to be positive,
  // Otherwise we will loose money on every trade.
  if (params.spreadEntry <= 0.0)
  {
    logFile << "   WARNING: Spread Entry should be positive" << std::endl;
  }
  if (params.spreadTarget <= 0.0)
  {
    logFile << "   WARNING: Spread Target should be positive" << std::endl;
  }
  logFile << std::endl;
  logFile << "[ Current balances ]" << std::endl;
  // Gets the the balances from every exchange
  // This is only done when not in Demo mode.
  std::vector<Balance> balance(numExch);
  if (!params.demoMode)
    std::transform(getAvail, getAvail + numExch,
                   begin(balance),
                   [&params](decltype(*getAvail) apply) {
                     Balance tmp{};
                     tmp.leg1 = apply(params, params.leg1.c_str());
                     tmp.leg2 = apply(params, params.leg2.c_str());
                     return tmp;
                   });

  // TODO: is m that necessary? there must be a way to combine a vec and struct without strict defs.
  // the tradesVec contains details of every trade executed.

  // Build possible trade vectors
  Result m;
  m.reset();
  //one vector for entries, other for active trades
  std::vector<Result> entryVec;
  entryVec.reserve(realNum);
  std::vector<Result> tradeVec;
  tradeVec.reserve(realNum);
  // could just passrealNum in their dec but whatever
  for (int i = 0; i < numExch; i++)
  {
    for (int j = 0; j < numExch; j++)
    {
      if (i != j)
      {
        if (btcVec[j].getHasShort())
        {
          // all of the combinations will be loaded into vectors with exchIds
          m.idExchLong = i;
          m.idExchShort = j;
          entryVec.push_back(m);
        }
      }
    }
  }
  int numTrades = getNumTradesOutstanding(params);
  if (realNum < numTrades)
  {
    std::cout << "Error: You have more outstanding positions than possible." << std::endl;
    std::cout << "You may have enabled/disabled exchanges since last run, Check your Db/exchanges." << std::endl;
    exit(EXIT_FAILURE);
  }
  
  getTradesFromDb(params, tradeVec);

  if (!tradeVec.empty())
  {
    for (int i = 0; i < numExch; i++)
    {
      for (size_t j = 0; j < tradeVec.size(); j++)
      {
        if (entryVec[i].idExchLong == tradeVec[j].idExchLong && entryVec[i].idExchShort == tradeVec[j].idExchShort){
          entryVec.erase(entryVec.begin()+i);
        }
      }
    }
  }

  // Writes the current balances into the log file
  for (int i = 0; i < numExch; ++i)
  {
    logFile << "   " << params.exchName[i] << ":\t";
    if (params.demoMode)
    {
      logFile << "n/a (demo mode)" << std::endl;
    }
    else if (!params.isImplemented[i])
    {
      logFile << "n/a (API not implemented)" << std::endl;
    }
    else
    {
      logFile << std::setprecision(8) << balance[i].leg2 << " " << params.leg2 << "\t"
              << std::setprecision(8) << balance[i].leg1 << " " << params.leg1 << std::endl;
    }
  }
  logFile << std::endl;
  logFile << "[ Cash exposure ]\n";
  if (params.demoMode)
  {
    logFile << "   No cash - Demo mode\n";
  }
  else
  {
    if (params.useFullExposure)
    {
      logFile << "   FULL exposure used!\n";
    }
    else
    {
      logFile << "   TEST exposure used\n   Value: "
              << std::setprecision(2) << params.testedExposure << '\n';
    }
  }
  logFile << std::endl;
  // Code implementing the loop function, that runs
  // every 'Interval' seconds.
  time_t rawtime = time(nullptr);
  tm timeinfo = *localtime(&rawtime);
  using std::this_thread::sleep_for;
  using millisecs = std::chrono::milliseconds;
  using secs = std::chrono::seconds;
  // Waits for the next 'interval' seconds before starting the loop
  while ((int)timeinfo.tm_sec % params.interval != 0)
  {
    sleep_for(millisecs(100));
    time(&rawtime);
    timeinfo = *localtime(&rawtime);
  }
  if (!params.verbose)
  {
    logFile << "Running..." << std::endl;
  }

  int resultId = 0;
  unsigned currIteration = 0;
  bool stillRunning = true;
  time_t currTime;
  time_t diffTime;

  // Main analysis loop
  while (stillRunning)
  {
    currTime = mktime(&timeinfo);
    time(&rawtime);
    diffTime = difftime(rawtime, currTime);
    // Checks if we are already too late in the current iteration
    // If that's the case we wait until the next iteration
    // and we show a warning in the log file.
    if (diffTime > 0)
    {
      logFile << "WARNING: " << diffTime << " second(s) too late at " << printDateTime(currTime) << std::endl;
      timeinfo.tm_sec += (ceil(diffTime / params.interval) + 1) * params.interval;
      currTime = mktime(&timeinfo);
      sleep_for(secs(params.interval - (diffTime % params.interval)));
      logFile << std::endl;
    }
    else if (diffTime < 0)
    {
      sleep_for(secs(-diffTime));
    }
    // Header for every iteration of the loop
    if (params.verbose)
    {
      logFile << "[ " << printDateTime(currTime) << " ]" << std::endl;
    }
    // Gets the bid and ask of all the exchanges
    for (int i = 0; i < numExch; ++i)
    {
      auto quote = getQuote[i](params);
      double bid = quote.bid();
      double ask = quote.ask();

      // Saves the bid/ask into the SQLite database
      addBidAskToDb(dbTableName[i], printDateTimeDb(currTime), bid, ask, params);

      // If there is an error with the bid or ask (i.e. value is null),
      // we show a warning but we don't stop the loop.
      if (bid == 0.0)
      {
        logFile << "   WARNING: " << params.exchName[i] << " bid is null" << std::endl;
      }
      if (ask == 0.0)
      {
        logFile << "   WARNING: " << params.exchName[i] << " ask is null" << std::endl;
      }
      // Shows the bid/ask information in the log file
      if (params.verbose)
      {
        logFile << "   " << params.exchName[i] << ": \t"
                << std::setprecision(2)
                << bid << " / " << ask << std::endl;
      }
      // Updates the Bitcoin vector with the latest bid/ask data
      btcVec[i].updateData(quote);
      curl_easy_reset(params.curl);
    }
    if (params.verbose)
    {
      logFile << "   ----------------------------" << std::endl;
    }
    // TODO: Reimplement Volatility
    // Stores all the spreads in arrays to
    // compute the volatility. The volatility
    // is not used for the moment.
    // if (params.useVolatility) {
    //   for (int i = 0; i < numExch; ++i) {
    //     for (int j = 0; j < numExch; ++j) {
    //       if (i != j) {
    //         if (btcVec[j].getHasShort()) {
    //           double longMidPrice = btcVec[i].getMidPrice();
    //           double shortMidPrice = btcVec[j].getMidPrice();
    //           if (longMidPrice > 0.0 && shortMidPrice > 0.0) {
    //             if (res.volatility[i][j].size() >= params.volatilityPeriod) {
    //               res.volatility[i][j].pop_back();
    //             }
    //             res.volatility[i][j].push_front((longMidPrice - shortMidPrice) / longMidPrice);
    //           }
    //         }
    //       }
    //     }
    //   }
    // }
    // Looks for arbitrage opportunities on all the exchange combinations
    for (size_t i = 0; i < entryVec.size(); i++)
    {
      if (checkEntry(&btcVec[entryVec[i].idExchLong], &btcVec[entryVec[i].idExchShort], entryVec[i], params))
      {
        // An entry opportunity has been found!
        entryVec[i].exposure = std::min(balance[entryVec[i].idExchLong].leg2, balance[entryVec[i].idExchShort].leg2);
        if (params.demoMode)
        {
          logFile << "INFO: Opportunity found but no trade will be generated (Demo mode)" << std::endl;
          break;
        }
        if (entryVec[i].exposure == 0.0)
        {
          logFile << "WARNING: Opportunity found but no cash available. Trade canceled" << std::endl;
          break;
        }
        //TODO: I can put the withdrawal/deposit function here
        if (params.useFullExposure == false && entryVec[i].exposure <= params.testedExposure)
        {
          logFile << "WARNING: Opportunity found but not enough cash. Need more than TEST cash (min. $"
                  << std::setprecision(2) << params.testedExposure << "). Trade canceled" << std::endl;
          break;
        }
        if (params.useFullExposure)
        {
          // Removes 1% of the exposure to have
          // a little bit of margin.
          entryVec[i].exposure -= 0.01 * entryVec[i].exposure;
          if (entryVec[i].exposure > params.maxExposure)
          {
            logFile << "WARNING: Opportunity found but exposure ("
                    << std::setprecision(2)
                    << entryVec[i].exposure << ") above the limit\n"
                    << "         Max exposure will be used instead (" << params.maxExposure << ")" << std::endl;
            entryVec[i].exposure = params.maxExposure;
          }
        }
        else
        {
          entryVec[i].exposure = params.testedExposure;
        }
        // Checks the volumes and, based on that, computes the limit prices
        // that will be sent to the exchanges
        double volumeLong = entryVec[i].exposure / btcVec[entryVec[i].idExchLong].getAsk();
        double volumeShort = entryVec[i].exposure / btcVec[entryVec[i].idExchShort].getBid();
        double limPriceLong = getLimitPrice[entryVec[i].idExchLong](params, volumeLong, false);
        double limPriceShort = getLimitPrice[entryVec[i].idExchShort](params, volumeShort, true);
        if (limPriceLong == 0.0 || limPriceShort == 0.0)
        {
          logFile << "WARNING: Opportunity found but error with the order books (limit price is null). Trade canceled\n";
          logFile.precision(2);
          logFile << "         Long limit price:  " << limPriceLong << std::endl;
          logFile << "         Short limit price: " << limPriceShort << std::endl;
          entryVec[i].trailing = -1.0;
          break;
        }
        if (limPriceLong - entryVec[i].priceLongIn > params.priceDeltaLim || entryVec[i].priceShortIn - limPriceShort > params.priceDeltaLim)
        {
          logFile << "WARNING: Opportunity found but not enough liquidity. Trade canceled\n";
          logFile.precision(2);
          logFile << "         Target long price:  " << entryVec[i].priceLongIn << ", Real long price:  " << limPriceLong << std::endl;
          logFile << "         Target short price: " << entryVec[i].priceShortIn << ", Real short price: " << limPriceShort << std::endl;
          entryVec[i].trailing = -1.0;
          break;
        }
        // We are in market now, meaning we have positions on leg1 (the hedged on)
        // We store the details of that first trade into the Result structure.
        resultId++;
        entryVec[i].id = resultId;
        entryVec[i].entryTime = currTime;
        entryVec[i].priceLongIn = limPriceLong;
        entryVec[i].priceShortIn = limPriceShort;
        entryVec[i].printEntryInfo(*params.logFile);
        entryVec[i].maxSpread = -1.0;
        entryVec[i].minSpread = 1.0;
        entryVec[i].trailing = 1.0;
        // Send the orders to the two exchanges
        auto longOrderId = sendLongOrder[entryVec[i].idExchLong](params, "buy", volumeLong, limPriceLong);
        auto shortOrderId = sendShortOrder[entryVec[i].idExchShort](params, "sell", volumeShort, limPriceShort);
        logFile << "Waiting for the two orders to be filled..." << std::endl;
        sleep_for(millisecs(5000));
        bool isLongOrderComplete = isOrderComplete[entryVec[i].idExchLong](params, longOrderId);
        bool isShortOrderComplete = isOrderComplete[entryVec[i].idExchShort](params, shortOrderId);
        // Loops until both orders are completed
        while (!isLongOrderComplete || !isShortOrderComplete)
        {
          sleep_for(millisecs(3000));
          if (!isLongOrderComplete)
          {
            logFile << "Long order on " << params.exchName[entryVec[i].idExchLong] << " still open..." << std::endl;
            isLongOrderComplete = isOrderComplete[entryVec[i].idExchLong](params, longOrderId);
          }
          if (!isShortOrderComplete)
          {
            logFile << "Short order on " << params.exchName[entryVec[i].idExchShort] << " still open..." << std::endl;
            isShortOrderComplete = isOrderComplete[entryVec[i].idExchShort](params, shortOrderId);
          }
        }
        // Both orders are now fully executed
        logFile << "Done" << std::endl;
        // Adding new parameters to result file to keep track of executed trades
        entryVec[i].longExchTradeId = longOrderId;
        entryVec[i].shortExchTradeId = shortOrderId;
        // Stores the partial result to file in case
        // the program exits before closing the position.
        if (addTradesToDb(entryVec[i], params, 0) != 0)
        {
          std::cerr << "ERROR: problems with database \'" << params.dbFile << "\'\n"
                    << std::endl;
          exit(EXIT_FAILURE);
        };
        tradeVec.push_back(entryVec[i]);
        entryVec.erase(entryVec.begin() + i);
        longOrderId = "0";
        shortOrderId = "0";
        break;
      }
    }
    if (params.verbose)
    {
      logFile << std::endl;
    }
    for (size_t i = 0; i < tradeVec.size(); i++)
    {
      if (params.verbose)
      {
        logFile << "[ " << printDateTime(currTime) << " IN MARKET: Long " << tradeVec[i].exchNameLong << " / Short " << tradeVec[i].exchNameShort << " ]" << std::endl;
      }
      if (checkExit(&btcVec[tradeVec[i].idExchLong], &btcVec[tradeVec[i].idExchShort], tradeVec[i], params, currTime))
      {
        // An exit opportunity has been found!
        // We check the current leg1 exposure
        std::vector<double> btcUsed(realNum);
        btcUsed[tradeVec[i].idExchLong] = getActivePos[tradeVec[i].idExchLong](params, tradeVec[i].longExchTradeId);
        btcUsed[tradeVec[i].idExchShort] = getActivePos[tradeVec[i].idExchShort](params, tradeVec[i].shortExchTradeId);
        // Checks the volumes and computes the limit prices that will be sent to the exchanges
        double volumeLong = btcUsed[tradeVec[i].idExchLong];
        double volumeShort = btcUsed[tradeVec[i].idExchShort];
        double limPriceLong = getLimitPrice[tradeVec[i].idExchLong](params, volumeLong, true);
        double limPriceShort = getLimitPrice[tradeVec[i].idExchShort](params, volumeShort, false);
        if (limPriceLong == 0.0 || limPriceShort == 0.0)
        {
          logFile << "WARNING: Opportunity found but error with the order books (limit price is null). Trade canceled\n";
          logFile.precision(2);
          logFile << "         Long limit price:  " << limPriceLong << std::endl;
          logFile << "         Short limit price: " << limPriceShort << std::endl;
          tradeVec[i].trailing = 1.0;
        }
        else if (tradeVec[i].priceLongOut - limPriceLong > params.priceDeltaLim || limPriceShort - tradeVec[i].priceShortOut > params.priceDeltaLim)
        {
          logFile << "WARNING: Opportunity found but not enough liquidity. Trade canceled\n";
          logFile.precision(2);
          logFile << "         Target long price:  " << tradeVec[i].priceLongOut << ", Real long price:  " << limPriceLong << std::endl;
          logFile << "         Target short price: " << tradeVec[i].priceShortOut << ", Real short price: " << limPriceShort << std::endl;
          tradeVec[i].trailing = 1.0;
        }
        else
        {
          tradeVec[i].exitTime = currTime;
          tradeVec[i].priceLongOut = limPriceLong;
          tradeVec[i].priceShortOut = limPriceShort;
          tradeVec[i].printExitInfo(*params.logFile);

          logFile.precision(6);
          logFile << params.leg1 << " exposure on " << params.exchName[tradeVec[i].idExchLong] << ": " << volumeLong << '\n'
                  << params.leg1 << " exposure on " << params.exchName[tradeVec[i].idExchShort] << ": " << volumeShort << '\n'
                  << std::endl;
          auto longOrderId = sendLongOrder[tradeVec[i].idExchLong](params, "sell", fabs(btcUsed[tradeVec[i].idExchLong]), limPriceLong);
          auto shortOrderId = sendShortOrder[tradeVec[i].idExchShort](params, "buy", fabs(btcUsed[tradeVec[i].idExchShort]), limPriceShort);
          logFile << "Waiting for the two orders to be filled..." << std::endl;
          sleep_for(millisecs(5000));
          bool isLongOrderComplete = isOrderComplete[tradeVec[i].idExchLong](params, longOrderId);
          bool isShortOrderComplete = isOrderComplete[tradeVec[i].idExchShort](params, shortOrderId);
          // Loops until both orders are completed
          while (!isLongOrderComplete || !isShortOrderComplete)
          {
            sleep_for(millisecs(3000));
            if (!isLongOrderComplete)
            {
              logFile << "Long order on " << params.exchName[tradeVec[i].idExchLong] << " still open..." << std::endl;
              isLongOrderComplete = isOrderComplete[tradeVec[i].idExchLong](params, longOrderId);
            }
            if (!isShortOrderComplete)
            {
              logFile << "Short order on " << params.exchName[tradeVec[i].idExchShort] << " still open..." << std::endl;
              isShortOrderComplete = isOrderComplete[tradeVec[i].idExchShort](params, shortOrderId);
            }
          }
          logFile << "Done\n"
                  << std::endl;
          longOrderId = "0";
          shortOrderId = "0";
          //inMarket = false;
          for (int i = 0; i < numExch; ++i)
          {
            balance[i].leg2After = getAvail[i](params, params.leg2.c_str()); // FIXME: currency hard-coded
            balance[i].leg1After = getAvail[i](params, params.leg1.c_str()); // FIXME: currency hard-coded
          }
          for (int i = 0; i < numExch; ++i)
          {
            logFile << "New balance on " << params.exchName[i] << ":  \t";
            logFile.precision(2);
            logFile << balance[i].leg2After << " " << params.leg2 << " (perf " << balance[i].leg2After - balance[i].leg2 << "), ";
            logFile << std::setprecision(6) << balance[i].leg1After << " " << params.leg1 << "\n";
          }
          logFile << std::endl;
          // Update total leg2 balance
          for (int i = 0; i < numExch; ++i)
          {
            tradeVec[i].leg2TotBalanceBefore += balance[i].leg2;
            tradeVec[i].leg2TotBalanceAfter += balance[i].leg2After;
          }
          // Update current balances
          for (int i = 0; i < numExch; ++i)
          {
            balance[i].leg2 = balance[i].leg2After;
            balance[i].leg1 = balance[i].leg1After;
          }

          // Prints the result in the result CSV file
          logFile.precision(2);
          logFile << "ACTUAL PERFORMANCE: "
                  << "$" << tradeVec[i].leg2TotBalanceAfter - tradeVec[i].leg2TotBalanceBefore << " (" << tradeVec[i].actualPerf() * 100.0 << "%)\n"
                  << std::endl;
          csvFile << tradeVec[i].id << ","
                  << tradeVec[i].exchNameLong << ","
                  << tradeVec[i].exchNameShort << ","
                  << printDateTimeCsv(tradeVec[i].entryTime) << ","
                  << printDateTimeCsv(tradeVec[i].exitTime) << ","
                  << tradeVec[i].getTradeLengthInMinute() << ","
                  << tradeVec[i].exposure * 2.0 << ","
                  << tradeVec[i].leg2TotBalanceBefore << ","
                  << tradeVec[i].leg2TotBalanceAfter << ","
                  << tradeVec[i].actualPerf() << std::endl;
          // Sends an email with the result of the trade
          if (params.sendEmail)
          {
            // TODO: Fix
            sendEmail(tradeVec[i], params);
            logFile << "Email sent" << std::endl;
          }
          if (closeTradeInDb(tradeVec[i], params) != 0)
          {
            std::cerr << "ERROR: problems with database \'" << params.dbFile << "\'\n"
                      << std::endl;
            exit(EXIT_FAILURE);
          };
          if (addTradesToDb(tradeVec[i], params, 1) != 0)
          {
            std::cerr << "ERROR: problems with database \'" << params.dbFile << "\'\n"
                      << std::endl;
            exit(EXIT_FAILURE);
          };
          // I dont think this is correct, tradeVec[i] needs to be reset
          entryVec.push_back(tradeVec[i]);
          tradeVec.erase(tradeVec.begin() + i);
          // Removes restore.txt since this trade is done.
          std::ofstream resFile("restore.txt", std::ofstream::trunc);
          resFile.close();
        }
      }
      if (params.verbose)
        logFile << '\n';
    }
    // Moves to the next iteration, unless
    // the maxmum is reached.
    timeinfo.tm_sec += params.interval;
    currIteration++;
    if (currIteration >= params.debugMaxIteration)
    {
      logFile << "Max iteration reached (" << params.debugMaxIteration << ")" << std::endl;
      stillRunning = false;
    }
    // Exits if a 'stop_after_notrade' file is found
    // Warning: by default on GitHub the file has a underscore
    // at the end, so Blackbird is not stopped by default.
    //FIXME: broke this, could use newbool
    //std::ifstream infile("stop_after_notrade");
    //if (infile && !inMarket) {
    //  logFile << "Exit after last trade (file stop_after_notrade found)\n";
    //  stillRunning = false;
    //}
  }
  // Analysis loop exited, does some cleanup
  curl_easy_cleanup(params.curl);
  csvFile.close();
  logFile.close();

  return 0;
}
