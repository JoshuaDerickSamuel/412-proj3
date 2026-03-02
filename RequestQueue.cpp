#include "RequestQueue.h"

/**
 * @file RequestQueue.cpp
 * @brief Implementation of custom Request FIFO queue.
 */

RequestQueue::RequestQueue() : head(nullptr), tail(nullptr), count(0) {}

RequestQueue::~RequestQueue() {
    clear();
}

void RequestQueue::enqueue(const Request& request) {
    Node* node = new Node(request);

    if (tail == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }

    ++count;
}

Request RequestQueue::dequeue() {
    if (empty()) {
        throw std::runtime_error("Attempted dequeue on empty RequestQueue");
    }

    Node* frontNode = head;
    Request nextRequest = frontNode->data;
    head = frontNode->next;

    if (head == nullptr) {
        tail = nullptr;
    }

    delete frontNode;
    --count;
    return nextRequest;
}

bool RequestQueue::empty() const {
    return count == 0;
}

std::size_t RequestQueue::size() const {
    return count;
}

void RequestQueue::clear() {
    while (!empty()) {
        dequeue();
    }
}
