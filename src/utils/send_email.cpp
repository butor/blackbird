#include "send_email.h"
#include "time_fun.h"
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <math.h>
#include <iomanip>
#include <unistd.h>

void sendEmail(Result &res, Parameters &params) {
    char tdStyle[] = "font-family:Georgia;font-size:11px;border-color:#A1A1A1;border-width:1px;border-style:solid;padding:2px;";
    char captionStyle[] = "font-family:Georgia;font-size:13px;font-weight:normal;color:#0021BF;padding-bottom:6px;text-align:left;";
    char tableTitleStyle[] = "font-family:Georgia;font-variant:small-caps;font-size:13px;text-align:center;border-color:#A1A1A1;border-width:1px;border-style:solid;background-color:#EAEAEA;";
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed;
    oss << "sendemail -f " << params.senderAddress << " -t " << params.receiverAddress
        << " -u \"Blackbird Bitcoin Arbitrage - Trade " << res.id << " (";
    if (res.actualPerf() >= 0) {
        oss << "+" << res.actualPerf() * 100.0;
    } else {
        oss << res.actualPerf() * 100.0;
    }
    oss << "%)\" -m \"";
    oss << "<html>";
    oss << "  <div>";
    oss << "    <br/><br/>";
    oss << "    <table style=\\\"border-width:0px;border-collapse:collapse;text-align:center;\\\">";
    oss << "      <caption style=\\\"" << captionStyle << "\\\">Blackbird Bitcoin Arbitrage - Trade " << res.id
        << "</caption>";
    oss << "      <tr style=\\\"" << tableTitleStyle << "\\\">";
    oss << "        <td style=\\\"" << tdStyle << "width:120px;\\\">Entry Date</td>";
    oss << "        <td style=\\\"" << tdStyle << "width:120px;\\\">Exit Date</td>";
    oss << "        <td style=\\\"" << tdStyle << "width:70px;\\\">Long</td>";
    oss << "        <td style=\\\"" << tdStyle << "width:70px;\\\">Short</td>";
    oss << "        <td style=\\\"" << tdStyle << "width:70px;\\\">Exposure</td>";
    oss << "        <td style=\\\"" << tdStyle << "width:70px;\\\">Profit</td>";
    oss << "        <td style=\\\"" << tdStyle << "width:70px;\\\">Return</td>";
    oss << "      </tr>";
    oss << "      <tr>";
    oss << "        <td style=\\\"" << tdStyle << "\\\">" << printDateTime(res.entryTime) << "</td>";
    oss << "        <td style=\\\"" << tdStyle << "\\\">" << printDateTime(res.exitTime) << "</td>";
    oss << "        <td style=\\\"" << tdStyle << "\\\">" << res.exchNameLong << "</td>";
    oss << "        <td style=\\\"" << tdStyle << "\\\">" << res.exchNameShort << "</td>";
    oss << "        <td style=\\\"" << tdStyle << "\\\">\\$" << res.exposure * 2.0 << "</td>";
    oss << "        <td style=\\\"" << tdStyle << "\\\">\\$" << res.usdTotBalanceAfter - res.usdTotBalanceBefore
        << "</td>";
    if (res.actualPerf() >= 0) {
        oss << "<td style=\\\"" << tdStyle << "color:#000092;\\\">+";
    } else {
        oss << "<td style=\\\"" << tdStyle << "color:#920000;\\\">";
    }
    oss << res.actualPerf() * 100.0 << "%</td></tr>";
    oss << "    </table>";
    oss << "  </div>";
    oss << "</html>\" -s " << params.smtpServerAddress << " -xu " << params.senderUsername << " -xp "
        << params.senderPassword << " -o tls=yes -o message-content-type=html >/dev/null" << std::endl;
    oss.flush();
    if (system(oss.str().c_str()) == -1) {
        *params.logFile << "<sendEmail> Error with system call" << std::endl;
    }
}

