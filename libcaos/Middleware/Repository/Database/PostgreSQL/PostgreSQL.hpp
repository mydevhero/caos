// PostgreSQL.hpp
#pragma once

#include <libcaos/config.hpp>

#include "../DatabaseFwd.hpp"

// #define CAOS_POSTGRESQL_QUERY_WITH_CONNECTION_GUARD(conn, CODE) \
//   ([&]() {                                                      \
//     try {                                                       \
//       auto result = (CODE)();                                   \
//       return result;                                            \
//     }                                                           \
//     catch (const pqxx::broken_connection& e) {                  \
//       if (conn) {                                               \
//         this->closeConnection(*((conn).getRaw()));  \
//       }                                                         \
//       throw PostgreSQL::broken_connection(e.what());            \
//     }                                                           \
//   }())

// #define CAOS_POSTGRESQL_CLOSE_CONNECTION()                      \
//   PostgreSQL::Pool::closeConnection(*(connection_opt.value().getRaw()));

class PostgreSQL final: public IRepository
{
  private:
    Database* database;

  public:

    PostgreSQL(Database*);

    ~PostgreSQL() override;

    std::optional<std::string> echoString(std::string str) override
    {
      static constexpr const char* fName = "PostgreSQL::echoString";

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
};





