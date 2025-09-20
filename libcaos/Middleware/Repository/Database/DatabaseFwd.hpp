#pragma once

#include "../IRepository.hpp"

#ifdef CAOS_USE_DB_POSTGRESQL
#include <pqxx/pqxx>
#include <pqxx/except>
#endif

#ifdef CAOS_USE_DB_MYSQL
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>
#endif

#include <spdlog/spdlog.h>
#include <terminal_options.hpp>
#include <environment.hpp>
#include <mutex>
#include <queue>
#include <functional>
#include <optional>
#include <memory>

enum class DatabaseType: std::uint8_t {
  PostgreSQL  = 0,
  MySQL       = 1,
  EOE                                                                                               // End Of Enum
};

#ifdef CAOS_USE_DB_POSTGRESQL
using dbconn            = pqxx::connection;
using dbuniq            = std::unique_ptr<pqxx::connection>;
using dboptuniqptr      = std::optional<const std::unique_ptr<pqxx::connection>*>;
using broken_connection = pqxx::broken_connection;
using sql_exception     = pqxx::sql_error;
#endif

#ifdef CAOS_USE_DB_MYSQL
using dbconn            = sql::Connection;
using dbuniq            = std::unique_ptr<sql::Connection>;
using dboptuniqptr      = std::optional<const std::unique_ptr<sql::Connection>*>;
using broken_connection = sql::SQLException;
using sql_exception     = sql::SQLException;
#endif

class Database : public IRepository
{
  public:
    class ConnectionWrapper
    {
      public:
        ConnectionWrapper(dboptuniqptr conn_ptr,
                          std::function<void(dboptuniqptr)> release_func)
          : connection_ptr(conn_ptr),
            release_func(std::move(release_func))
        {}

        ~ConnectionWrapper()
        {
          if (!released)
          {
            release_func(connection_ptr);
          }
        }

        // Move constructor
        ConnectionWrapper(ConnectionWrapper&& other) noexcept
          : connection_ptr(other.connection_ptr),
            release_func(std::move(other.release_func)),
            released(other.released)
        {
          other.released = true;
          other.connection_ptr = std::nullopt;
        }

        // No copy
        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        dbconn& operator*() const
        {
          try
          {
            if (connection_ptr.has_value())
            {
              return *(connection_ptr.value()->get());
            }

            throw repository::broken_connection("Connection wrapper is null");
          }
          catch (repository::broken_connection)
          {
            throw;
          }
        }

        dbconn* operator->() const
        {
          return connection_ptr.has_value() ? connection_ptr.value()->get() : nullptr;
        }

        [[nodiscard]] dbconn* get() const
        {
          return connection_ptr.has_value() ? connection_ptr.value()->get() : nullptr;
        }

        [[nodiscard]] const dbuniq* getRaw() const
        {
          return connection_ptr.has_value() ? connection_ptr.value() : nullptr;
        }

        void release()
        {
          if (!released && connection_ptr.has_value())
          {
            release_func(connection_ptr);
            released = true;
            connection_ptr = std::nullopt;
          }
        }

        explicit operator bool() const
        {
          return connection_ptr.has_value() && !released && connection_ptr.value() != nullptr;
        }

      private:
        dboptuniqptr                                  connection_ptr                            ;
        std::function<void(dboptuniqptr)>             release_func                              ;
        bool                                          released{false}                           ;
        std::function<void()>                         cleanup_handler_                          ;
    };

    class Pool
    {
      private:
        TerminalOptions*                              terminalPtr           = nullptr           ;
        Environment*                                  environmentRef        = nullptr           ;
        std::condition_variable                       shutdown_cv_                              ;
        std::mutex                                    shutdown_mutex_                           ;
        std::condition_variable                       condition                                 ;
        std::atomic<bool>                             running_                                  ;
        std::thread                                   healthCheckThread_                        ;

