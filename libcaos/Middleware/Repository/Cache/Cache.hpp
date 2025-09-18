#pragma once

#include "../IRepository.hpp"
#include <memory>

class Cache : public IRepository
{
  public:
    Cache(std::unique_ptr<IRepository> database_)
      : database(std::move(database_)) {}

    ~Cache() override
    {
      spdlog::trace("Destroying Cache");
    };

    std::optional<std::string> echoString(std::string str) override
    {
      if (str=="pippo")
      {
        // Simulo un ritorno da Redis
        return str+ " from cache";
      }
      else
      {
        // Interroga il db
        auto ret = database->echoString (str);

        if (ret.has_value())
        {
          // Store value into the cache
        }

        return ret;
      }
    };

  private:
    std::unique_ptr<IRepository> database;
    // Logica cache (da implementare dopo)
};
