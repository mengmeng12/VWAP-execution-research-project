#pragma once

#include <vector>

#include "Config.h"
#include "Types.h"
#include "ExecutionStrategy.h"

/*
 * Build a strategy-specific execution schedule.
 *
 * Phase 2 design:
 * - VolumeProfile decides how many shares to trade in each bucket.
 * - ExecutionStrategy decides which trade price to use in each bucket.
 * - VWAPExecution combines both into one execution schedule.
 */
std::vector<ExecutionScheduleRow> buildExecutionSchedule(
    const Config& config,
    const std::vector<VolumeProfileRow>& volumeProfile,
    const ReducedResult& reduced,
    ExecutionStrategyType strategy,
    const StrategyConfig& strategyConfig
);

/*
 * Compatibility wrapper for the original Phase 1 buy-first schedule.
 *
 * The implementation now calls buildExecutionSchedule(... BUY_FIRST ...).
 * This keeps main.cpp simple while moving price selection into ExecutionStrategy.
 */
std::vector<ExecutionScheduleRow> buildBuyFirstSchedule(
    const Config& config,
    const std::vector<VolumeProfileRow>& volumeProfile,
    const ReducedResult& reduced
);

/*
 * Compute VWAP performance for one execution schedule.
 */
PerformanceSummary computePerformanceSummary(
    const Config& config,
    const std::vector<ExecutionScheduleRow>& schedule,
    const ReducedResult& reduced
);