        struct config_s
        {
          std::string                                 user                                      ;
          std::string                                 pass                                      ;
          std::string                                 host                                      ;
          std::uint16_t                               port                  {0}                 ;
          std::string                                 name                                      ;
          std::size_t                                 poolsizemin           {0}                 ;
          std::size_t                                 poolsizemax           {0}                 ;
          std::uint32_t                               poolwait              {0}                 ;
          std::chrono::milliseconds                   pooltimeout           {0}                 ;

          #ifdef CAOS_USE_DB_POSTGRESQL
          std::size_t                                 keepalives            {0}                 ;
          std::size_t                                 keepalives_idle       {0}                 ;
          std::size_t                                 keepalives_interval   {0}                 ;
          std::size_t                                 keepalives_count      {0}                 ;
          std::string                                 connection_string                         ;
          #endif

          std::size_t                                 connect_timeout       {0}                 ;

          #if (defined(CAOS_USE_DB_MYSQL)||defined(CAOS_USE_DB_MARIADB))
          sql::ConnectOptionsMap                      connection_options                        ;
          #endif

          std::chrono::milliseconds                   max_wait              {0}                 ;
          std::chrono::milliseconds                   health_check_interval {0}                 ;
        };
        config_s config;

        // Setters ---------------------------------------------------------------------------------
        void                                          setUser()                                 ;
        void                                          setPass()                                 ;
        void                                          setHost()                                 ;
        void                                          setPort()                                 ;
        void                                          setName()                                 ;
        void                                          setPoolSizeMin()                          ;
        void                                          setPoolSizeMax()                          ;
        void                                          setPoolWait()                             ;
        void                                          setPoolTimeout()                          ;
        #ifdef CAOS_USE_DB_POSTGRESQL
        void                                          setKeepAlives()                           ;
        void                                          setKeepAlivesIdle()                       ;
        void                                          setKeepAlivesInterval()                   ;
        void                                          setKeepAlivesCount()                      ;
        void                                          setConnectStr()                   noexcept;
        #endif
        void                                          setConnectTimeout()                       ;
        void                                          setMaxWait()                              ;
        void                                          setHealthCheckInterval()                  ;

        #if (defined(CAOS_USE_DB_MYSQL)||defined(CAOS_USE_DB_MARIADB))
        void                                          setConnectOpt()                   noexcept;
        #endif

        std::size_t                                   init(std::size_t = 0)                     ;
        void                                          healthCheckLoop()                         ;
        dboptuniqptr                                  acquireConnection()                       ;
        void                                          handleInvalidConnection(const dbuniq&);
        void                                          cleanupMarkedConnections()                ;

        // Getters ---------------------------------------------------------------------------------
        [[nodiscard]] const std::string&              getUser()                   const noexcept;
        [[nodiscard]] const std::string&              getPass()                   const noexcept;
        [[nodiscard]] const std::string&              getHost()                   const noexcept;
        [[nodiscard]] const std::uint16_t&            getPort()                   const noexcept;
        [[nodiscard]] const std::string&              getName()                   const noexcept;
        [[nodiscard]] const std::size_t&              getPoolSizeMin()            const noexcept;
        [[nodiscard]] const std::size_t&              getPoolSizeMax()            const noexcept;
        [[nodiscard]] const std::uint32_t&            getPoolWait()               const noexcept;
        [[nodiscard]] const std::chrono::milliseconds& getPoolTimeout()           const noexcept;

        #ifdef CAOS_USE_DB_POSTGRESQL
        [[nodiscard]] const std::size_t&              getKeepAlives()             const noexcept;
        [[nodiscard]] const std::size_t&              getKeepAlivesIdle()         const noexcept;
        [[nodiscard]] const std::size_t&              getKeepAlivesInterval()     const noexcept;
        [[nodiscard]] const std::size_t&              getKeepAlivesCount()        const noexcept;
        [[nodiscard]] const std::string&              getConnectStr()             const noexcept;
        #endif

        [[nodiscard]] const std::size_t&              getConnectTimeout()         const noexcept;

        #if (defined(CAOS_USE_DB_MYSQL)||defined(CAOS_USE_DB_MARIADB))
        [[nodiscard]]       sql::ConnectOptionsMap&   getConnectOpt()                   noexcept;
        #endif

