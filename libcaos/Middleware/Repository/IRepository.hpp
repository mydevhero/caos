#pragma once
#include <optional>

class IRepository {
public:
    IRepository() = default;

    virtual ~IRepository() = default;

    virtual std::optional<std::string> echoString(std::string str) = 0;
};
