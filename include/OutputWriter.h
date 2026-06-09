#pragma once

#include <string>
#include <vector>
#include "Types.h"

void writeVolumeProfile(const std::string& path,
                        const std::vector<VolumeProfileRow>& rows);

void writeExecutionSchedule(const std::string& path,
                            const std::vector<ExecutionScheduleRow>& rows);

void writePerformanceSummary(const std::string& path,
                             const PerformanceSummary& summary);