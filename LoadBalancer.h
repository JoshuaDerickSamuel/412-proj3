#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "RequestQueue.h"
#include "WebServer.h"

/**
 * @file LoadBalancer.h
 * @brief Core load balancer declarations: config, statistics, and runtime behavior.
 */

/**
 * @struct LoadBalancerStats
 * @brief Summary counters collected during simulation.
 */
struct LoadBalancerStats {
    long long initialCreated = 0;
    long long generated = 0;
    long long accepted = 0;
    long long blocked = 0;
    long long assigned = 0;
    long long completed = 0;
    long long addedServers = 0;
    long long removedServers = 0;
    long long peakQueue = 0;
};

/** @brief Console event colors used by the logger callback. */
enum class EventColor {
    Default,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
};

/**
 * @struct LoadBalancerConfig
 * @brief Tunable values that control queue behavior, request generation, and scaling.
 */
struct LoadBalancerConfig {
    int initialQueueMultiplier = 100;
    int minRequestTime = 5;
    int maxRequestTime = 200;
    int scaleLowMultiplier = 50;
    int scaleHighMultiplier = 80;
    int scaleCooldownCycles = 30;
    double processingJobProbability = 0.6;
    std::vector<std::string> blockedCidrs;
};

/**
 * @typedef LoadBalancerLogFn
 * @brief Callback signature used for event logging.
 */
using LoadBalancerLogFn = std::function<void(int, const std::string&, EventColor)>;

/**
 * @class LoadBalancer
 * @brief Owns request queue, server pool, dispatch policy, and scaling decisions.
 *
 * The load balancer receives requests, filters blocked IPs, dispatches queued
 * jobs to idle servers, and scales server count according to queue thresholds.
 */
class LoadBalancer {
public:
    /** @brief Constructs a named load balancer instance with initial servers. */
    LoadBalancer(std::string balancerName, int initialServers, const LoadBalancerConfig& settings, LoadBalancerLogFn loggerFn);

    /** @brief Generates one random request payload. */
    /**
     * @brief Creates one randomized request.
     * @param clock Current simulation cycle.
     * @return Newly created request object.
     */
    Request createRandomRequest(int clock);

    /** @brief Submits one request to this load balancer. */
    /**
     * @brief Submits a request to the balancer queue if not blocked.
     * @param request Request to enqueue.
     * @param clock Current simulation cycle.
     * @return True if accepted; false if blocked.
     */
    bool submit(const Request& request, int clock);

    /** @brief Seeds the initial queue for startup load. */
    /**
     * @brief Seeds startup queue with initial request volume.
     * @param clock Current simulation cycle.
     */
    void seedInitialQueue(int clock);

    /** @brief Runs one simulation cycle for this load balancer. */
    /**
     * @brief Executes one full simulation cycle.
     * @param clock Current simulation cycle.
     */
    void tick(int clock);

    /** @brief Returns current request queue size. */
    /** @brief Current queue depth. */
    std::size_t queueSize() const;

    /** @brief Returns current number of active servers. */
    /** @brief Current active server count. */
    std::size_t serverCount() const;

    /** @brief Returns simulation counters. */
    /** @brief Read-only statistics snapshot. */
    const LoadBalancerStats& stats() const;

    /** @brief Returns display name for logs and summary output. */
    /** @brief Human-readable balancer name. */
    const std::string& getName() const;

private:
    std::string lbName;
    RequestQueue queue;
    std::vector<std::unique_ptr<WebServer>> servers;
    LoadBalancerConfig config;
    LoadBalancerLogFn logEvent;
    LoadBalancerStats counters;

    int nextRequestId;
    int nextServerId;
    int cooldownUntil;
    bool seedingPhase;

    std::mt19937 rng;
    std::uniform_int_distribution<int> ipOctet;
    std::uniform_int_distribution<int> timeDist;
    std::uniform_real_distribution<double> typeDist;

    std::vector<std::pair<uint32_t, uint32_t>> blockedRanges;

    static uint32_t ipToInt(const std::string& ip);
    static std::string randomIp(std::mt19937& engine, std::uniform_int_distribution<int>& octetDist);
    static std::optional<std::pair<uint32_t, uint32_t>> parseCidr(const std::string& cidr);

    bool isBlocked(const std::string& ip) const;

    void processServers(int clock);
    void dispatchRequests(int clock);
    void rebalanceServers(int clock);

    void addServer(int clock);
    void removeServer(int clock);
};

#endif
