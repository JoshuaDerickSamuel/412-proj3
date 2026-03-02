#include "LoadBalancer.h"

#include <algorithm>
#include <sstream>

/**
 * @file LoadBalancer.cpp
 * @brief Implements queue dispatch, firewall filtering, and dynamic scaling logic.
 */

LoadBalancer::LoadBalancer(std::string balancerName, int initialServers, const LoadBalancerConfig& settings, LoadBalancerLogFn loggerFn)
    : lbName(std::move(balancerName)),
    config(settings),
    logEvent(std::move(loggerFn)),
      nextRequestId(1),
      nextServerId(1),
      cooldownUntil(0),
    seedingPhase(false),
      rng(std::random_device{}()),
      ipOctet(1, 254),
      timeDist(std::max(1, config.minRequestTime), std::max(config.minRequestTime, config.maxRequestTime)),
      typeDist(0.0, 1.0) {
    const int startServerCount = std::max(1, initialServers);
    for (int i = 0; i < startServerCount; ++i) {
        servers.push_back(std::make_unique<WebServer>(nextServerId++));
    }

    for (const std::string& cidr : config.blockedCidrs) {
        auto cidrRange = parseCidr(cidr);
        if (cidrRange.has_value()) {
            blockedRanges.push_back(*cidrRange);
        }
    }
}

Request LoadBalancer::createRandomRequest(int clock) {
    if (seedingPhase) {
        ++counters.initialCreated;
    }

    const char type = (typeDist(rng) < config.processingJobProbability) ? 'P' : 'S';
    return Request(
        nextRequestId++,
        randomIp(rng, ipOctet),
        randomIp(rng, ipOctet),
        timeDist(rng),
        type,
        clock
    );
}

bool LoadBalancer::submit(const Request& request, int clock) {
    if (!seedingPhase) {
        ++counters.generated;
    }

    if (isBlocked(request.ipIn)) {
        ++counters.blocked;
        if (logEvent) {
            logEvent(clock, lbName + " blocked request #" + std::to_string(request.id) + " from " + request.ipIn, EventColor::Red);
        }
        return false;
    }

    queue.enqueue(request);
    ++counters.accepted;
    counters.peakQueue = std::max(counters.peakQueue, static_cast<long long>(queue.size()));
    return true;
}

void LoadBalancer::seedInitialQueue(int clock) {
    const int target = std::max(1, config.initialQueueMultiplier) * static_cast<int>(servers.size());
    seedingPhase = true;
    for (int i = 0; i < target; ++i) {
        submit(createRandomRequest(clock), clock);
    }
    seedingPhase = false;

    if (logEvent) {
        logEvent(clock,
                 lbName + " initial queue seeded | created=" + std::to_string(counters.initialCreated) +
                     " accepted=" + std::to_string(queue.size()),
                 EventColor::Cyan);
    }
}

void LoadBalancer::tick(int clock) {
    processServers(clock);
    dispatchRequests(clock);
    rebalanceServers(clock);
    counters.peakQueue = std::max(counters.peakQueue, static_cast<long long>(queue.size()));
}

std::size_t LoadBalancer::queueSize() const {
    return queue.size();
}

std::size_t LoadBalancer::serverCount() const {
    return servers.size();
}

const LoadBalancerStats& LoadBalancer::stats() const {
    return counters;
}

const std::string& LoadBalancer::getName() const {
    return lbName;
}

uint32_t LoadBalancer::ipToInt(const std::string& ip) {
    unsigned int a, b, c, d;
    char dot1, dot2, dot3;
    std::stringstream ss(ip);
    ss >> a >> dot1 >> b >> dot2 >> c >> dot3 >> d;

    if (ss.fail() || dot1 != '.' || dot2 != '.' || dot3 != '.') {
        return 0;
    }

    return (a << 24U) | (b << 16U) | (c << 8U) | d;
}

