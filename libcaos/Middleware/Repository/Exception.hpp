#pragma once
#include <exception>
#include <string>

namespace repository
{
  class broken_connection : public std::exception
  {
  private:
    std::string message_;

  public:
    explicit broken_connection(const std::string& what_arg)
      : message_(what_arg) {}

    explicit broken_connection(const char* what_arg)
      : message_(what_arg) {}

    virtual const char* what() const noexcept override {
      return message_.c_str();
    }

    virtual ~broken_connection() = default;
  };
}
