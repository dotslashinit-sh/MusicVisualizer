#pragma once

#include <string>
#include <optional>

enum ERROR_CODE {
    ERROR_CODE_UNKNOWN,
    ERROR_CODE_INVALID_ARG,
    ERROR_CODE_INSTANCE_INIT_FAIL,
    ERROR_CODE_DEVICE_INIT_FAIL,
    ERROR_CODE_DEVICE_FETCH_FAIL,
    ERROR_CODE_DATA_FETCH_FAIL,
    ERROR_CODE_AUDIO_CAPTURE_START_FAIL,
    ERROR_CODE_FILE_OPEN_FAIL,
    ERROR_CODE_YAML_PARSE_FAIL,
    ERROR_CODE_MISSING_PARAMS
};

class Error_t {
public:
    Error_t(const ERROR_CODE code, const std::string& msg) : Message(msg), Code(code) {}

    std::string Message;
    ERROR_CODE Code;
};

typedef std::optional<Error_t> Status_t;
constexpr Status_t STATUS_OK = std::nullopt;

inline Status_t GenError(const ERROR_CODE code, const std::string& msg) {
    return std::make_optional<Error_t>(code, msg);
}

bool operator==(const Status_t& a, const Status_t& b);