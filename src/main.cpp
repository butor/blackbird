#include "bitcoin.h"
#include "result.h"
#include "time_fun.h"
#include "curl_fun.h"
#include "db_fun.h"
#include "parameters.h"
#include "check_entry_exit.h"
#include "exchanges/bitfinex.h"
#include "exchanges/okcoin.h"
#include "exchanges/bitstamp.h"
#include "exchanges/gemini.h"
#include "exchanges/kraken.h"
#include "exchanges/quadrigacx.h"
#include "exchanges/itbit.h"
#include "exchanges/btce.h"
#include "exchanges/poloniex.h"
#include "exchanges/gdax.h"
#include "exchanges/exmo.h"
#include "exchanges/cexio.h"
#include "exchanges/bittrex.h"
#include "exchanges/binance.h"

#include "exchanges/nulllongexch.h"
#include "exchanges/nullshortexch.h"

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
typedef quote_t (*getQuoteType) (Parameters& params);
typedef double (*getAvailType) (Parameters& params, std::string currency);
typedef std::string (*sendOrderType) (Parameters& params, std::string direction, double quantity, double price);
typedef bool (*isOrderCompleteType) (Parameters& params, std::string orderId);
typedef double (*getActivePosType) (Parameters& params, std::string orderId);
typedef double (*getLimitPriceType) (Parameters& params, double volume, bool isBid);


// This structure contains the balance of both exchanges,
// *before* and *after* an arbitrage trade.
// This is used to compute the performance of the trade,
// by comparing the balance before and after the trade.
struct Balance {
  double leg1, leg2;
  double leg1After, leg2After;
};


