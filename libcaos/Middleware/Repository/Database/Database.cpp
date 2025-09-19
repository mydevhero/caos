#include "Database.hpp"
// #include <cctype> // for toupper
// #include <format>
std::unique_ptr<IRepository> Database::chooseDatabase(DatabaseType type)
{
  #ifdef CAOS_USE_DB_POSTGRESQL
  if (type == DatabaseType::PostgreSQL)
  {
    spdlog::trace("DatabaseType is PostgreSQL");
    return std::make_unique<PostgreSQL>(this);
  }
  #endif

  #ifdef CAOS_USE_DB_MYSQL
  if (type == DatabaseType::MySQL)
  {
    spdlog::trace("DatabaseType is MySQL");
    return std::make_unique<MySQL>(this);
  }
  #endif

  throw std::runtime_error("Unsupported database type");
}













