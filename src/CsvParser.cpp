#include "CsvParser.h"
#include "TimeUtils.h"

#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cctype>

/*
    The parser follows the same field logic as the original homework code:

        ... SPY, date, time, field_to_skip, field_to_skip, price, quantity, ...

    So after finding the SPY field:
        fields[symbolIndex + 1] = date
        fields[symbolIndex + 2] = GMT time
        fields[symbolIndex + 5] = trade price
        fields[symbolIndex + 6] = trade quantity
*/


/*
    Remove leading and trailing spaces.
    This is a small local helper function.
*/
static std::string trimString(const std::string& value)
{
    if (value.empty())
    {
        return "";
    }

    size_t start = 0;
    while (start < value.size() &&
           std::isspace(static_cast<unsigned char>(value[start])))
    {
        start++;
    }

    size_t end = value.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        end--;
    }

    return value.substr(start, end - start);
}


/*
    Split one CSV line by comma.
    
*/
static std::vector<std::string> splitByComma(const std::string& line)
{
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        fields.push_back(trimString(item));
    }

    return fields;
}


/*
    Check whether a string represents a positive or zero integer.

    Example:
        "100" -> true
        "0"   -> true
        ""    -> false
        "abc" -> false
*/
static bool isIntegerText(const std::string& text)
{
    if (text.empty())
    {
        return false;
    }

    for (size_t i = 0; i < text.size(); i++)
    {
        if (!std::isdigit(static_cast<unsigned char>(text[i])))
        {
            return false;
        }
    }

    return true;
}


/*
    Basic check for price text.

    We allow digits and one decimal point.
    Example:
        "132.45" -> true
        "132"    -> true
*/
static bool isDoubleText(const std::string& text)
{
    if (text.empty())
    {
        return false;
    }

    int dotCount = 0;

    for (size_t i = 0; i < text.size(); i++)
    {
        char c = text[i];

        if (c == '.')
        {
            dotCount++;

            if (dotCount > 1)
            {
                return false;
            }
        }
        else if (!std::isdigit(static_cast<unsigned char>(c)))
        {
            return false;
        }
    }

    return true;
}


/*
    Parse one line into TradeRecord.

    Return:
        true  = this line is a valid trade line for the target symbol
        false = not a trade line, not target symbol, or invalid format
*/
bool parseTradeLine(const std::string& line,
                    const std::string& symbol,
                    TradeRecord& record)
{
    /*
        First filter:
        The original homework code used strstr(buffer, "Trade").
        We keep the same idea here.
    */
    if (line.find("Trade") == std::string::npos)
    {
        return false;
    }

    std::vector<std::string> fields = splitByComma(line);

    if (fields.empty())
    {
        return false;
    }

    /*
        Find the target symbol field, for example "SPY".
    */
    int symbolIndex = -1;

    for (size_t i = 0; i < fields.size(); i++)
    {
        if (fields[i] == symbol)
        {
            symbolIndex = static_cast<int>(i);
            break;
        }
    }

    if (symbolIndex == -1)
    {
        return false;
    }

    /*
        We need at least these fields after symbol:

            symbolIndex + 1 = date
            symbolIndex + 2 = time
            symbolIndex + 5 = price
            symbolIndex + 6 = quantity
    */
    if (symbolIndex + 6 >= static_cast<int>(fields.size()))
    {
        return false;
    }

    std::string dateText = fields[symbolIndex + 1];
    std::string timeText = fields[symbolIndex + 2];
    std::string priceText = fields[symbolIndex + 5];
    std::string quantityText = fields[symbolIndex + 6];

    if (dateText.empty() || timeText.empty())
    {
        return false;
    }

    if (!isDoubleText(priceText))
    {
        return false;
    }

    if (!isIntegerText(quantityText))
    {
        return false;
    }

    double price = std::atof(priceText.c_str());
    long quantity = std::atol(quantityText.c_str());

    if (price <= 0.0 || quantity <= 0)
    {
        return false;
    }

    /*
        Fill the output record.
    */
    record.symbol = symbol;
    record.date = dateText;
    record.gmtTime = stripMilliseconds(timeText);
    record.nyBucket = convertGmtTimeToNyBucket(record.gmtTime);
    record.price = price;
    record.quantity = quantity;

    return true;
}