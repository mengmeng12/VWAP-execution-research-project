#pragma once

#include <string>
#include "Types.h"

bool parseTradeLine(const std::string& line,
                    const std::string& symbol,
                    TradeRecord& record);