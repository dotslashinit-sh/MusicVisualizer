#pragma once

#include "Error.hpp"

#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>
#include <thread>
#include <atomic>

class ErrorReporter {
public:
    ErrorReporter();
    ErrorReporter(ErrorReporter&&) = delete;
    ErrorReporter(const ErrorReporter&) = delete;

    ErrorReporter& operator=(ErrorReporter&&) = delete;
    ErrorReporter& operator=(const ErrorReporter&) = delete;

    virtual ~ErrorReporter();

    void Report(const Error_t& err);

private:
    std::mutex m_mutex;
    std::optional<Error_t> m_error;
    std::atomic_bool m_shutDownRequest;
    std::unique_ptr<std::thread> m_workerThreadPtr;
    std::condition_variable m_cv;

    void _ReporterWorker();
};