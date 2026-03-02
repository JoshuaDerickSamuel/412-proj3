#ifndef REQUEST_H
#define REQUEST_H

#include <string>

/**
 * @struct Request
 * @brief Represents one web request handled by the load-balancer simulation.
 *
 * A request stores source and destination IP addresses, processing time in
 * clock cycles, job type (`P` or `S`), and the cycle at which the request was created.
 */
struct Request {
    int id;
    std::string ipIn;
    std::string ipOut;
    int timeRequired;
    char jobType;
    int arrivalTime;

    /**
     * @brief Creates a request object.
     * @param requestId Unique request identifier.
     * @param sourceIp Source/requester IPv4 address.
     * @param destinationIp Destination/response IPv4 address.
     * @param processingCycles Required processing cycles.
     * @param requestType Job type (`P` processing, `S` streaming).
     * @param createdAt Simulation cycle when this request was created.
     */
    Request(
        int requestId = 0,
        std::string sourceIp = "0.0.0.0",
        std::string destinationIp = "0.0.0.0",
        int processingCycles = 1,
        char requestType = 'P',
        int createdAt = 0
    );
};

#endif
