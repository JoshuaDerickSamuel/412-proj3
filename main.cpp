#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "LoadBalancer.h"
#include "Switch.h"

/**
 * @file main.cpp
 * @brief Simulation entry point, configuration parsing, logging, and execution loop.
 */

namespace {
/**
 * @struct AppConfig
 * @brief High-level app settings loaded from config file.
 */
struct AppConfig {
    int initialServers;
    int simulationCycles;

    int randomArrivalMin;
    int randomArrivalMax;

    bool useSwitchMode;
    int streamingInitialServers;
    int processingInitialServers;

    std::string logFile;

    LoadBalancerConfig lb;
    AppConfig()
        : initialServers(10),
          simulationCycles(10000),
          randomArrivalMin(0),
          randomArrivalMax(4),
          useSwitchMode(false),
          streamingInitialServers(5),
          processingInitialServers(5),
          logFile("simulation.log") {
        lb.initialQueueMultiplier = 100;
        lb.minRequestTime = 5;
        lb.maxRequestTime = 200;
        lb.scaleLowMultiplier = 50;
        lb.scaleHighMultiplier = 80;
        lb.scaleCooldownCycles = 30;
        lb.processingJobProbability = 0.60;
        lb.blockedCidrs = {"10.0.0.0/8", "192.168.0.0/16"};
    }
};

/** @brief Trims leading and trailing whitespace from a string. */
std::string trim(const std::string& input) {
    if (input.empty()) {
        return "";
    }

    std::size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    if (start == input.size()) {
        return "";
    }

    std::size_t end = input.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(input[end]))) {
        --end;
    }

    return input.substr(start, end - start + 1);
}

/** @brief Converts a text token into boolean (`true/false`, `yes/no`, `1/0`). */
bool asBool(const std::string& value) {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

/** @brief Splits a comma-separated string into a list of trimmed tokens. */
std::vector<std::string> splitCsv(const std::string& value) {
    std::vector<std::string> output;
    std::stringstream ss(value);
    std::string token;

    while (std::getline(ss, token, ',')) {
        token = trim(token);
        if (!token.empty()) {
            output.push_back(token);
        }
    }

    return output;
}

/**
 * @brief Loads configuration keys from `config.txt` style `key=value` file.
 * @param path Config file path.
 * @param config Output configuration structure to update.
 * @return True if file exists and is parsed.
 */
bool loadConfigFromFile(const std::string& path, AppConfig& config) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, pos));
        const std::string value = trim(line.substr(pos + 1));

        try {
            if (key == "initialServers") config.initialServers = std::stoi(value);
            else if (key == "simulationCycles") config.simulationCycles = std::stoi(value);
            else if (key == "initialQueueMultiplier") config.lb.initialQueueMultiplier = std::stoi(value);
            else if (key == "minRequestTime") config.lb.minRequestTime = std::stoi(value);
            else if (key == "maxRequestTime") config.lb.maxRequestTime = std::stoi(value);
            else if (key == "randomArrivalMin") config.randomArrivalMin = std::stoi(value);
            else if (key == "randomArrivalMax") config.randomArrivalMax = std::stoi(value);
            else if (key == "scaleLowMultiplier") config.lb.scaleLowMultiplier = std::stoi(value);
            else if (key == "scaleHighMultiplier") config.lb.scaleHighMultiplier = std::stoi(value);
            else if (key == "scaleCooldownCycles") config.lb.scaleCooldownCycles = std::stoi(value);
            else if (key == "processingJobProbability") config.lb.processingJobProbability = std::stod(value);
            else if (key == "useSwitchMode") config.useSwitchMode = asBool(value);
            else if (key == "streamingInitialServers") config.streamingInitialServers = std::stoi(value);
            else if (key == "processingInitialServers") config.processingInitialServers = std::stoi(value);
            else if (key == "blockedCidrs") config.lb.blockedCidrs = splitCsv(value);
            else if (key == "logFile") config.logFile = value;
        } catch (...) {
            continue;
        }
    }

    return true;
}

/** @brief Returns ANSI color prefix for console event rendering. */
std::string colorCode(EventColor color) {
    switch (color) {
        case EventColor::Red: return "\033[31m";
        case EventColor::Green: return "\033[32m";
        case EventColor::Yellow: return "\033[33m";
        case EventColor::Blue: return "\033[34m";
        case EventColor::Magenta: return "\033[35m";
        case EventColor::Cyan: return "\033[36m";
        case EventColor::White: return "\033[37m";
        default: return "\033[0m";
    }
}

/**
 * @brief Writes one timestamped event to console and log file.
 * @param clock Current simulation cycle.
 * @param message Event message.
 * @param color Console color.
 * @param logOut Output file stream.
 */
void logEvent(int clock, const std::string& message, EventColor color, std::ofstream& logOut) {
    std::ostringstream line;
    line << "[T=";
    line.width(6);
    line << clock << "] " << message;

    std::cout << colorCode(color) << line.str() << "\033[0m" << '\n';
    if (logOut.is_open()) {
        logOut << line.str() << '\n';
    }
}

/** @brief Writes a plain summary line to console and log file. */
void writeSummaryLine(const std::string& message, std::ofstream& logOut) {
    std::cout << message << '\n';
    if (logOut.is_open()) {
        logOut << message << '\n';
    }
}

