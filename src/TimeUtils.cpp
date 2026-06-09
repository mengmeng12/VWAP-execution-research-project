#include "TimeUtils.h"
#include "Config.h"

#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>

/*
    Important project convention:
    - The raw timestamps are GMT.
    - May 2012 New York time = GMT - 4 hours.
    - The last bucket is 16:00:00 NY, corresponding to [20:00:00, 20:15:00) GMT.
*/


const std::vector<TimePointPair>& getTimePoints()
{
    static const std::vector<TimePointPair> timePoints = {
        TimePointPair("09:30:00", "13:30:00"),
        TimePointPair("09:45:00", "13:45:00"),
        TimePointPair("10:00:00", "14:00:00"),
        TimePointPair("10:15:00", "14:15:00"),
        TimePointPair("10:30:00", "14:30:00"),
        TimePointPair("10:45:00", "14:45:00"),
        TimePointPair("11:00:00", "15:00:00"),
        TimePointPair("11:15:00", "15:15:00"),
        TimePointPair("11:30:00", "15:30:00"),
        TimePointPair("11:45:00", "15:45:00"),
        TimePointPair("12:00:00", "16:00:00"),
        TimePointPair("12:15:00", "16:15:00"),
        TimePointPair("12:30:00", "16:30:00"),
        TimePointPair("12:45:00", "16:45:00"),
        TimePointPair("13:00:00", "17:00:00"),
        TimePointPair("13:15:00", "17:15:00"),
        TimePointPair("13:30:00", "17:30:00"),
        TimePointPair("13:45:00", "17:45:00"),
        TimePointPair("14:00:00", "18:00:00"),
        TimePointPair("14:15:00", "18:15:00"),
        TimePointPair("14:30:00", "18:30:00"),
        TimePointPair("14:45:00", "18:45:00"),
        TimePointPair("15:00:00", "19:00:00"),
        TimePointPair("15:15:00", "19:15:00"),
        TimePointPair("15:30:00", "19:30:00"),
        TimePointPair("15:45:00", "19:45:00"),
        TimePointPair("16:00:00", "20:00:00"),
        TimePointPair("16:00:00", "20:15:00")
    };

    return timePoints;
}

/*
    Remove spaces, '\r', '\n', and other leading/trailing whitespace.
    This is a small local helper, so it is not declared in TimeUtils.h.
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
    Convert a string to uppercase.
    Used for month names such as MAY, May, may.
*/
static std::string toUpperString(const std::string& value)
{
    std::string result = value;

    for (size_t i = 0; i < result.size(); i++)
    {
        result[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(result[i]))
        );
    }

    return result;
}


/*
    Convert month abbreviation to month number.

    Example:
        "JAN" -> 1
        "MAY" -> 5
        "DEC" -> 12
*/
static int monthToNumber(const std::string& monthText)
{
    std::string month = toUpperString(trimString(monthText));

    if (month == "JAN") return 1;
    if (month == "FEB") return 2;
    if (month == "MAR") return 3;
    if (month == "APR") return 4;
    if (month == "MAY") return 5;
    if (month == "JUN") return 6;
    if (month == "JUL") return 7;
    if (month == "AUG") return 8;
    if (month == "SEP") return 9;
    if (month == "OCT") return 10;
    if (month == "NOV") return 11;
    if (month == "DEC") return 12;

    return 0;
}


/*
    Convert a date string like "21-MAY-2012" into an integer:

        20120521

    This makes date comparison easier and safer than raw string comparison.
*/
static int dateToNumber(const std::string& dateText)
{
    std::string date = trimString(dateText);

    size_t dash1 = date.find('-');
    if (dash1 == std::string::npos)
    {
        return 0;
    }

    size_t dash2 = date.find('-', dash1 + 1);
    if (dash2 == std::string::npos)
    {
        return 0;
    }

    std::string dayText = date.substr(0, dash1);
    std::string monthText = date.substr(dash1 + 1, dash2 - dash1 - 1);
    std::string yearText = date.substr(dash2 + 1);

    int day = std::atoi(dayText.c_str());
    int month = monthToNumber(monthText);
    int year = std::atoi(yearText.c_str());

    if (day <= 0 || month <= 0 || year <= 0)
    {
        return 0;
    }

    return year * 10000 + month * 100 + day;
}


