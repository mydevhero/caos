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
        return database->echoString (str);
      }
    };

  private:
    std::unique_ptr<IRepository> database;
    // Logica cache (da implementare dopo)
};
