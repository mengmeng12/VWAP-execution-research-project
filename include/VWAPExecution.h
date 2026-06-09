#pragma once

#include <vector>
#include "Config.h"
#include "Types.h"

std::vector<ExecutionScheduleRow> buildBuyFirstSchedule(
    const Config& config,
    const std::vector<VolumeProfileRow>& volumeProfile
);

PerformanceSummary computePerformanceSummary(
    const Config& config,
    const std::vector<ExecutionScheduleRow>& schedule,
    const ReducedResult& reduced
);