/*
    Strip milliseconds from a time string.

    Example:
        "13:30:00.123" -> "13:30:00"
        "13:30:00"     -> "13:30:00"
*/
std::string stripMilliseconds(const std::string& time)
{
    std::string cleanTime = trimString(time);

    size_t dotPosition = cleanTime.find('.');
    if (dotPosition != std::string::npos)
    {
        cleanTime = cleanTime.substr(0, dotPosition);
    }

    return cleanTime;
}


/*
    Check whether a GMT timestamp is inside the market processing window.

    Important:
        The end time is not included.

    For this project:
        13:30:00 <= time < 20:15:00
*/
bool isWithinMarketWindow(const std::string& gmtTime,
                          const std::string& marketStartGmt,
                          const std::string& marketEndGmt)
{
    std::string time = stripMilliseconds(gmtTime);
    std::string startTime = stripMilliseconds(marketStartGmt);
    std::string endTime = stripMilliseconds(marketEndGmt);

    if (time.empty())
    {
        return false;
    }

    return (time >= startTime && time < endTime);
}


/*
    Convert a GMT time into the NY local 15-minute bucket.

    Example:
        13:30:00 GMT -> 09:30:00 NY bucket
        13:44:59 GMT -> 09:30:00 NY bucket
        13:45:00 GMT -> 09:45:00 NY bucket
        ...
        20:00:00 GMT -> 16:00:00 NY bucket
        20:14:59 GMT -> 16:00:00 NY bucket

    The last bucket is:
        [20:00:00, 20:15:00) GMT -> 16:00:00 NY
*/
std::string convertGmtTimeToNyBucket(const std::string& gmtTime)
{
    std::string time = stripMilliseconds(gmtTime);
    const std::vector<TimePointPair>& timePoints = getTimePoints();

    if (time.empty() || timePoints.size() < 2)
    {
        return "";
    }

    /*
        Each bucket uses two adjacent GMT boundaries.

        timePoints[i].second     = bucket start GMT
        timePoints[i + 1].second = bucket end GMT

        The bucket label is timePoints[i].first, which is NY local time.
    */
    for (size_t i = 0; i + 1 < timePoints.size(); i++)
    {
        std::string bucketStartGmt = timePoints[i].second;
        std::string bucketEndGmt = timePoints[i + 1].second;

        if (time >= bucketStartGmt && time < bucketEndGmt)
        {
            return timePoints[i].first;
        }
    }

    return "";
}


/*
    Check whether a date is in the training window.

    For Phase 1:
        trainingStartDate = "01-MAY-2012"
        trainingEndDate   = "20-MAY-2012"

    This function is inclusive on both sides:
        start <= date <= end
*/
bool isTrainingDate(const std::string& date,
                    const std::string& trainingStartDate,
                    const std::string& trainingEndDate)
{
    int currentDate = dateToNumber(date);
    int startDate = dateToNumber(trainingStartDate);
    int endDate = dateToNumber(trainingEndDate);

    if (currentDate == 0 || startDate == 0 || endDate == 0)
    {
        return false;
    }

    return (currentDate >= startDate && currentDate <= endDate);
}


/*
    Check whether a date is the test execution date.

    For Phase 1:
        testDate = "21-MAY-2012"
*/
bool isTestDate(const std::string& date,
                const std::string& testDate)
{
    int currentDate = dateToNumber(date);
    int targetDate = dateToNumber(testDate);

    if (currentDate == 0 || targetDate == 0)
    {
        return false;
    }

    return (currentDate == targetDate);
}