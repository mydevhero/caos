// MySQL.hpp
#pragma once

#include <libcaos/config.hpp>

#include "../DatabaseFwd.hpp"


class MySQL final: public IRepository
{
  private:
    Database* database;

  public:

    MySQL(Database*);

    ~MySQL() override;

    std::optional<std::string> echoString(std::string str) override
    {
      static constexpr const char* fName = "MySQL::echoString";

      if(!running_.load(std::memory_order_relaxed))
      {
        return std::nullopt;
      }

      auto connection_opt = this->database->acquire();

      try
      {
        if (connection_opt)
        {
          Database::ConnectionWrapper& connection = connection_opt.value();

          // Disabilita autocommit per simulare una transazione
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
              returnValue = result->getString("echoed_string") + " from MySQL";
            }

            // Commit esplicito
            connection->commit();
            connection->setAutoCommit(true); // Ripristina autocommit

            return returnValue;
          }
          catch (...)
          {
            // Rollback in caso di eccezione
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
      catch (const sql::SQLException& e)
      {
        const int errorCode = e.getErrorCode();

        // Mappatura errori di connessione
        if (errorCode == 2002 || errorCode == 2003 || errorCode == 2006 ||
            errorCode == 2013 || errorCode == 1927) {
          throw repository::broken_connection("MySQL connection broken: " + std::string(e.what()));
        }

        spdlog::error("[{}] MySQL error [{}:{}]: {}", fName, errorCode, e.getSQLState(), e.what());
        throw std::runtime_error("MySQL database error");
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
};





