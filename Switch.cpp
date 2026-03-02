#include "Switch.h"

/**
 * @file Switch.cpp
 * @brief Implements job-type based routing between multiple load balancers.
 */

Switch::Switch(LoadBalancer& streamingBalancer, LoadBalancer& processingBalancer)
    : streaming(streamingBalancer), processing(processingBalancer) {}

bool Switch::route(const Request& request, int clock) {
    if (request.jobType == 'S') {
        return streaming.submit(request, clock);
    }

    return processing.submit(request, clock);
}
