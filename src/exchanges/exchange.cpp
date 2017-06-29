#include "exchange.h"
#include <cmath>

Exchange::Exchange(unsigned i, std::string n, double f, bool h, bool m) {
  id = i;
  exchName = n;
  fees = f;
  hasShort = h;
  isImplemented = m;
  bid = 0.0;
  ask = 0.0;
}

void Exchange::updateData(quote_t quote) {
  bid = quote.bid();
  ask = quote.ask();
}

unsigned Exchange::getId() const { return id; }

double Exchange::getBid()  const { return bid; }

double Exchange::getAsk()  const { return ask; }

double Exchange::getMidPrice() const {
  if (bid > 0.0 && ask > 0.0) {
    return (bid + ask) / 2.0;
  } else {
    return 0.0;
  }
}

std::string Exchange::getExchName()  const { return exchName; }

double Exchange::getFees()           const { return fees; }

bool Exchange::getHasShort()         const { return hasShort; }

bool Exchange::getIsImplemented()    const { return isImplemented; }
