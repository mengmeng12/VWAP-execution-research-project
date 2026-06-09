#include "OutputWriter.h"
#include "Types.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>


/*
    This file writes the final outputs to CSV files.

        1. volume_profile.csv
        2. execution_schedule_buy_first.csv
        3. performance_summary.csv
*/


/*
    Write historical volume profile.

    Columns:
        bucket
        avg_qty
        relative_volume
        execution_price
*/
void writeVolumeProfile(const std::string& path,
                        const std::vector<VolumeProfileRow>& rows)
{
    std::ofstream output(path.c_str());

    if (!output.is_open())
    {
        std::cerr << "Error: cannot open output file: "
                  << path
                  << std::endl;
        return;
    }

    output << "bucket,avg_qty,relative_volume,execution_price\n";

    output << std::fixed << std::setprecision(8);

    for (size_t i = 0; i < rows.size(); i++)
    {
        const VolumeProfileRow& row = rows[i];

        output << row.bucket << ","
               << row.avgQty << ","
               << row.relativeVolume << ","
               << row.executionPrice
               << "\n";
    }

    output.close();

    std::cout << "Wrote volume profile to: "
              << path
              << std::endl;
}


/*
    Write baseline buy-first execution schedule.

    Columns:
        bucket
        avg_qty
        relative_volume
        order_size
        execution_price
        executed_value
*/
void writeExecutionSchedule(const std::string& path,
                            const std::vector<ExecutionScheduleRow>& rows)
{
    std::ofstream output(path.c_str());

    if (!output.is_open())
    {
        std::cerr << "Error: cannot open output file: "
                  << path
                  << std::endl;
        return;
    }

    output << "bucket,avg_qty,relative_volume,order_size,execution_price,executed_value\n";

    output << std::fixed << std::setprecision(8);

    for (size_t i = 0; i < rows.size(); i++)
    {
        const ExecutionScheduleRow& row = rows[i];

        output << row.bucket << ","
               << row.avgQty << ","
               << row.relativeVolume << ","
               << row.orderSize << ","
               << row.executionPrice << ","
               << row.executedValue
               << "\n";
    }

    output.close();

    std::cout << "Wrote execution schedule to: "
              << path
              << std::endl;
}


/*
    Write performance summary.

    Columns:
        target_shares
        my_vwap
        market_vwap
        difference
        difference_bps

    Buy order interpretation:
        difference_bps < 0 means MyVWAP is lower than MarketVWAP,
        so the simulated buy execution is better than market VWAP.
*/
void writePerformanceSummary(const std::string& path,
                             const PerformanceSummary& summary)
{
    std::ofstream output(path.c_str());

    if (!output.is_open())
    {
        std::cerr << "Error: cannot open output file: "
                  << path
                  << std::endl;
        return;
    }

    output << "target_shares,my_vwap,market_vwap,difference,difference_bps,result\n";

    output << std::fixed << std::setprecision(8);

    std::string resultText;

    if (summary.myVWAP <= 0.0 || summary.marketVWAP <= 0.0)
    {
        resultText = "invalid";
    }
    else if (summary.myVWAP < summary.marketVWAP)
    {
        resultText = "better_than_market_vwap_for_buy";
    }
    else if (summary.myVWAP > summary.marketVWAP)
    {
        resultText = "worse_than_market_vwap_for_buy";
    }
    else
    {
        resultText = "equal_to_market_vwap";
    }

    output << summary.targetShares << ","
           << summary.myVWAP << ","
           << summary.marketVWAP << ","
           << summary.difference << ","
           << summary.differenceBps << ","
           << resultText
           << "\n";

    output.close();

    std::cout << "Wrote performance summary to: "
              << path
              << std::endl;
}