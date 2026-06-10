#include "MapReduceProcessor.h"
#include "Config.h"
#include "Types.h"
#include "CsvParser.h"
#include "TimeUtils.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <algorithm>


/*
    Initialize all 15-minute buckets.

    We create one BucketAggregate for each NY local bucket:

        09:30:00
        09:45:00
        ...
        15:45:00
        16:00:00

    The final 16:00:00 bucket corresponds to:
        20:00:00 <= GMT time < 20:15:00
*/
void initializeBucketMap(std::map<std::string, BucketAggregate>& bucketMap)
{
    bucketMap.clear();

    const std::vector<TimePointPair>& timePoints = getTimePoints();

    /*
        We use i + 1 < timePoints.size() because each bucket needs
        a start boundary and an end boundary.

        Example:
            timePoints[i].second     = 13:30:00
            timePoints[i + 1].second = 13:45:00

        The bucket label is:
            timePoints[i].first      = 09:30:00
    */
    for (size_t i = 0; i + 1 < timePoints.size(); i++)
    {
        std::string bucketName = timePoints[i].first;
        bucketMap[bucketName] = BucketAggregate();
    }
}
/*
 * Compare two bucket trades by GMT time.
 *
 * The raw file is split across mappers, so after reducer merges trades from
 * different mapper chunks, we sort trades inside each bucket again.
 */
static bool compareBucketTradeByTime(const BucketTrade& a, const BucketTrade& b) {
    return a.gmtTime < b.gmtTime;
}

/*
    Get file size in bytes.

    This is used to split the large CSV file into NUM_MAPPERS chunks.
*/
static long getFileSize(const std::string& fileName)
{
    std::ifstream input(fileName.c_str(), std::ios::binary | std::ios::ate);

    if (!input.is_open())
    {
        return -1;
    }

    std::streampos fileSize = input.tellg();
    input.close();

    return static_cast<long>(fileSize);
}


/*
    Move file stream to the mapper start position.

    Important issue:
    If a mapper starts in the middle of a line, we should skip that partial line.
    Otherwise, the parser may read a broken CSV line.

    But if startPos is exactly at the beginning of a line, we should NOT skip it.

    This helper checks the previous character:
        - If previous char is '\n', startPos is already a line start.
        - Otherwise, skip one partial line.
*/
static void moveToSafeStartPosition(std::ifstream& input, long startPos)
{
    if (startPos == 0)
    {
        input.seekg(0);
        return;
    }

    input.seekg(startPos - 1);

    char previousChar;
    input.get(previousChar);

    input.seekg(startPos);

    if (previousChar != '\n')
    {
        std::string dummyLine;
        std::getline(input, dummyLine);
    }
}


