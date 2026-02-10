#pragma once

#include "Middleware/Repository/Database/MariaDB/Query.hpp"

#ifdef QUERY_EXISTS_IQuery_Template_echoString
std::optional<std::string> MariaDB::IQuery_Template_echoString(std::string str)
{
  static constexpr const char* fName = "MariaDB::IQuery_Template_echoString";

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

      // Disable autocommit mocking a transaction
      connection->setAutoCommit(false);

      try
      {
        std::unique_ptr<sql::PreparedStatement> pstmt(
          connection->prepareStatement("SELECT ? as echoed_string")
        );

        pstmt->setString(1, str);
        std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

        std::optional<std::string> returnValue = std::nullopt;

        if (result->next())
        {
          returnValue = result->getString("echoed_string") + " from MariaDB";
        }

        // Explicit commit
        connection->commit();
        connection->setAutoCommit(true);

        return returnValue;
      }
      catch (...)
      {
        // Rollback
        try { connection->rollback(); } catch (...) {}
        try { connection->setAutoCommit(true); } catch (...) {}
        throw;
      }
    }

    throw repository::broken_connection("Database connection unavailable - cannot acquire connection from pool");
  }
  catch (const repository::broken_connection& e)
  {
    throw;
  }
  catch (sql::SQLException& e)
  {
    std::uint32_t errorCode = e.getErrorCode();

    if (errorCode == 2002 ||
        errorCode == 2003 ||
        errorCode == 2006 ||
        errorCode == 2013 ||
        errorCode == 1927)
    {
      throw repository::broken_connection("MariaDB connection broken: " + std::string(e.what()));
    }

    spdlog::error("[{}] MariaDB error [{}:{}]: {}", fName, errorCode, e.getSQLState(), e.what());

    throw std::runtime_error("MariaDB database error");
  }
  catch (const std::exception& e)
  {
    spdlog::error("[{}] Exception: {}", fName, e.what());
    throw;
  }
  catch(...)
  {
    spdlog::error("[{}] Unknown exception", fName);
    throw std::runtime_error("Unknown error in echoString");
  }

  return std::nullopt;
}
#endif // End of ifdef QUERY_EXISTS_IQuery_Template_echoString

#ifdef QUERY_EXISTS_IQuery_Template_echoString_custom
std::optional<std::string> MariaDB::IQuery_Template_echoString_custom(std::string str)
{
  static constexpr const char* fName = "MariaDB::IQuery_Template_echoString_custom";

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

      // Disable autocommit mocking a transaction
      connection->setAutoCommit(false);

      try
      {
        std::unique_ptr<sql::PreparedStatement> pstmt(
          connection->prepareStatement("SELECT ? as echoed_string")
        );

        pstmt->setString(1, str);
        std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

        std::optional<std::string> returnValue = std::nullopt;

        if (result->next())
        {
          returnValue = result->getString("echoed_string") + " from MariaDB";
        }

        // Explicit commit
        connection->commit();
        connection->setAutoCommit(true);

        return returnValue;
      }
      catch (...)
      {
        // Rollback
        try { connection->rollback(); } catch (...) {}
        try { connection->setAutoCommit(true); } catch (...) {}
        throw;
      }
    }

    throw repository::broken_connection("Database connection unavailable - cannot acquire connection from pool");
  }
  catch (const repository::broken_connection& e)
  {
    throw;
  }
  catch (sql::SQLException& e)
  {
    std::uint32_t errorCode = e.getErrorCode();

    if (errorCode == 2002 ||
        errorCode == 2003 ||
        errorCode == 2006 ||
        errorCode == 2013 ||
        errorCode == 1927)
    {
      throw repository::broken_connection("MariaDB connection broken: " + std::string(e.what()));
    }

    spdlog::error("[{}] MariaDB error [{}:{}]: {}", fName, errorCode, e.getSQLState(), e.what());

    throw std::runtime_error("MariaDB database error");
  }
  catch (const std::exception& e)
  {
    spdlog::error("[{}] Exception: {}", fName, e.what());
    throw;
  }
  catch(...)
  {
    spdlog::error("[{}] Unknown exception", fName);
    throw std::runtime_error("Unknown error in echoString");
  }

  return std::nullopt;
}
#endif // End of ifdef QUERY_EXISTS_IQuery_Template_echoString_custom
