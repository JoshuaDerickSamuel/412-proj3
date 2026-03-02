#include "Request.h"

#include <utility>

/**
 * @file Request.cpp
 * @brief Implementation for request payload construction.
 */

Request::Request(
    int requestId,
    std::string sourceIp,
    std::string destinationIp,
    int processingCycles,
    char requestType,
    int createdAt
)
    : id(requestId),
      ipIn(std::move(sourceIp)),
      ipOut(std::move(destinationIp)),
      timeRequired(processingCycles),
      jobType(requestType),
      arrivalTime(createdAt) {}