/** @brief Prints startup banner for interactive runs. */
void printBanner() {
    std::cout << "  J-Tech Load Balancer Simulation\n";
}

/**
 * @brief Prints end-of-run summary counters for one balancer.
 * @param lb Balancer to summarize.
 * @param logOut Output file stream.
 */
void printSummary(const LoadBalancer& lb, std::ofstream& logOut) {
    const LoadBalancerStats& s = lb.stats();

    writeSummaryLine("\n--- Summary: " + lb.getName() + " ---", logOut);
    writeSummaryLine("Initial Created:" + std::to_string(s.initialCreated), logOut);
    writeSummaryLine("Generated(In):  " + std::to_string(s.generated), logOut);
    writeSummaryLine("Accepted:       " + std::to_string(s.accepted), logOut);
    writeSummaryLine("Blocked:        " + std::to_string(s.blocked), logOut);
    writeSummaryLine("Assigned:       " + std::to_string(s.assigned), logOut);
    writeSummaryLine("Completed:      " + std::to_string(s.completed), logOut);
    writeSummaryLine("Peak Queue:     " + std::to_string(s.peakQueue), logOut);
    writeSummaryLine("Servers Added:  " + std::to_string(s.addedServers), logOut);
    writeSummaryLine("Servers Removed:" + std::to_string(s.removedServers), logOut);
    writeSummaryLine("Final Queue:    " + std::to_string(lb.queueSize()), logOut);
    writeSummaryLine("Final Servers:  " + std::to_string(lb.serverCount()), logOut);
}
}

/**
 * @brief Program entry point.
 * @return 0 on success; non-zero when configuration cannot be loaded.
 */
int main() {
    printBanner();

    std::mt19937 rng(std::random_device{}());

    AppConfig config;
    if (!loadConfigFromFile("config.txt", config)) {
        std::cerr << "Missing config.txt\n";
        return 1;
    }
    std::cout << "Loaded configuration\n";

    int chosenServers = config.initialServers;
    int chosenCycles = config.simulationCycles;

    std::cout << "Enter initial server count: ";
    if (!(std::cin >> chosenServers)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        chosenServers = config.initialServers;
    }

    std::cout << "Enter simulation cycles: ";
    if (!(std::cin >> chosenCycles)) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        chosenCycles = config.simulationCycles;
    }

    config.initialServers = std::max(1, chosenServers);
    config.simulationCycles = std::max(1, chosenCycles);

    std::ofstream logOut(config.logFile, std::ios::out);
    auto logFn = [&logOut](int clock, const std::string& message, EventColor color) {
        logEvent(clock, message, color, logOut);
    };

    if (config.useSwitchMode) {
        auto streamingBalancer = std::make_unique<LoadBalancer>("Streaming-LB", std::max(1, config.streamingInitialServers), config.lb, logFn);
        auto processingBalancer = std::make_unique<LoadBalancer>("Processing-LB", std::max(1, config.processingInitialServers), config.lb, logFn);
        Switch trafficSwitch(*streamingBalancer, *processingBalancer);

        streamingBalancer->seedInitialQueue(0);
        processingBalancer->seedInitialQueue(0);

        for (int clock = 1; clock <= config.simulationCycles; ++clock) {
            const int arrivalFloor = std::max(0, config.randomArrivalMin);
            const int arrivalCeil = std::max(arrivalFloor, config.randomArrivalMax);
            std::uniform_int_distribution<int> arrivalDist(arrivalFloor, arrivalCeil);
            const int arrivalsThisTick = arrivalDist(rng);

            for (int i = 0; i < arrivalsThisTick; ++i) {
                Request request = processingBalancer->createRandomRequest(clock);
                trafficSwitch.route(request, clock);
            }

            streamingBalancer->tick(clock);
            processingBalancer->tick(clock);

            if (clock % 500 == 0) {
                logFn(clock,
                      "Switch mode status check | StreamQ=" + std::to_string(streamingBalancer->queueSize()) +
                          " ProcQ=" + std::to_string(processingBalancer->queueSize()),
                      EventColor::White);
            }
        }

        printSummary(*streamingBalancer, logOut);
        printSummary(*processingBalancer, logOut);
        return 0;
    }

    auto primaryBalancer = std::make_unique<LoadBalancer>("Primary-LB", config.initialServers, config.lb, logFn);
    primaryBalancer->seedInitialQueue(0);

    for (int clock = 1; clock <= config.simulationCycles; ++clock) {
        const int arrivalFloor = std::max(0, config.randomArrivalMin);
        const int arrivalCeil = std::max(arrivalFloor, config.randomArrivalMax);
        std::uniform_int_distribution<int> arrivalDist(arrivalFloor, arrivalCeil);
        const int arrivalsThisTick = arrivalDist(rng);

        for (int i = 0; i < arrivalsThisTick; ++i) {
            primaryBalancer->submit(primaryBalancer->createRandomRequest(clock), clock);
        }

        primaryBalancer->tick(clock);

        if (clock % 500 == 0) {
            logFn(clock,
                  "Status Check | queue=" + std::to_string(primaryBalancer->queueSize()) +
                      " servers=" + std::to_string(primaryBalancer->serverCount()),
                  EventColor::White);
        }
    }

    printSummary(*primaryBalancer, logOut);
    return 0;
}
