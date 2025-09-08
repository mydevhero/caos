#include "Database.hpp"



std::unique_ptr<IRepository> Database::chooseDatabase(DatabaseType type)
{
  if (type == DatabaseType::PostgreSQL)
  {
    spdlog::trace("DatabaseType is PostgreSQL");
    return std::make_unique<PostgreSQL>();
  }

  throw std::runtime_error("Unsupported database type");
}

Database::Database()
{
  database = Database::chooseDatabase(CAOS_DEFAULT_DATABASE_TYPE);

  spdlog::trace("Repository initialized");
}

Database::~Database()
{
  database.reset();
  spdlog::trace("Destroying Repository");
}
