#pragma once

enum STATUS_CODE {
    STATUS_SUCCESS,
    STATUS_GLFW_INIT_FAILED,
    STATUS_GLAD_INIT_FAILED,
    STATUS_ERROR_MEMORY_ALLOC,
    STATUS_NOT_INITIALIZED,
    STATUS_WINDOW_CREATE_EXCEPT
};

// /// @brief Fetches the message in string format for the specified status code. Note:
// /// Not all status codes may give meaningful messages.
// /// @param code `STATUS_CODE` for which the message must be fetched
// /// @return Returns the status code message as a C-style string.
// constexpr const char* GetStatusMessage(const STATUS_CODE code) noexcept;