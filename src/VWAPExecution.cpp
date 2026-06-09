#include "VWAPExecution.h"
#include "Config.h"
#include "Types.h"

#include <iostream>
#include <vector>


/*
    Build the baseline buy-first execution schedule.

    Strategy:
        For each 15-minute bucket:
            - Use historical relative volume to decide order size.
            - Use the first trade price on 21-MAY-2012 as execution price.

    Formula:
        OrderSize_i = TargetShares * RelativeVolume_i
        ExecutedValue_i = OrderSize_i * ExecutionPrice_i
*/


std::vector<ExecutionScheduleRow> buildBuyFirstSchedule(
    const Config& config,
    const std::vector<VolumeProfileRow>& volumeProfile
)
{
    std::vector<ExecutionScheduleRow> schedule;

    if (volumeProfile.empty())
    {
        std::cerr << "Warning: volume profile is empty." << std::endl;
        return schedule;
    }

    double totalOrderSize = 0.0;
    double totalExecutedValue = 0.0;

    for (size_t i = 0; i < volumeProfile.size(); i++)
    {
        const VolumeProfileRow& profileRow = volumeProfile[i];

        ExecutionScheduleRow scheduleRow;

        scheduleRow.bucket = profileRow.bucket;
        scheduleRow.avgQty = profileRow.avgQty;
        scheduleRow.relativeVolume = profileRow.relativeVolume;

        /*
            Allocate the target order according to historical relative volume.

            Example:
                targetShares = 50000
                relativeVolume = 0.04

                orderSize = 50000 * 0.04 = 2000 shares
        */
        scheduleRow.orderSize =
            static_cast<double>(config.targetShares) *
            profileRow.relativeVolume;

        /*
            Buy-first baseline execution price.

            This price comes from the first trade in the same bucket
            on the test date 21-MAY-2012.
        */
        scheduleRow.executionPrice = profileRow.executionPrice;

        /*
            Executed notional value for this bucket.
        */
        scheduleRow.executedValue =
            scheduleRow.orderSize *
            scheduleRow.executionPrice;

        schedule.push_back(scheduleRow);

        totalOrderSize += scheduleRow.orderSize;
        totalExecutedValue += scheduleRow.executedValue;
    }

    std::cout << "Buy-first execution schedule built." << std::endl;
    std::cout << "Target shares: " << config.targetShares << std::endl;
    std::cout << "Total scheduled order size: " << totalOrderSize << std::endl;
    std::cout << "Total scheduled executed value: " << totalExecutedValue << std::endl;

    return schedule;
}


/*
    Compute the performance summary.

    MyVWAP:
        This is the VWAP of our simulated execution schedule.

        MyVWAP =
            sum(OrderSize_i * ExecutionPrice_i) / sum(OrderSize_i)

    MarketVWAP:
        This is the actual market VWAP on 21-MAY-2012 using all trade messages
        inside the same market window.

        MarketVWAP =
            sum(MarketTradePrice_j * MarketTradeQty_j) / sum(MarketTradeQty_j)

    Difference:
        MyVWAP - MarketVWAP

    DifferenceBps:
        (MyVWAP / MarketVWAP - 1.0) * 10000

    Buy order interpretation:
        If MyVWAP < MarketVWAP, our execution is better than market VWAP.
        If MyVWAP > MarketVWAP, our execution is worse than market VWAP.
*/


PerformanceSummary computePerformanceSummary(
    const Config& config,
    const std::vector<ExecutionScheduleRow>& schedule,
    const ReducedResult& reduced
)
{
    PerformanceSummary summary;

    summary.targetShares = config.targetShares;

    if (schedule.empty())
    {
        std::cerr << "Warning: execution schedule is empty." << std::endl;
        return summary;
    }

    double totalOrderSize = 0.0;
    double totalExecutedValue = 0.0;

    for (size_t i = 0; i < schedule.size(); i++)
    {
        const ExecutionScheduleRow& row = schedule[i];

        /*
            Skip rows with invalid order size or price.

            In a clean run, all buckets should have a positive order size
            and a valid execution price.
        */
        if (row.orderSize <= 0.0)
        {
            continue;
        }

        if (row.executionPrice <= 0.0)
        {
            std::cerr << "Warning: missing execution price for bucket "
                      << row.bucket << std::endl;
            continue;
        }

        totalOrderSize += row.orderSize;
        totalExecutedValue += row.executedValue;
    }

    if (totalOrderSize > 0.0)
    {
        summary.myVWAP = totalExecutedValue / totalOrderSize;
    }
    else
    {
        std::cerr << "Warning: total order size is zero." << std::endl;
        summary.myVWAP = 0.0;
    }

    if (reduced.marketQtyTotal > 0)
    {
        summary.marketVWAP =
            reduced.marketNotionalTotal /
            static_cast<double>(reduced.marketQtyTotal);
    }
    else
    {
        std::cerr << "Warning: market quantity total is zero." << std::endl;
        summary.marketVWAP = 0.0;
    }

    if (summary.marketVWAP > 0.0)
    {
        summary.difference = summary.myVWAP - summary.marketVWAP;

        summary.differenceBps =
            (summary.myVWAP / summary.marketVWAP - 1.0) * 10000.0;
    }
    else
    {
        summary.difference = 0.0;
        summary.differenceBps = 0.0;
    }

    std::cout << "Performance summary computed." << std::endl;
    std::cout << "My VWAP: " << summary.myVWAP << std::endl;
    std::cout << "Market VWAP: " << summary.marketVWAP << std::endl;
    std::cout << "Difference: " << summary.difference << std::endl;
    std::cout << "Difference bps: " << summary.differenceBps << std::endl;

    if (summary.myVWAP > 0.0 && summary.marketVWAP > 0.0)
    {
        if (summary.myVWAP < summary.marketVWAP)
        {
            std::cout << "Buy execution result: better than market VWAP."
                      << std::endl;
        }
        else if (summary.myVWAP > summary.marketVWAP)
        {
            std::cout << "Buy execution result: worse than market VWAP."
                      << std::endl;
        }
        else
        {
            std::cout << "Buy execution result: equal to market VWAP."
                      << std::endl;
        }
    }

    return summary;
}