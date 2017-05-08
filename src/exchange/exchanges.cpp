#include "exchanges.h"

#include "exchange/bitfinex.h"
#include "exchange/okcoin.h"
#include "exchange/bitstamp.h"
#include "exchange/gemini.h"
#include "exchange/kraken.h"
#include "exchange/itbit.h"
#include "exchange/btce.h"
#include "exchange/poloniex.h"
#include "exchange/gdax.h"

getQuoteType getQuote[10];
getAvailType getAvail[10];
sendOrderType sendLongOrder[10];
sendOrderType sendShortOrder[10];
isOrderCompleteType isOrderComplete[10];
getActivePosType getActivePos[10];
getLimitPriceType getLimitPrice[10];
std::string dbTableName[10];

int initExchanges(Parameters& params)
{
  int index = 0;
  if (params.bitfinexApi.empty() == false || params.demoMode == true) {
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
  if (params.okcoinApi.empty() == false || params.demoMode == true) {
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
  if (params.bitstampClientId.empty() == false || params.demoMode == true) {
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
  if (params.geminiApi.empty() == false || params.demoMode == true) {
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
  if (params.krakenApi.empty() == false || params.demoMode == true) {
    params.addExchange("Kraken", params.krakenFees, false, true);
    getQuote[index] = Kraken::getQuote;
    getAvail[index] = Kraken::getAvail;
    sendLongOrder[index] = Kraken::sendLongOrder;
    isOrderComplete[index] = Kraken::isOrderComplete;
    getActivePos[index] = Kraken::getActivePos;
    getLimitPrice[index] = Kraken::getLimitPrice;

    dbTableName[index] = "kraken";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.itbitApi.empty() == false || params.demoMode == true) {
    params.addExchange("ItBit", params.itbitFees, false, false);
    getQuote[index] = ItBit::getQuote;
    getAvail[index] = ItBit::getAvail;
    getActivePos[index] = ItBit::getActivePos;
    getLimitPrice[index] = ItBit::getLimitPrice;

    dbTableName[index] = "itbit";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.btceApi.empty() == false || params.demoMode == true) {
    params.addExchange("BTC-e", params.btceFees, false, false);
    getQuote[index] = BTCe::getQuote;
    getAvail[index] = BTCe::getAvail;
    getActivePos[index] = BTCe::getActivePos;
    getLimitPrice[index] = BTCe::getLimitPrice;

    dbTableName[index] = "btce";
    createTable(dbTableName[index], params);

    index++;
  }
  if (params.poloniexApi.empty() == false || params.demoMode == true) {
    params.addExchange("Poloniex", params.poloniexFees, true, false);
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
  if (params.gdaxApi.empty() == false || params.demoMode == true) {
    params.addExchange("GDAX", params.gdaxFees, false, false);
    getQuote[index] = GDAX::getQuote;
    getAvail[index] = GDAX::getAvail;
    getActivePos[index] = GDAX::getActivePos;
    getLimitPrice[index] = GDAX::getLimitPrice;

    dbTableName[index] = "gdax";
    createTable(dbTableName[index], params);

    index++;
  }

  return index;
}