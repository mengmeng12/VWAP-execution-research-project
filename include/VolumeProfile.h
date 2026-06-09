#pragma once

#include <vector>
#include "Config.h"
#include "Types.h"

std::vector<VolumeProfileRow> buildVolumeProfile(const Config& config,
                                                 const ReducedResult& reduced);