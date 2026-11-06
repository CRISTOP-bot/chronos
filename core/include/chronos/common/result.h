#pragma once
#include "chronos/common/error.h"
#include <variant>
#include <utility>

namespace chronos {

template <typename T>
class Result {
public:
    explicit Result(T value) : data_(std::move(value)) {}
    explicit Result(Error err) : data_(std::move(err)) {}

    bool ok() const { return std::holds_alternative<T>(data_); }
    T& value() { return std::get<T>(data_); }
    const T& value() const { return std::get<T>(data_); }
    Error& error() { return std::get<Error>(data_); }
    const Error& error() const { return std::get<Error>(data_); }

    // convenience
    T value_or(T fallback) const {
        if (ok()) return value();
        return std::move(fallback);
    }

private:
    std::variant<T, Error> data_;
};

template <>
class Result<void> {
public:
    Result() : err_(Error::none()) {}
    explicit Result(Error err) : err_(std::move(err)) {}
    bool ok() const { return err_.ok(); }
    Error& error() { return err_; }
    const Error& error() const { return err_; }
private:
    Error err_;
};

} // namespace chronos