        [[nodiscard]] const std::chrono::milliseconds& getMaxWait()                const noexcept;
        [[nodiscard]] const std::chrono::milliseconds& getHealthCheckInterval()    const noexcept;
        [[nodiscard]] const bool                      isDevOrTestEnv()            const noexcept;
        [[nodiscard]] const bool checkPoolSize(std::size_t&) noexcept;

        [[nodiscard]] bool                            validateConnection(const dbuniq&)         ;
                      void                            createConnection(std::size_t&)            ;

        [[nodiscard]] const std::chrono::milliseconds getTotalDuration(const dbuniq&)           ;
        [[nodiscard]] const std::chrono::milliseconds getLastDuration(const dbuniq&)            ;
        [[nodiscard]] const std::chrono::milliseconds calculateAverageDuration()                ;
        [[nodiscard]] const int                       getUsageCount(const dbuniq&)              ;
                      void                            printConnectionStats()                    ;

        // Connections map -------------------------------------------------------------------------
        struct ConnectionMetrics
        {
          std::chrono::steady_clock::time_point       start_time                                ;
          std::chrono::steady_clock::time_point       end_time                                  ;
          std::chrono::milliseconds                   total_duration        {0}                 ;
          std::chrono::milliseconds                   last_duration         {0}                 ;
          std::chrono::steady_clock::time_point       last_acquired                             ;
          int                                         usage_count           {0}                 ;
          bool                                        is_acquired           {false}             ;

          ConnectionMetrics() = default;
          ConnectionMetrics(ConnectionMetrics&&) noexcept = default;
          ConnectionMetrics(const ConnectionMetrics&) = delete;
        };

        struct UniquePtrHash
        {
          std::size_t operator()(const dbuniq& ptr) const
          {
            return std::hash<dbconn*>()(ptr.get());
          }
        };

        // Equality comparator for unique_ptr<pqxx::connection>
        struct UniquePtrEqual
        {
          bool operator()(const dbuniq& a,
                          const dbuniq& b) const
          {
            return a.get() == b.get();
          }
        };

        struct PoolData {
          std::unordered_map<dbuniq,
                             ConnectionMetrics,
                             UniquePtrHash,
                             UniquePtrEqual>          connections;

          std::vector<decltype(connections)::iterator>connectionsToRemove;

          std::mutex                                  connections_mutex;

          PoolData(size_t capacity)
          {
            connections.reserve(capacity);
            connectionsToRemove.reserve(capacity);
          }
        };

        std::atomic<bool>                             connectionRefused                         ;



        // static inline void                              cleanupIdleConnections()          ;
        // void                                            startCleanupTask()                ;



      public:
        void                                          closeConnection(const dbuniq&)            ;
        void                                          closeConnection(std::optional<Database::ConnectionWrapper>&);

        struct Metrics {
          std::size_t                                 available                                 ;
          std::size_t                                 total_known                               ;
          std::size_t                                 creations                                 ;
          std::size_t                                 failures                                  ;
          std::size_t                                 validation_errors                         ;
        };

        Pool();
        ~Pool();

        // [[nodiscard]] const Metrics                   getMetrics()                const noexcept;
        [[nodiscard]] const std::size_t               getAvailableConnections()         noexcept;
        [[nodiscard]] const std::size_t               getTotalConnections()             noexcept;
        [[nodiscard]] PoolData&                       getPoolData()                     noexcept;

        std::optional<Database::ConnectionWrapper>    acquire()                                 ;
        void                                          releaseConnection(dboptuniqptr)           ;
    };

  private:
    std::unique_ptr<IRepository>                      database                                  ;
    std::unique_ptr<IRepository>                      chooseDatabase(DatabaseType type)         ;
    std::unique_ptr<Pool>                             pool                                      ;

  public:

    Database();
    ~Database() override;

    std::optional<Database::ConnectionWrapper>        acquire()                                 ;
    void                                              releaseConnection(dboptuniqptr)           ;

    std::optional<std::string> echoString(std::string str) override
    {
      return this->database->echoString(str);
    };
};
