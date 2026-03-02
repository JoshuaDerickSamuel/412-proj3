#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include <cstddef>
#include <stdexcept>
#include "Request.h"

/**
 * @file RequestQueue.h
 * @brief Linked-list FIFO queue specialized for Request objects.
 */

/**
 * @class RequestQueue
 * @brief Provides queue operations used by the load balancer.
 */
class RequestQueue {
public:
    /** @brief Creates an empty queue. */
    RequestQueue();

    /** @brief Releases all remaining nodes. */
    ~RequestQueue();

    RequestQueue(const RequestQueue&) = delete;
    RequestQueue& operator=(const RequestQueue&) = delete;

    /** @brief Adds one request to the back of the queue. */
    void enqueue(const Request& request);

    /**
     * @brief Removes and returns the oldest request.
     * @throws std::runtime_error when queue is empty.
     */
    Request dequeue();

    /** @brief Returns true if no requests are queued. */
    bool empty() const;

    /** @brief Returns total number of queued requests. */
    std::size_t size() const;

    /** @brief Removes all requests from the queue. */
    void clear();

private:
    /** @brief One linked-list node for queue storage. */
    struct Node {
        Request data;
        Node* next;

        explicit Node(const Request& request) : data(request), next(nullptr) {}
    };

    Node* head;
    Node* tail;
    std::size_t count;
};

#endif
