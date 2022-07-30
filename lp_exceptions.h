//
// Created by jpricarte on 15/07/22.
//

#ifndef LEAVE_PACKAGE_LP_EXCEPTIONS_H
#define LEAVE_PACKAGE_LP_EXCEPTIONS_H

#include <exception>

struct SemaphoreOverused : public std::exception {
    [[nodiscard]] const char *what() const noexcept override {
        return "Waiting too long for semaphore";
    }
};

struct SocketWriteError : public std::exception {
    [[nodiscard]] const char *what() const noexcept override {
        return "Error writing to socket";
    }
};

struct SocketReadError : public std::exception {
    [[nodiscard]] const char *what() const noexcept override {
        return "Error reading data from socket";
    }
};

#endif //LEAVE_PACKAGE_LP_EXCEPTIONS_H