/*
    Process one file chunk.

    Each mapper reads one part of the large CSV file and produces a MapResult.

    For training dates 5/1/2012 - 5/20/2012:
        - Add trade quantity into the corresponding 15-minute bucket.
        - Record the trading date.

    For test date 5/21/2012:
        - Record the first trade price in each bucket as execution price.
        - Accumulate market notional and market quantity for Market VWAP.
*/
MapResult runMapper(const Config& config,
                    long startPos,
                    long endPos)
{
    MapResult result;
    initializeBucketMap(result.bucketMap);

    std::ifstream input(config.inputFile.c_str(), std::ios::binary);

    if (!input.is_open())
    {
        result.rc = 1;
        result.errorMessage = "Cannot open input file: " + config.inputFile;
        return result;
    }

    moveToSafeStartPosition(input, startPos);

    std::string line;

    while (true)
    {
        std::streampos currentPosition = input.tellg();

        if (currentPosition == std::streampos(-1))
        {
            break;
        }

        /*
            Stop this mapper if the next line starts outside its chunk.

            The current line may still extend past endPos, and that is OK.
            We assign a line to the mapper based on where the line starts.
        */
        if (static_cast<long>(currentPosition) >= endPos)
        {
            break;
        }

        if (!std::getline(input, line))
        {
            break;
        }

        TradeRecord trade;

        if (!parseTradeLine(line, config.symbol, trade))
        {
            continue;
        }

        /*
            Only process trades inside the configured GMT market window.

            For this project:
                13:30:00 <= GMT time < 20:15:00
        */
        if (!isWithinMarketWindow(trade.gmtTime,
                                  config.marketStartGmt,
                                  config.marketEndGmt))
        {
            continue;
        }

        if (trade.nyBucket.empty())
        {
            continue;
        }

        /*
            Training period:
                01-MAY-2012 to 20-MAY-2012

            We use these dates to build the historical volume profile.
        */
        if (isTrainingDate(trade.date,
                           config.trainingStartDate,
                           config.trainingEndDate))
        {
            result.bucketMap[trade.nyBucket].historicalQty += trade.quantity;
            result.trainingDates.insert(trade.date);
        }

        /*
            Test date:
                21-MAY-2012

            We use this date to simulate the VWAP execution.
        */
        else if (isTestDate(trade.date, config.testDate)) 
        {
            BucketAggregate& bucket = result.bucketMap[trade.nyBucket];
            /*
            * Phase 2:
            * Store every test-date trade inside this bucket.
            *
            * Later, ExecutionStrategy will use bucket.testTrades to choose
            * strategy-specific execution prices.
            */
            BucketTrade bucketTrade;
            bucketTrade.gmtTime = trade.gmtTime;
            bucketTrade.price = trade.price;
            bucketTrade.quantity = trade.quantity;

            bucket.testTrades.push_back(bucketTrade);

            /*
            * Phase 1 buy-first baseline:
            * Keep this for now so the old Phase 1 path still works.
            *
            * Later, buy-first will move into ExecutionStrategy.
            */
            if (bucket.executionGmtTime.empty() || trade.gmtTime < bucket.executionGmtTime) 
            {
                bucket.executionGmtTime = trade.gmtTime;
                bucket.executionPrice = trade.price;
            }

            /*
            * Market VWAP components:
            *
            * MarketVWAP = sum(price * quantity) / sum(quantity)
            */
            double tradeNotional = trade.price * static_cast<double>(trade.quantity);

            bucket.marketQty += trade.quantity;
            bucket.marketNotional += tradeNotional;

            result.marketQtyTotal += trade.quantity;
            result.marketNotionalTotal += tradeNotional;
        }
    }

    input.close();

    return result;
}


