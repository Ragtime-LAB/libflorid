#ifndef FLORID_EXCEPTIONS_HPP
#define FLORID_EXCEPTIONS_HPP

#include <stdexcept>
#include <system_error>

namespace florid {

class Exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class NetworkException : public Exception {
public:
    using Exception::Exception;

    NetworkException(const std::string& s_message,
                     std::error_code s_system_error)
        : Exception(s_message), m_system_error(s_system_error) {}

    [[nodiscard]] const std::error_code& systemError() const noexcept {
        return m_system_error;
    }

private:
    std::error_code m_system_error;
};

class ProtocolException : public Exception {
public:
    using Exception::Exception;
};

class IncompatibleVersionException : public Exception {
public:
    using Exception::Exception;
};

class ControlException : public Exception {
public:
    using Exception::Exception;
};

class RealtimeException : public Exception {
public:
    using Exception::Exception;
};

class InvalidOperationException : public Exception {
public:
    using Exception::Exception;
};

class CommandException : public Exception {
public:
    using Exception::Exception;
};

} // namespace florid

#endif // FLORID_EXCEPTIONS_HPP
