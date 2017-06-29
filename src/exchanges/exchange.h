#ifndef BITCOIN_H
#define BITCOIN_H

#include "quote_t.h"
#include <string>

// Contains all the information for a given exchange, 
// like fees and wether we can short on that exchange.
// FIXME: short selling should depend on the currency.
class Exchange {
  
  private:
    unsigned id;
    std::string exchName;
    double fees;
    bool hasShort;
    bool isImplemented;
    double bid, ask;

  public:
    Exchange(unsigned id, std::string n, double f, bool h, bool m);
    void updateData(quote_t quote);
    unsigned getId() const;
    double getAsk() const;
    double getBid() const;
    double getMidPrice() const;
    std::string getExchName() const;
    double getFees() const;
    bool getHasShort() const;
    bool getIsImplemented() const;
};

#endif