/*
    Reduce all mapper outputs into one ReducedResult.

    Reducer responsibilities:
        1. Add historical quantities across mappers.
        2. Merge all training dates.
        3. Choose earliest 5/21 execution price for each bucket.
        4. Add market VWAP notional and quantity.
*/
ReducedResult reduceMapperResults(const Config& config,
                                  const std::vector<MapResult>& results)
{
    ReducedResult reduced;
    initializeBucketMap(reduced.bucketMap);

    for (size_t i = 0; i < results.size(); i++)
    {
        const MapResult& mapperResult = results[i];

        if (mapperResult.rc != 0)
        {
            std::cerr << "Warning: mapper " << i
                      << " returned error: "
                      << mapperResult.errorMessage << std::endl;
            continue;
        }

        /*
            Merge training dates.
        */
        for (std::set<std::string>::const_iterator it =
                 mapperResult.trainingDates.begin();
             it != mapperResult.trainingDates.end();
             ++it)
        {
            reduced.trainingDates.insert(*it);
        }

        /*
            Merge bucket-level data.
        */
        for (std::map<std::string, BucketAggregate>::const_iterator it =
                 mapperResult.bucketMap.begin();
             it != mapperResult.bucketMap.end();
             ++it)
        {
            std::string bucketName = it->first;
            const BucketAggregate& mapperBucket = it->second;

            BucketAggregate& reducedBucket = reduced.bucketMap[bucketName];

            /*
                Historical quantity for volume profile.
            */
            reducedBucket.historicalQty += mapperBucket.historicalQty;

            /*
                Market VWAP bucket-level components.
            */
            reducedBucket.marketQty += mapperBucket.marketQty;
            reducedBucket.marketNotional += mapperBucket.marketNotional;

            /*
            * Phase 2:
            * Merge test-date trades from this mapper into the reduced bucket.
            *
            * We sort them after all mapper results have been merged.
            */
            for (size_t j = 0; j < mapperBucket.testTrades.size(); j++) {
                reducedBucket.testTrades.push_back(mapperBucket.testTrades[j]);
            }

            /*
                Execution price:

                Keep the earliest 5/21 trade price inside each bucket.
            */
            if (!mapperBucket.executionGmtTime.empty())
            {
                if (reducedBucket.executionGmtTime.empty() ||
                    mapperBucket.executionGmtTime < reducedBucket.executionGmtTime)
                {
                    reducedBucket.executionGmtTime = mapperBucket.executionGmtTime;
                    reducedBucket.executionPrice = mapperBucket.executionPrice;
                }
            }
        }

        /*
            Market VWAP total components.
        */
        reduced.marketQtyTotal += mapperResult.marketQtyTotal;
        reduced.marketNotionalTotal += mapperResult.marketNotionalTotal;
    }
    /*
    * Phase 2:
    * Sort test-date trades inside each bucket.
    *
    * This is important because mapper chunks are processed independently.
    * After reducer merges them, the vector order is not guaranteed.
    */

    for (std::map<std::string, BucketAggregate>::iterator it = reduced.bucketMap.begin();
        it != reduced.bucketMap.end();
        ++it) {
        std::vector<BucketTrade>& trades = it->second.testTrades;
        std::sort(trades.begin(), trades.end(), compareBucketTradeByTime);
    }
    return reduced;
}


/*
    Run the full MapReduce-style process.

    Steps:
        1. Get input file size.
        2. Split file into config.numMappers chunks.
        3. Launch one thread per mapper.
        4. Join all threads.
        5. Reduce mapper results.
*/
ReducedResult runMapReduce(const Config& config)
{
    long fileSize = getFileSize(config.inputFile);

    if (fileSize <= 0)
    {
        std::cerr << "Error: invalid input file size." << std::endl;
        std::cerr << "Input file: " << config.inputFile << std::endl;

        ReducedResult emptyResult;
        initializeBucketMap(emptyResult.bucketMap);
        return emptyResult;
    }

    int numMappers = config.numMappers;

    if (numMappers <= 0)
    {
        numMappers = 1;
    }

    std::cout << "Input file: " << config.inputFile << std::endl;
    std::cout << "File size: " << fileSize << " bytes" << std::endl;
    std::cout << "Number of mappers: " << numMappers << std::endl;

    std::vector<MapResult> mapperResults(numMappers);
    std::vector<std::thread> threads;

    long chunkSize = fileSize / numMappers;

    for (int i = 0; i < numMappers; i++)
    {
        long startPos = i * chunkSize;
        long endPos;

        if (i == numMappers - 1)
        {
            endPos = fileSize;
        }
        else
        {
            endPos = (i + 1) * chunkSize;
        }

        std::cout << "Mapper " << i
                  << " start=" << startPos
                  << " end=" << endPos
                  << std::endl;

        threads.push_back(
            std::thread(
                [&mapperResults, &config, i, startPos, endPos]()
                {
                    mapperResults[i] = runMapper(config, startPos, endPos);
                }
            )
        );
    }

    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }

    std::cout << "All mapper threads finished." << std::endl;

    ReducedResult reduced = reduceMapperResults(config, mapperResults);

    std::cout << "Reducer finished." << std::endl;
    std::cout << "Training days found: "
              << reduced.trainingDates.size()
              << std::endl;

    std::cout << "Market quantity on test date: "
              << reduced.marketQtyTotal
              << std::endl;

    std::cout << "Market notional on test date: "
              << reduced.marketNotionalTotal
              << std::endl;

    return reduced;
}