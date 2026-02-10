// Complete this file by creating the methods you described in query_definitions.txt

#pragma once

#include "Middleware/Repository/Database/PostgreSQL/PostgreSQL.hpp"

#ifdef QUERY_EXISTS_IQuery_Template_echoString
std::optional<std::string> PostgreSQL::IQuery_Template_echoString(std::string str)
{
  if(!running.load(std::memory_order_relaxed))
  {
    return std::nullopt;
  }

  auto connection_opt = this->database->acquire();

  try
  {
    if (connection_opt)
    {
      Database::ConnectionWrapper& connection = connection_opt.value();

      pqxx::result result;

      {
        pqxx::work tx(*connection);
        pqxx::params p;
        p.append(str);
        result = tx.exec("SELECT  $1", p);/*pg_sleep(0.05),*/
        tx.commit();
      }

      if (!result.empty())
      {
        return result[0][0].as<std::string>() + " from PostgreSQL";
      }

      return std::nullopt;
    }

    throw repository::broken_connection("Database connection unavailable - cannot acquire connection from pool");

  }
  catch (const repository::broken_connection& e)
  {
    throw;
  }
  catch (const pqxx::sql_error& e)
  {
    throw;
  }
  catch (const std::exception& e)
  {
    throw;
  }
  catch(...)
  {
    throw;
  }

  return std::nullopt;
}
#endif // End of ifdef QUERY_EXISTS_IQuery_Template_echoString

#ifdef QUERY_EXISTS_IQuery_Template_echoString_custom
std::optional<std::string> PostgreSQL::IQuery_Template_echoString_custom(std::string str)
{
  if(!running.load(std::memory_order_relaxed))
  {
    return std::nullopt;
  }

  auto connection_opt = this->database->acquire();

  try
  {
    if (connection_opt)
    {
      Database::ConnectionWrapper& connection = connection_opt.value();

      pqxx::result result;

      {
        pqxx::work tx(*connection);
        pqxx::params p;
        p.append(str);
        result = tx.exec("SELECT  $1", p);/*pg_sleep(0.05),*/
        tx.commit();
      }

      if (!result.empty())
      {
        return result[0][0].as<std::string>() + " from PostgreSQL";
      }

      return std::nullopt;
    }

    throw repository::broken_connection("Database connection unavailable - cannot acquire connection from pool");

  }
  catch (const repository::broken_connection& e)
  {
    throw;
  }
  catch (const pqxx::sql_error& e)
  {
    throw;
  }
  catch (const std::exception& e)
  {
    throw;
  }
  catch(...)
  {
    throw;
  }

  return std::nullopt;
}
#endif // End of ifdef QUERY_EXISTS_IQuery_Template_echoString_custom
