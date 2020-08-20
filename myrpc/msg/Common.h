/* 标志状态 */

#pragma once

namespace myrpc {

enum class ReturnCode {
    OK = 0,
    ERROR = -1,
    ERROR_UNIMPLEMENT = -101,
    ERROR_SOCKET = -102,
    ERROR_STREAM_NOT_GOOD = -103,
    ERROR_LENGTH_UNDERFLOW = - 104,
    ERROR_LENGTH_OVERFLOW = -105,
    ERROR_SOCKET_STREAM_TIMEOUT = -202,
    ERROR_SOCKET_STREAM_NORMAL_CLOSED = -303,
    ERROR_VIOLATE_PROTOCOL = -401,
    MAX,
};

enum class Direcction {
    NON3 = 0,
    REQUEST,
    RESPONSE,
    MAX,
};

}