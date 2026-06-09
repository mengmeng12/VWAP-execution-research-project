#pragma once

#include <string>
#include <vector>
#include "Config.h"
#include "Types.h"

void initializeBucketMap(std::map<std::string, BucketAggregate>& bucketMap);

MapResult runMapper(const Config& config,
                    long startPos,
                    long endPos);

ReducedResult reduceMapperResults(const Config& config,
                                  const std::vector<MapResult>& results);

ReducedResult runMapReduce(const Config& config);