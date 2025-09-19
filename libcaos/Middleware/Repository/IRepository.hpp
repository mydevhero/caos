#pragma once
#include <optional>
#include <atomic>
#include "libcaos/config.hpp"

#include "Exception.hpp"

class IRepository
{
  public:
    std::atomic<bool>                                   running_{true}                    ;

    IRepository() = default;

    virtual ~IRepository() = default;

    virtual std::optional<std::string> echoString(std::string str) = 0;
};
