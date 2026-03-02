#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <optional>
#include "Request.h"

/**
 * @class WebServer
 * @brief Simulates one worker server that handles one request at a time.
 *
 * Each server can be idle or active. When active, it counts down remaining
 * cycles each tick until the current request completes.
 */
class WebServer {
public:
    /** @brief Constructs a server with a unique integer id. */
    explicit WebServer(int id);

    /** @brief Returns this server's unique id. */
    int getId() const;

    /** @brief Returns true when the server has no active request. */
    bool isIdle() const;

    /** @brief Assigns a request if the server is currently idle. */
    bool assign(const Request& request);

    /** @brief Advances one cycle and reports completion when a request finishes. */
    bool tick(Request& completedRequest);

    /** @brief Returns remaining cycles for the current request (0 if idle). */
    int remainingCycles() const;

private:
    int serverId;
    int remaining;
    std::optional<Request> active;
};

#endif