std::string LoadBalancer::randomIp(std::mt19937& engine, std::uniform_int_distribution<int>& octetDist) {
    return std::to_string(octetDist(engine)) + "." +
           std::to_string(octetDist(engine)) + "." +
           std::to_string(octetDist(engine)) + "." +
           std::to_string(octetDist(engine));
}

std::optional<std::pair<uint32_t, uint32_t>> LoadBalancer::parseCidr(const std::string& cidr) {
    const std::size_t slash = cidr.find('/');
    if (slash == std::string::npos) {
        return std::nullopt;
    }

    const std::string baseIp = cidr.substr(0, slash);
    int prefix = -1;
    try {
        prefix = std::stoi(cidr.substr(slash + 1));
    } catch (...) {
        return std::nullopt;
    }

    if (prefix < 0 || prefix > 32) {
        return std::nullopt;
    }

    const uint32_t ip = ipToInt(baseIp);
    const uint32_t mask = (prefix == 0) ? 0 : (0xFFFFFFFFU << (32 - prefix));
    const uint32_t start = ip & mask;
    const uint32_t end = start | (~mask);
    return std::make_pair(start, end);
}

bool LoadBalancer::isBlocked(const std::string& ip) const {
    const uint32_t ipValue = ipToInt(ip);
    for (const auto& range : blockedRanges) {
        if (ipValue >= range.first && ipValue <= range.second) {
            return true;
        }
    }

    return false;
}

void LoadBalancer::processServers(int clock) {
    for (auto& server : servers) {
        Request completed;
        if (server->tick(completed)) {
            ++counters.completed;
            if (logEvent) {
                logEvent(clock,
                         lbName + " server #" + std::to_string(server->getId()) +
                             " completed request #" + std::to_string(completed.id) +
                             " (type " + completed.jobType + ")",
                         EventColor::Green);
            }
        }
    }
}

void LoadBalancer::dispatchRequests(int clock) {
    for (auto& server : servers) {
        if (queue.empty()) {
            break;
        }

        if (!server->isIdle()) {
            continue;
        }

        Request request = queue.dequeue();
        if (server->assign(request)) {
            ++counters.assigned;
            if (logEvent) {
                logEvent(clock,
                         lbName + " assigned request #" + std::to_string(request.id) +
                             " to server #" + std::to_string(server->getId()) +
                             " (" + request.jobType + ", " + std::to_string(request.timeRequired) + " cycles)",
                         EventColor::Blue);
            }
        }
    }
}

void LoadBalancer::rebalanceServers(int clock) {
    if (clock < cooldownUntil) {
        return;
    }

    const int lowThreshold = config.scaleLowMultiplier * static_cast<int>(servers.size());
    const int highThreshold = config.scaleHighMultiplier * static_cast<int>(servers.size());
    const int queuedNow = static_cast<int>(queue.size());

    if (queuedNow > highThreshold) {
        addServer(clock);
        cooldownUntil = clock + std::max(1, config.scaleCooldownCycles);
        return;
    }

    if (queuedNow < lowThreshold && servers.size() > 1) {
        removeServer(clock);
        cooldownUntil = clock + std::max(1, config.scaleCooldownCycles);
    }
}

void LoadBalancer::addServer(int clock) {
    servers.push_back(std::make_unique<WebServer>(nextServerId++));
    ++counters.addedServers;
    if (logEvent) {
        logEvent(clock,
                 lbName + " scaled up: queue=" + std::to_string(queue.size()) +
                     " servers=" + std::to_string(servers.size()),
                 EventColor::Yellow);
    }
}

void LoadBalancer::removeServer(int clock) {
    auto idleServerIt = std::find_if(servers.rbegin(), servers.rend(), [](const std::unique_ptr<WebServer>& server) {
        return server->isIdle();
    });

    if (idleServerIt == servers.rend()) {
        return;
    }

    servers.erase(std::next(idleServerIt).base());
    ++counters.removedServers;
    if (logEvent) {
        logEvent(clock,
                 lbName + " scaled down: queue=" + std::to_string(queue.size()) +
                     " servers=" + std::to_string(servers.size()),
                 EventColor::Magenta);
    }
}
