#include "ErrorReporter.hpp"

#include <functional>
#include <print>

ErrorReporter::ErrorReporter() :
    m_error(), m_mutex(), m_workerThreadPtr(),
    m_shutDownRequest(false), m_cv()
{
    m_workerThreadPtr = std::make_unique<std::thread>(
        &ErrorReporter::_ReporterWorker, this
    );
}

void ErrorReporter::Report(const Error_t& err) {
    std::unique_lock lk(m_mutex);
    m_error = err;
    m_cv.notify_all();
}

void ErrorReporter::_ReporterWorker() {
    while(!m_shutDownRequest) {
        std::unique_lock lk(m_mutex);
        m_cv.wait(lk, [this]() {
            return m_shutDownRequest || m_error.has_value();
        });
        if(m_error.has_value()) {
            std::println("Error: Code: {}: {}", static_cast<int>(m_error.value().Code), m_error.value().Message);
            m_error = std::nullopt;
        }
    }
}

ErrorReporter::~ErrorReporter() {
    {
        std::lock_guard lk(m_mutex);
        m_shutDownRequest = true;
        m_cv.notify_all();
    }
    m_workerThreadPtr->join();
}