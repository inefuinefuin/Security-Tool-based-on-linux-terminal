#pragma once 
#include <optional>
#include <variant>
#include <functional>
#include <iostream>

namespace Type{
    template<typename Ok, typename Error>
    class Result{
        std::variant<Ok, Error> data;
    public:
        explicit Result(const Ok _ok): data(_ok) {}
        explicit Result(const Error _error): data(_error) {}

        bool is_ok() const {
            return std::holds_alternative<Ok>(data);
        }
        bool is_err() const {
            return std::holds_alternative<Error>(data);
        }

        std::optional<Ok> ok() const {
            if(is_ok())
                return std::get<Ok>(data);
            return std::nullopt;
        }
        std::optional<Error> error() const {
            if(is_err())
                return std::get<Error>(data);
            return std::nullopt;
        }

        Ok unwrap() const {
            if(is_ok())
                return std::get<Ok>(data);
            throw std::runtime_error("Attempted to unwrap an error Result");
        }
        Error unwrap_err() const {
            if(is_err())
                return std::get<Error>(data);
            throw std::runtime_error("Attempted to unwrap an ok Result");
        }

        template<typename U>
        Result<U, Error> map(std::function<U(const Ok&)> func) const {
            if(is_ok()){
                return Result<U, Error>(func(std::get<Ok>(data)));
            }else {
                return Result<U, Error>(std::get<Error>(data));
            }
        }
     };


    template<typename Ok>
    class Result<Ok,void> {
        std::optional<Ok> data;
    public:
        explicit Result(const Ok value): data(value) {}

        bool is_ok() const {
            return data.has_value();
        }
        bool is_err() const {
            return !data.has_value();
        }

        std::optional<Ok> ok() const {
            return data;
        }
        std::optional<Ok> error() const {
            return std::nullopt;
        }

        Ok unwrap() const {
            if(is_ok())
                return data.value();
            throw std::runtime_error("Attempted to unwrap an error Result");
        }

        void unwrap_err() const {
            if(is_err())
                return;
            throw std::runtime_error("Attempted to unwrap_err on an ok Result");
        }
    };
}
