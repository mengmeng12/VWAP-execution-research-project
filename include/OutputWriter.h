#pragma once

#include <string>
#include <vector>

#include "Types.h"

/*
 * Write historical volume profile.
 *
 * Output:
 *   outputs/volume_profile.csv
 */
void writeVolumeProfile(
    const std::string& path,
    const std::vector<VolumeProfileRow>& rows
);

/*
 * Write one strategy-specific execution schedule.
 *
 * Output examples:
 *   outputs/execution_schedule_buy_first.csv
 *   outputs/execution_schedule_buy_tick.csv
 *   outputs/execution_schedule_sell_tick.csv
 */
void writeExecutionSchedule(
    const std::string& path,
    const std::vector<ExecutionScheduleRow>& rows
);

/*
 * Write one performance summary.
 *
 * This keeps Phase 1 compatibility:
 *   outputs/performance_summary.csv
 */
void writePerformanceSummary(
    const std::string& path,
    const PerformanceSummary& summary
);

/*
 * Write multi-strategy comparison.
 *
 * Phase 2 output:
 *   outputs/strategy_comparison.csv
 */
void writeStrategyComparison(
    const std::string& path,
    const std::vector<PerformanceSummary>& summaries
);