// 'main' function.
// Blackbird doesn't require any arguments for now.
int main(int argc, char** argv) {
  std::cout << "Blackbird Bitcoin Arbitrage" << std::endl;
  std::cout << "DISCLAIMER: USE THE SOFTWARE AT YOUR OWN RISK\n" << std::endl;
  // Replaces the C++ global locale with the user-preferred locale
  std::locale mylocale("");
  // Loads all the parameters
  Parameters params("blackbird.conf");
  // Does some verifications about the parameters
  if (!params.demoMode) {
    if (!params.useFullExposure) {
      if (params.testedExposure < 10.0 && params.leg2.compare("USD") == 0) {
        // TODO do the same check for other currencies. Is there a limi?
        std::cout << "ERROR: Minimum USD needed: $10.00" << std::endl;
        std::cout << "       Otherwise some exchanges will reject the orders\n" << std::endl;
        exit(EXIT_FAILURE);
      }
      if (params.testedExposure > params.maxExposure) {
        std::cout << "ERROR: Test exposure (" << params.testedExposure << ") is above max exposure (" << params.maxExposure << ")\n" << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }

// Connects to the SQLite3 database.
// This database is used to collect bid and ask information
// from the exchanges. Not really used for the moment, but
// would be useful to collect historical bid/ask data.
  if (createDbConnection(params) != 0) {
    std::cerr << "ERROR: cannot connect to the database \'" << params.dbFile << "\'\n" << std::endl;
    exit(EXIT_FAILURE);
  }
  // Creates the trade table for trade results.
  if (createTradeTable(params) != 0)
  {
    std::cerr << "ERROR: problems with database \'" << params.dbFile << "\'\n"
              << std::endl;
    exit(EXIT_FAILURE);
  };

  // We only trade BTC/USD for the moment
  if (params.leg1.compare("BTC") != 0 || params.leg2.compare("USD") != 0) {
    std::cout << "ERROR: Valid currency pair is only BTC/USD for now.\n" << std::endl;
//    exit(EXIT_FAILURE);
  }

  // Function arrays containing all the exchanges functions
  // using the 'typedef' declarations from above.

  getQuoteType getQuote[15];
  getAvailType getAvail[15];
  sendOrderType sendLongOrder[15];
  sendOrderType sendShortOrder[15];
  isOrderCompleteType isOrderComplete[15];
  getActivePosType getActivePos[15];
  getLimitPriceType getLimitPrice[15];
  std::string dbTableName[15];



  // Adds the exchange functions to the arrays for all the defined exchanges
  int index = 0;
  if (params.bitfinexEnable &&
     (params.bitfinexApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Bitfinex", params.bitfinexFees, true, true);
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
  if (params.okcoinEnable &&
     (params.okcoinApi.empty() == false || params.demoMode == true)) {
    params.addExchange("OKCoin", params.okcoinFees, false, true);
    getQuote[index] = OKCoin::getQuote;
    getAvail[index] = OKCoin::getAvail;
    sendLongOrder[index] = OKCoin::sendLongOrder;
    sendShortOrder[index] = OKCoin::sendShortOrder;
    isOrderComplete[index] = OKCoin::isOrderComplete;
    getActivePos[index] = OKCoin::getActivePos;
    getLimitPrice[index] = OKCoin::getLimitPrice;

    dbTableName[index] = "okcoin";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.bitstampEnable &&
     (params.bitstampClientId.empty() == false || params.demoMode == true)) {
    params.addExchange("Bitstamp", params.bitstampFees, false, true);
    getQuote[index] = Bitstamp::getQuote;
    getAvail[index] = Bitstamp::getAvail;
    sendLongOrder[index] = Bitstamp::sendLongOrder;
    isOrderComplete[index] = Bitstamp::isOrderComplete;
    getActivePos[index] = Bitstamp::getActivePos;
    getLimitPrice[index] = Bitstamp::getLimitPrice;

    dbTableName[index] = "bitstamp";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.geminiEnable &&
     (params.geminiApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Gemini", params.geminiFees, false, true);
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
     (params.krakenApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Kraken", params.krakenFees, true, true);
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
  if (params.itbitEnable &&
     (params.itbitApi.empty() == false || params.demoMode == true)) {
    params.addExchange("ItBit", params.itbitFees, false, false);
    getQuote[index] = ItBit::getQuote;
    getAvail[index] = ItBit::getAvail;
    getActivePos[index] = ItBit::getActivePos;
    getLimitPrice[index] = ItBit::getLimitPrice;

    dbTableName[index] = "itbit";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.btceEnable &&
     (params.btceApi.empty() == false || params.demoMode == true)) {
    params.addExchange("BTC-e", params.btceFees, false, true);
    getQuote[index] = BTCe::getQuote;
    getAvail[index] = BTCe::getAvail;
    sendLongOrder[index] = BTCe::sendLongOrder;
    isOrderComplete[index] = BTCe::isOrderComplete;
    getActivePos[index] = BTCe::getActivePos;
    getLimitPrice[index] = BTCe::getLimitPrice;

    dbTableName[index] = "btce";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.poloniexEnable &&
     (params.poloniexApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Poloniex", params.poloniexFees, false, true);
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
  if (params.gdaxEnable &&
     (params.gdaxApi.empty() == false || params.demoMode == true)) {
    params.addExchange("GDAX", params.gdaxFees, false, true);
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
  if (params.quadrigaEnable &&
         (params.quadrigaApi.empty() == false || params.demoMode == true)) {
    params.addExchange("QuadrigaCX", params.quadrigaFees, false, true);
    getQuote[index] = QuadrigaCX::getQuote;
    getAvail[index] = QuadrigaCX::getAvail;
    sendLongOrder[index] = QuadrigaCX::sendLongOrder;
    isOrderComplete[index] = QuadrigaCX::isOrderComplete;
    getActivePos[index] = QuadrigaCX::getActivePos;
    getLimitPrice[index] = QuadrigaCX::getLimitPrice;

    dbTableName[index] = "quadriga";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.exmoEnable &&
         (params.exmoApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Exmo", params.exmoFees, false, true);
    getQuote[index] = Exmo::getQuote;
    getAvail[index] = Exmo::getAvail;
    sendLongOrder[index] = Exmo::sendLongOrder;
    isOrderComplete[index] = Exmo::isOrderComplete;
    getActivePos[index] = Exmo::getActivePos;
    getLimitPrice[index] = Exmo::getLimitPrice;

    dbTableName[index] = "exmo";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.cexioEnable &&
         (params.cexioApi.empty() == false || params.demoMode == true)) {
    params.addExchange("Cexio", params.cexioFees, false, true);
    getQuote[index] = Cexio::getQuote;
    getAvail[index] = Cexio::getAvail;
    sendLongOrder[index] = Cexio::sendLongOrder;
    sendShortOrder[index] = Cexio::sendShortOrder;
    isOrderComplete[index] = Cexio::isOrderComplete;
    getActivePos[index] = Cexio::getActivePos;
    getLimitPrice[index] = Cexio::getLimitPrice;

    dbTableName[index] = "cexio";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.bittrexEnable &&
      (params.bittrexApi.empty() == false || params.demoMode == true))
  {
    params.addExchange("Bittrex", params.bittrexFees, false, true);
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
  if (params.binanceEnable &&
      (params.binanceApi.empty() == false || params.demoMode == true))
  {
    params.addExchange("Binance", params.binanceFees, false, true);
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

  if (params.nullLongExchEnable)
  {
    params.addExchange("NullLongExch", params.nullLongExchFees, false, true);
    getQuote[index] = NullLongExch::getQuote;
    getAvail[index] = NullLongExch::getAvail;
    sendLongOrder[index] = NullLongExch::sendLongOrder;
    isOrderComplete[index] = NullLongExch::isOrderComplete;
    getActivePos[index] = NullLongExch::getActivePos;
    getLimitPrice[index] = NullLongExch::getLimitPrice;
    dbTableName[index] = "NullLongExch";

    index++;
  }
  if (params.nullShortExchEnable)
  {
    params.addExchange("NullShortExch", params.nullShortExchFees, true, true);
    getQuote[index] = NullShortExch::getQuote;
    getAvail[index] = NullShortExch::getAvail;
    sendLongOrder[index] = NullShortExch::sendLongOrder;
    sendShortOrder[index] = NullShortExch::sendShortOrder;
    isOrderComplete[index] = NullShortExch::isOrderComplete;
    getActivePos[index] = NullShortExch::getActivePos;
    getLimitPrice[index] = NullShortExch::getLimitPrice;
    dbTableName[index] = "NullShortExch";

    index++;
  }

  // We need at least two exchanges to run Blackbird
  if (index < 2) {
    std::cout << "ERROR: Blackbird needs at least two Bitcoin exchanges. Please edit the config.json file to add new exchanges\n" << std::endl;
    exit(EXIT_FAILURE);
  }
  // Creates the CSV file that will collect the trade results
  std::string currDateTime = printDateTimeFileName();
  std::string csvFileName = "output/blackbird_result_" + currDateTime + ".csv";
  std::ofstream csvFile(csvFileName, std::ofstream::trunc);
  csvFile << "TRADE_ID,EXCHANGE_LONG,EXHANGE_SHORT,ENTRY_TIME,EXIT_TIME,DURATION,"
          << "TOTAL_EXPOSURE,BALANCE_BEFORE,BALANCE_AFTER,RETURN"
          << std::endl;
  // Creates the log file where all events will be saved
  std::string logFileName = "output/blackbird_log_" + currDateTime + ".log";
  std::ofstream logFile(logFileName, std::ofstream::trunc);
  logFile.imbue(mylocale);
  logFile.precision(2);
  logFile << std::fixed;
  params.logFile = &logFile;
  // Log file header
  logFile << "--------------------------------------------" << std::endl;
  logFile << "|   Blackbird Bitcoin Arbitrage Log File   |" << std::endl;
  logFile << "--------------------------------------------\n" << std::endl;
  logFile << "Blackbird started on " << printDateTime() << "\n" << std::endl;

  logFile << "Connected to database \'" << params.dbFile << "\'\n" << std::endl;

  if (params.demoMode) {
    logFile << "Demo mode: trades won't be generated\n" << std::endl;
  }

  // Shows which pair we are trading (BTC/USD only for the moment)
  logFile << "Pair traded: " << params.leg1 << "/" << params.leg2 << "\n" << std::endl;

  std::cout << "Log file generated: " << logFileName << "\nBlackbird is running... (pid " << getpid() << ")\n" << std::endl;
  int numExch = params.nbExch();
  int comboExch = 0;
  // The btcVec vector contains details about every exchange,
  // like fees, as specified in bitcoin.h
  std::vector<Bitcoin> btcVec;
  btcVec.reserve(numExch);
  // Creates a new Bitcoin structure within btcVec for every exchange we want to trade on
  for (int i = 0; i < numExch; ++i) {
    btcVec.push_back(Bitcoin(i, params.exchName[i], params.fees[i], params.canShort[i], params.isImplemented[i]));
    if (params.canShort[i]){
      comboExch +=1;
    }
  }
  //creates the number of possible trade combos -- not really used but useful
  int realNum = (comboExch -1) * comboExch + (numExch - comboExch) * comboExch;
  // Inits cURL connections
  params.curl = curl_easy_init();
  // Shows the spreads
  logFile << "[ Targets ]\n"
          << std::setprecision(2)
          << "   Spread Entry:  " << params.spreadEntry * 100.0 << "%\n"
          << "   Spread Target: " << params.spreadTarget * 100.0  << "%\n";

  // SpreadEntry and SpreadTarget have to be positive,
  // Otherwise we will loose money on every trade.
  if (params.spreadEntry <= 0.0) {
    logFile << "   WARNING: Spread Entry should be positive" << std::endl;
  }
  if (params.spreadTarget <= 0.0) {
    logFile << "   WARNING: Spread Target should be positive" << std::endl;
  }
  logFile << std::endl;
  logFile << "[ Current balances ]" << std::endl;
  // Gets the the balances from every exchange
  // This is only done when not in Demo mode.
  // This is the first curl on exchanges
  std::vector<Balance> balance(numExch);
  if (!params.demoMode)
    std::transform(getAvail, getAvail + numExch,
                   begin(balance),
                   [&params]( decltype(*getAvail) apply )
                   {
                     Balance tmp {};
                     tmp.leg1 = apply(params, params.leg1.c_str()); 
                     tmp.leg2 = apply(params, params.leg2.c_str()); 
                     return tmp;
                   } );

  // Checks for a restore.txt file, to see if
  // the program exited with an open position.
  //Result m;
  //m.reset();

  //creates vectors to store entry opportunities and in market positions
  std::vector<Result> entryVec;
  entryVec.reserve(realNum);
  //entryVec.reserve(realNum);
  std::vector<Result> tradeVec;
  tradeVec.reserve(realNum);
  // Creates blank entryVec entries for every possible combination
  for (int i = 0; i < numExch; i++)
  {
    for (int j = 0; j < numExch; j++)
    {
      if (i != j)
      {
        if (btcVec[j].getHasShort())
        {
          Result tmp {};
          // all of the combinations will be loaded into vectors with exchIds
          tmp.idExchLong = i;
          tmp.idExchShort = j;
          entryVec.push_back(tmp);
        }
      }
    }
  }

  // grabs the number of trades outstanding
  int numTrades = getNumTradesOutstanding(params);
  if (realNum < numTrades)
  {
    std::cout << "Error: You have more outstanding positions than possible." << std::endl;
    std::cout << "You may have enabled/disabled exchanges since last run, Check your Db/exchanges." << std::endl;
    exit(EXIT_FAILURE);
  }

  // Get trades that were left open last session
  getTradesFromDb(params, tradeVec);

  // this is to remove the outstanding trades exchange pairs (i.e. Long GDAX short Kraken) from EntryVec
  //FIXME: there are better ways to do this for sure...
  if (!tradeVec.empty()){
    for (size_t i = 0; i< tradeVec.size(); i++){
      Result tmp {};
      tmp.removePair(entryVec,tradeVec[i].idExchLong);
    }
  }

  // Writes the current balances into the log file
  for (int i = 0; i < numExch; ++i) {
    logFile << "   " << params.exchName[i] << ":\t";
    if (params.demoMode) {
      logFile << "n/a (demo mode)" << std::endl;
    } else if (!params.isImplemented[i]) {
      logFile << "n/a (API not implemented)" << std::endl;
    } else {
      logFile << std::setprecision(2) << balance[i].leg2 << " " << params.leg2 << "\t"
              << std::setprecision(6) << balance[i].leg1 << " " << params.leg1 << std::endl;
    }
    // FIXME: I do not use this safety -- does it have an effect?
    if (balance[i].leg1 > 0.0050 && tradeVec.empty()) { // FIXME: hard-coded number
      logFile << "ERROR: All " << params.leg1 << " accounts must be empty before starting Blackbird" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  logFile << std::endl;
  logFile << "[ Cash exposure ]\n";
  if (params.demoMode) {
    logFile << "   No cash - Demo mode\n";
  } else {
    if (params.useFullExposure) {
      logFile << "   FULL exposure used!\n";
    } else {
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
  using secs      = std::chrono::seconds;
  // Waits for the next 'interval' seconds before starting the loop
  while ((int)timeinfo.tm_sec % params.interval != 0) {
    sleep_for(millisecs(100));
    time(&rawtime);
    timeinfo = *localtime(&rawtime);
  }
  if (!params.verbose) {
    logFile << "Running..." << std::endl;
  }

  int resultId = 0;
  unsigned currIteration = 0;
  bool stillRunning = true;
  time_t currTime;
  time_t diffTime;
  
  // Main analysis loop
  while (stillRunning) {
    currTime = mktime(&timeinfo);
    time(&rawtime);
    diffTime = difftime(rawtime, currTime);
    // Checks if we are already too late in the current iteration
    // If that's the case we wait until the next iteration
    // and we show a warning in the log file.
    if (diffTime > 0) {
      logFile << "WARNING: " << diffTime << " second(s) too late at " << printDateTime(currTime) << std::endl;
      timeinfo.tm_sec += (ceil(diffTime / params.interval) + 1) * params.interval;
      currTime = mktime(&timeinfo);
      sleep_for(secs(params.interval - (diffTime % params.interval)));
      logFile << std::endl;
    } else if (diffTime < 0) {
      sleep_for(secs(-diffTime));
    }
    // Header for every iteration of the loop
    if (params.verbose)
    {
      logFile << "[ " << printDateTime(currTime) << " ]" << std::endl;
    }
    // Gets the bid and ask of all the exchanges
    for (int i = 0; i < numExch; ++i) {
      auto quote = getQuote[i](params);
      double bid = quote.bid();
      double ask = quote.ask();

      // Saves the bid/ask into the SQLite database
      addBidAskToDb(dbTableName[i], printDateTimeDb(currTime), bid, ask, params);

      // If there is an error with the bid or ask (i.e. value is null),
      // we show a warning but we don't stop the loop.
      if (bid == 0.0) {
        logFile << "   WARNING: " << params.exchName[i] << " bid is null" << std::endl;
      }
      if (ask == 0.0) {
        logFile << "   WARNING: " << params.exchName[i] << " ask is null" << std::endl;
      }
      // Shows the bid/ask information in the log file
      if (params.verbose) {
        logFile << "   " << params.exchName[i] << ": \t"
                << std::setprecision(2)
                << bid << " / " << ask << std::endl;
      }
      // Updates the Bitcoin vector with the latest bid/ask data
      btcVec[i].updateData(quote);
      curl_easy_reset(params.curl);
    }
    if (params.verbose) {
      logFile << "   ----------------------------" << std::endl;
    }
    // Stores all the spreads in arrays to
    // compute the volatility. The volatility
    // is not used for the moment.
    if (params.useVolatility)
    {
      for (size_t i = 0; i < entryVec.size(); i++)
      {
        double longMidPrice = btcVec[entryVec[i].idExchLong].getMidPrice();
        double shortMidPrice = btcVec[entryVec[i].idExchShort].getMidPrice();
        if (longMidPrice > 0.0 && shortMidPrice > 0.0)
        {
          if (entryVec[i].volatility.size() >= params.volatilityPeriod)
          {
            entryVec[i].volatility.pop_back();
          }
          entryVec[i].volatility.push_front((longMidPrice - shortMidPrice) / longMidPrice);
        }
      }
      for (size_t i = 0; i < tradeVec.size(); i++)
      {
        double longMidPrice = btcVec[tradeVec[i].idExchLong].getMidPrice();
        double shortMidPrice = btcVec[tradeVec[i].idExchShort].getMidPrice();
        if (longMidPrice > 0.0 && shortMidPrice > 0.0)
        {
          if (tradeVec[i].volatility.size() >= params.volatilityPeriod)
          {
            tradeVec[i].volatility.pop_back();
          }
          tradeVec[i].volatility.push_front((longMidPrice - shortMidPrice) / longMidPrice);
        }
      }
    }
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
        if (params.useFullExposure == false && entryVec[i].exposure <= params.testedExposure)
        {
          logFile << "WARNING: Opportunity found but not enough cash. Need more than TEST cash (min. $"
                  << std::setprecision(2) << params.testedExposure << "). Trade canceled" << std::endl;
          break;
        }
        if (entryVec[i].exposure < 10){
          logFile << "WARNING: Opportunity found but not enough cash. Need more than $10" << std::endl;
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
        // We store the details of that first trade into the entryVec (Result Structure).
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
        // add the leg2TotalBalance to the entryVec for export to db
        // we already know it so no curl needed
        entryVec[i].leg2TotBalanceBefore = balance[entryVec[i].idExchShort].leg2 + balance[entryVec[i].idExchLong].leg2;
        // updates the balances of the executed exchanges
        balance[entryVec[i].idExchLong].leg1 = getAvail[entryVec[i].idExchLong](params, params.leg1.c_str()); // Technically we know this and dont need to curl
        balance[entryVec[i].idExchLong].leg2 = getAvail[entryVec[i].idExchLong](params, params.leg2.c_str()); // FIXME: currency hard-coded
        balance[entryVec[i].idExchShort].leg2 = getAvail[entryVec[i].idExchShort](params, params.leg2.c_str()); // FIXME: currency hard-coded
        balance[entryVec[i].idExchShort].leg1 = getAvail[entryVec[i].idExchShort](params, params.leg1.c_str()); // Technically we know this and dont need to curl
        // Adds the tradeIds to the entryVec for export to DB
        entryVec[i].longExchTradeId = longOrderId;
        entryVec[i].shortExchTradeId = shortOrderId;
        // Stores the partial result to db file in case
        // the program exits before closing the position.
        if (addTradesToDb(entryVec[i], params, 0) != 0)
        {
          std::cerr << "ERROR: problems with database \'" << params.dbFile << "\'\n"
                    << std::endl;
          exit(EXIT_FAILURE);
        };
        //push the entered position into the tradeVec
        tradeVec.push_back(entryVec[i]);
        //erase the pairing from the entryVec
        entryVec.erase(entryVec.begin() + i);
        //reset the orderIds
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
        std::vector<double> btcUsed(numExch);
        // use getActivePos to query exchanges with our old tradeId
        // ensures we only sell the amount of <currency> we were executed for
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
          //we calculate the new balances, we really already knew idExchLong leg1 and idExchShort leg2 exposure (as they are the same as volume)
          balance[tradeVec[i].idExchLong].leg1After = getAvail[tradeVec[i].idExchLong](params,params.leg1.c_str());
          balance[tradeVec[i].idExchLong].leg2After = getAvail[tradeVec[i].idExchLong](params, params.leg2.c_str());
          balance[tradeVec[i].idExchShort].leg1After = getAvail[tradeVec[i].idExchShort](params, params.leg1.c_str());
          balance[tradeVec[i].idExchShort].leg2After = getAvail[tradeVec[i].idExchShort](params, params.leg2.c_str());
  
          // this is broken and wrong
          logFile << "New balance on " << tradeVec[i].exchNameLong << ":  \t";
          logFile.precision(2);
          logFile << balance[tradeVec[i].idExchLong].leg2After << " " << params.leg2 << " (perf " 
          << balance[tradeVec[i].idExchLong].leg2After - balance[tradeVec[i].idExchLong].leg2 << "), ";
          logFile << std::setprecision(6) 
          << balance[tradeVec[i].idExchLong].leg1After << " " << params.leg1 << "\n";

          logFile << "New balance on " << tradeVec[i].exchNameShort << ": \t";
          logFile.precision(2);
          logFile << balance[tradeVec[i].idExchShort].leg2After << " " << params.leg2 << " (perf "
          << balance[tradeVec[i].idExchShort].leg2After - balance[tradeVec[i].idExchShort].leg2 << "), ";
          logFile << std::setprecision(6)
          << balance[tradeVec[i].idExchShort].leg1After << " " << params.leg1 << "\n";
          logFile << std:: endl;
          /*old way
          for (int i = 0; i < numExch; ++i)
          {
            balance[i].leg2After = getAvail[i](params, params.leg2.c_str()); // FIXME: currency hard-coded
            balance[i].leg1After = getAvail[i](params, params.leg1.c_str()); // FIXME: currency hard-coded
          }*/
          /* old way
          for (int i = 0; i < numExch; ++i)
          {
            logFile << "New balance on " << params.exchName[i] << ":  \t";
            logFile.precision(2);
            logFile << balance[i].leg2After << " " << params.leg2 << " (perf " << balance[i].leg2After - balance[i].leg2 << "), ";
            logFile << std::setprecision(6) << balance[i].leg1After << " " << params.leg1 << "\n";
          }
          logFile << std::endl; */
          // Update total leg2 balance
          //tradeVec[i].leg2TotBalanceBefore += balance[tradeVec[i].idExchShort]
          // update the tradeVec for the balance after the trade for performance
          tradeVec[i].leg2TotBalanceAfter = balance[tradeVec[i].idExchLong].leg2After + balance[tradeVec[i].idExchShort].leg2After;
          // update currency balances
          balance[tradeVec[i].idExchLong].leg1 = balance[tradeVec[i].idExchLong].leg1After;
          balance[tradeVec[i].idExchLong].leg2 = balance[tradeVec[i].idExchLong].leg2After;
          balance[tradeVec[i].idExchShort].leg1 = balance[tradeVec[i].idExchShort].leg1After;
          balance[tradeVec[i].idExchShort].leg2 = balance[tradeVec[i].idExchShort].leg2After;
          /*for (int i = 0; i < numExch; ++i)
          {
            tradeVec[i].leg2TotBalanceBefore += balance[i].leg2;
            tradeVec[i].leg2TotBalanceAfter += balance[i].leg2After;
          }*/
          // Update current balances
          /*for (int i = 0; i < numExch; ++i)
          {
            balance[i].leg2 = balance[i].leg2After;
            balance[i].leg1 = balance[i].leg1After;
          }*/

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
          // Add the new order ids to the tradeVec
          tradeVec[i].longExchTradeId = longOrderId;
          tradeVec[i].shortExchTradeId = shortOrderId;
          // Add the closing trade to the DB
          if (addTradesToDb(tradeVec[i], params, 1) != 0)
          {
            std::cerr << "ERROR: problems with database \'" << params.dbFile << "\'\n"
                      << std::endl;
            exit(EXIT_FAILURE);
          };
          // reset orderIds
          longOrderId = "0";
          shortOrderId = "0";
          // I dont think this is correct, tradeVec[i] may need to be reset
          entryVec.push_back(tradeVec[i]);
          tradeVec.erase(tradeVec.begin() + i);
        }
      }
      if (params.verbose)
        logFile << '\n';
    }
    // Moves to the next iteration, unless
    // the maxmum is reached.
    timeinfo.tm_sec += params.interval;
    currIteration++;
    if (currIteration >= params.debugMaxIteration) {
      logFile << "Max iteration reached (" << params.debugMaxIteration << ")" <<std::endl;
      stillRunning = false;
    }
    // Exits if a 'stop_after_notrade' file is found
    // Warning: by default on GitHub the file has a underscore
    // at the end, so Blackbird is not stopped by default.
    //FIXME: this is brokenish, could make a bool
    std::ifstream infile("stop_after_notrade");
    if (infile && tradeVec.empty()) {
      logFile << "Exit after last trade (file stop_after_notrade found)\n";
      stillRunning = false;
    }
  }
  // Analysis loop exited, does some cleanup
  curl_easy_cleanup(params.curl);
  csvFile.close();
  logFile.close();

  return 0;
}
