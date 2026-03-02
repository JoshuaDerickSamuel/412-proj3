#include "WebServer.h"

/**
 * @file WebServer.cpp
 * @brief Implements single-server request processing behavior.
 */

WebServer::WebServer(int id) : serverId(id), remaining(0), active(std::nullopt) {}

int WebServer::getId() const {
    return serverId;
}

bool WebServer::isIdle() const {
    return !active.has_value();
}

bool WebServer::assign(const Request& request) {
    if (!isIdle()) {
        return false;
    }

    active = request;
    remaining = request.timeRequired;
    return true;
}

bool WebServer::tick(Request& completedRequest) {
    if (isIdle()) {
        return false;
    }

    --remaining;

    if (remaining <= 0) {
        completedRequest = *active;
        active.reset();
        remaining = 0;
        return true;
    }

    return false;
}

int WebServer::remainingCycles() const {
    return remaining;
}
