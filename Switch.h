#ifndef SWITCH_H
#define SWITCH_H

#include "LoadBalancer.h"

/**
 * @file Switch.h
 * @brief Optional request router for split load-balancer mode.
 */

/**
 * @class Switch
 * @brief Routes incoming requests to streaming or processing balancer by job type.
 */
class Switch {
public:
    /** @brief Constructs switch with references to both target balancers. */
    Switch(LoadBalancer& streamingBalancer, LoadBalancer& processingBalancer);

    /**
     * @brief Routes one request based on `jobType`.
     * @param request Incoming request.
     * @param clock Current simulation cycle.
     * @return True if accepted by the target balancer.
     */
    bool route(const Request& request, int clock);

private:
    LoadBalancer& streaming;
    LoadBalancer& processing;
};

#endif
