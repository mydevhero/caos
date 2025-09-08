#pragma once

#include <libcaos/config.hpp>
#include "../IRepository.hpp"
#include "PostgreSQL/PostgreSQL.hpp"

enum class DatabaseType: std::uint8_t {PostgreSQL=0, EOE};                                          // End Of Enum

class Database : public IRepository
{
  private:
      std::unique_ptr<IRepository> database;

      static std::unique_ptr<IRepository> chooseDatabase(DatabaseType type);

  public:
    Database();

    ~Database() override;

    std::optional<std::string> echoString(std::string str) override
    {
      return database->echoString(str);
    };
};
