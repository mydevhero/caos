#include "Database.hpp"
#include <cctype> // for toupper
// #include <format>
std::unique_ptr<IRepository> Database::chooseDatabase(DatabaseType type)
{
  if (type == DatabaseType::PostgreSQL)
  {
    spdlog::trace("DatabaseType is PostgreSQL");
    return std::make_unique<PostgreSQL>(this);
  }

  throw std::runtime_error("Unsupported database type");
}













