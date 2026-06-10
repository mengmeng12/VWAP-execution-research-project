#include "OutputWriter.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "Types.h"

/*
 * Helper:
 * Use the result already computed in VWAPExecution.
 * If it is empty, create a simple fallback interpretation.
 */
static std::string getResultText(const PerformanceSummary& summary) {
    if (!summary.result.empty()) {
        return summary.result;
    }

    if (summary.myVWAP <= 0.0 || summary.marketVWAP <= 0.0) {
        return "invalid_vwap";
    }

    if (summary.myVWAP < summary.marketVWAP) {
        return "better_than_market_vwap_for_buy_order";
    }

    if (summary.myVWAP > summary.marketVWAP) {
        return "worse_than_market_vwap_for_buy_order";
    }

    return "equal_to_market_vwap_for_buy_order";
}

/*
 * Write historical volume profile.
 *
 * Columns:
 *   bucket
 *   avg_qty
 *   relative_volume
 *   execution_price
 *
 * Note:
 *   execution_price is kept for Phase 1 compatibility.
 *   In Phase 2, strategy-specific execution prices are written in
 *   execution_schedule_*.csv instead.
 */
void writeVolumeProfile(
    const std::string& path,
    const std::vector<VolumeProfileRow>& rows
) {
    std::ofstream output(path.c_str());

    if (!output.is_open()) {
        std::cerr << "Error: cannot open output file: " << path << std::endl;
        return;
    }

    output << "bucket,avg_qty,relative_volume,execution_price\n";
    output << std::fixed << std::setprecision(8);

    for (size_t i = 0; i < rows.size(); i++) {
        const VolumeProfileRow& row = rows[i];

        output
            << row.bucket << ","
            << row.avgQty << ","
            << row.relativeVolume << ","
            << row.executionPrice
            << "\n";
    }

    output.close();

    std::cout << "Wrote volume profile to: " << path << std::endl;
}

/*
 * Write one strategy-specific execution schedule.
 *
 * Required Phase 2 columns:
 *   bucket
 *   avg_qty
 *   relative_volume
 *   order_size
 *   execution_price
 *   executed_value
 *
 * Extra debug columns:
 *   strategy
 *   execution_gmt_time
 *   decision_reason
 *
 * These extra columns make it easier to check why buy_tick or sell_tick
 * selected a different price from buy_first.
 */
void writeExecutionSchedule(
    const std::string& path,
    const std::vector<ExecutionScheduleRow>& rows
) {
    std::ofstream output(path.c_str());

    if (!output.is_open()) {
        std::cerr << "Error: cannot open output file: " << path << std::endl;
        return;
    }

    output
        << "strategy,"
        << "bucket,"
        << "avg_qty,"
        << "relative_volume,"
        << "order_size,"
        << "execution_gmt_time,"
        << "execution_price,"
        << "executed_value,"
        << "decision_reason\n";

    output << std::fixed << std::setprecision(8);

    for (size_t i = 0; i < rows.size(); i++) {
        const ExecutionScheduleRow& row = rows[i];

        output
            << row.strategy << ","
            << row.bucket << ","
            << row.avgQty << ","
            << row.relativeVolume << ","
            << row.orderSize << ","
            << row.executionGmtTime << ","
            << row.executionPrice << ","
            << row.executedValue << ","
            << row.decisionReason
            << "\n";
    }

    output.close();

    std::cout << "Wrote execution schedule to: " << path << std::endl;
}

/*
 * Write one performance summary.
 *
 * This keeps the old Phase 1 output path working:
 *   outputs/performance_summary.csv
 *
 * Phase 2 will additionally write:
 *   outputs/strategy_comparison.csv
 */
void writePerformanceSummary(
    const std::string& path,
    const PerformanceSummary& summary
) {
    std::ofstream output(path.c_str());

    if (!output.is_open()) {
        std::cerr << "Error: cannot open output file: " << path << std::endl;
        return;
    }

    output
        << "strategy,"
        << "target_shares,"
        << "my_vwap,"
        << "market_vwap,"
        << "difference,"
        << "difference_bps,"
        << "result\n";

    output << std::fixed << std::setprecision(8);

    output
        << summary.strategy << ","
        << summary.targetShares << ","
        << summary.myVWAP << ","
        << summary.marketVWAP << ","
        << summary.difference << ","
        << summary.differenceBps << ","
        << getResultText(summary)
        << "\n";

    output.close();

    std::cout << "Wrote performance summary to: " << path << std::endl;
}

/*
 * Write Phase 2 multi-strategy comparison.
 *
 * Columns:
 *   strategy
 *   my_vwap
 *   market_vwap
 *   difference
 *   difference_bps
 *   result
 */
void writeStrategyComparison(
    const std::string& path,
    const std::vector<PerformanceSummary>& summaries
) {
    std::ofstream output(path.c_str());

    if (!output.is_open()) {
        std::cerr << "Error: cannot open output file: " << path << std::endl;
        return;
    }

    output
        << "strategy,"
        << "my_vwap,"
        << "market_vwap,"
        << "difference,"
        << "difference_bps,"
        << "result\n";

    output << std::fixed << std::setprecision(8);

    for (size_t i = 0; i < summaries.size(); i++) {
        const PerformanceSummary& summary = summaries[i];

        output
            << summary.strategy << ","
            << summary.myVWAP << ","
            << summary.marketVWAP << ","
            << summary.difference << ","
            << summary.differenceBps << ","
            << getResultText(summary)
            << "\n";
    }

    output.close();

    std::cout << "Wrote strategy comparison to: " << path << std::endl;
}