#pragma once

#include <libcaos/config.hpp>
#include "../../IRepository.hpp"
#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <spdlog/spdlog.h>
#include <terminal_options.hpp>
#include <environment.hpp>
#include <functional>
#include <shared_mutex>

// Nel header file (es. PostgreSQL.hpp)
#include <pqxx/except>
#include <functional>

#define CAOS_POSTGRESQL_QUERY_WITH_CONNECTION_GUARD(conn, CODE) \
  ([&]() {                                                      \
    try {                                                       \
      auto result = (CODE)();                                   \
      return result;                                            \
    }                                                           \
    catch (const pqxx::broken_connection& e) {                  \
      if (conn) {                                               \
        PostgreSQL::Pool::closeConnection(*((conn).getRaw()));  \
      }                                                         \
      throw PostgreSQL::broken_connection(e.what());            \
    }                                                           \
  }())

#define CAOS_POSTGRESQL_CLOSE_CONNECTION()                      \
  PostgreSQL::Pool::closeConnection(*(connection_opt.value().getRaw()));


class PostgreSQL final: public IRepository
{
  private:
    class ConnectionWrapper;

    class Pool
    {
      private:
        static inline TerminalOptions*                  terminalPtr           = nullptr   ;
        static inline Environment*                      environmentRef        = nullptr   ;
        static std::condition_variable                  shutdown_cv_                      ;
        static std::mutex                               shutdown_mutex_                   ;
        static std::condition_variable                  condition                         ;
        static std::atomic<bool>                        running_                          ;
        static std::atomic<bool>                        offline_                          ;
        static std::thread                              healthCheckThread_                ;

        struct ConnectionMetrics
        {
          std::chrono::steady_clock::time_point         start_time                        ;
          std::chrono::steady_clock::time_point         end_time                          ;
          std::chrono::milliseconds                     total_duration        {0}         ;
          std::chrono::milliseconds                     last_duration         {0}         ;
          std::chrono::steady_clock::time_point         last_acquired                     ;
          int                                           usage_count           {0}         ;
          bool                                          is_acquired           {false}     ;

          ConnectionMetrics() = default;
          ConnectionMetrics(ConnectionMetrics&&) noexcept = default;
          ConnectionMetrics(const ConnectionMetrics&) = delete;
        };

        struct UniquePtrHash
        {
          std::size_t operator()(const std::unique_ptr<pqxx::connection>& ptr) const
          {
            return std::hash<pqxx::connection*>()(ptr.get());
          }
        };

        // Equality comparator for unique_ptr<pqxx::connection>
        struct UniquePtrEqual {
            bool operator()(const std::unique_ptr<pqxx::connection>& a,
                            const std::unique_ptr<pqxx::connection>& b) const
            {
              return a.get() == b.get();
            }
        };

        static std::unordered_map<std::unique_ptr<pqxx::connection>,
                                  ConnectionMetrics,
                                  UniquePtrHash,
                                  UniquePtrEqual>       connections                       ;

        static std::vector<decltype(connections)::iterator>  connectionsToRemove;

        static std::mutex                               connections_mutex                 ;
        // static std::shared_mutex                        connections_mutex                 ;

        struct config_s
        {
          std::string                                   user                              ;
          std::string                                   pass                              ;
          std::string                                   host                              ;
          std::uint16_t                                 port                  {0}         ;
          std::string                                   name                              ;
          std::size_t                                   poolsizemin           {0}         ;
          std::size_t                                   poolsizemax           {0}         ;
          std::uint32_t                                 poolwait              {0}         ;
          std::chrono::milliseconds                     pooltimeout           {0}         ;
          std::size_t                                   keepalives            {0}         ;
          std::size_t                                   keepalives_idle       {0}         ;
          std::size_t                                   keepalives_interval   {0}         ;
          std::size_t                                   keepalives_count      {0}         ;
          std::size_t                                   connect_timeout       {0}         ;
          std::string                                   connection_string                 ;
          std::chrono::milliseconds                     max_wait              {0}         ;
          std::chrono::milliseconds                     health_check_interval {0}         ;
        };
        static config_s config;

        // Setters ---------------------------------------------------------------------------------
        static void                                     setUser()                         ;
        static void                                     setPass()                         ;
        static void                                     setHost()                         ;
        static void                                     setPort()                         ;
        static void                                     setName()                         ;
        static void                                     setPoolSizeMin()                  ;
        static void                                     setPoolSizeMax()                  ;
        static void                                     setPoolWait()                     ;
        static void                                     setPoolTimeout()                  ;
        static void                                     setKeepAlives()                   ;
        static void                                     setKeepAlivesIdle()               ;
        static void                                     setKeepAlivesInterval()           ;
        static void                                     setKeepAlivesCount()              ;
        static void                                     setConnectTimeout()               ;
        static void                                     setMaxWait()                      ;
        static void                                     setHealthCheckInterval()          ;
        static void                                     setConnectStr()           noexcept;

        // Getters ---------------------------------------------------------------------------------
        [[nodiscard]] static std::string                getUser()                 noexcept;
        [[nodiscard]] static std::string                getPass()                 noexcept;
        [[nodiscard]] static std::string                getHost()                 noexcept;
        [[nodiscard]] static std::uint16_t              getPort()                 noexcept;
        [[nodiscard]] static std::string                getName()                 noexcept;
        [[nodiscard]] static std::size_t                getPoolSizeMin()          noexcept;
        [[nodiscard]] static std::size_t                getPoolSizeMax()          noexcept;
        [[nodiscard]] static std::uint32_t              getPoolWait()             noexcept;
        [[nodiscard]] static std::chrono::milliseconds  getPoolTimeout()          noexcept;
        [[nodiscard]] static std::size_t                getKeepAlives()           noexcept;
        [[nodiscard]] static std::size_t                getKeepAlivesIdle()       noexcept;
        [[nodiscard]] static std::size_t                getKeepAlivesInterval()   noexcept;
        [[nodiscard]] static std::size_t                getKeepAlivesCount()      noexcept;
        [[nodiscard]] static std::size_t                getConnectTimeout()       noexcept;
        [[nodiscard]] static std::string&               getConnectStr()           noexcept;
        [[nodiscard]] static std::chrono::milliseconds  getMaxWait()              noexcept;
        [[nodiscard]] static std::chrono::milliseconds  getHealthCheckInterval()  noexcept;
        [[nodiscard]] static bool                       isDevOrTestEnv()          noexcept;

        static inline std::size_t                       init(std::size_t = 0)             ;

        // [[nodiscard]] static bool                       validateConnection(const std::unique_ptr<pqxx::connection>&)     ;
        [[nodiscard]] static bool                       validateConnection(const std::unique_ptr<pqxx::connection>& connection);
        [[nodiscard]] static bool                       createConnection(std::size_t&)           ;
        static void                                     healthCheckLoop()                 ;
        static std::optional<const std::unique_ptr<pqxx::connection>*>                            acquireConnection()               ;
        void                                            handleInvalidConnection(const std::unique_ptr<pqxx::connection>&);
        static void                                     cleanupMarkedConnections();
        // static inline void                              cleanupIdleConnections()          ;
        // void                                            startCleanupTask()                ;

        static std::chrono::milliseconds                getTotalDuration(const std::unique_ptr<pqxx::connection>&)      ;
        static std::chrono::milliseconds                getLastDuration(const std::unique_ptr<pqxx::connection>&)       ;
        static std::chrono::milliseconds                calculateAverageDuration()        ;
        static int                                      getUsageCount(const std::unique_ptr<pqxx::connection>&)         ;
        static void                                     printConnectionStats()            ;
        static bool inline                              checkPoolSize(std::size_t&)       ;

      public:
        static void                                     closeConnection(const std::unique_ptr<pqxx::connection>&)       ;
        static void                                     closeConnection(std::optional<PostgreSQL::ConnectionWrapper>& connection);

        struct Metrics {
          std::size_t                                   available                         ;
          std::size_t                                   total_known                       ;
          std::size_t                                   creations                         ;
          std::size_t                                   failures                          ;
          std::size_t                                   validation_errors                 ;
        };

        Pool();
        ~Pool();

        [[nodiscard]] static Metrics            getMetrics()                      noexcept;
        [[nodiscard]] static std::size_t        getAvailableConnections()         noexcept;
        [[nodiscard]] static std::size_t        getTotalConnections()             noexcept;

        static std::optional<PostgreSQL::ConnectionWrapper> acquire();
        static void                                    releaseConnection(std::optional<const std::unique_ptr<pqxx::connection>*>);
    };

    std::unique_ptr<Pool>                       pool                                      ;
    std::atomic<bool>                           running_{true}                            ;

  public:
    class broken_connection : public std::runtime_error {
      public:
        explicit broken_connection(const std::string& msg)
          : std::runtime_error(msg) {}
    };

    PostgreSQL();

    ~PostgreSQL() override;

    std::optional<PostgreSQL::ConnectionWrapper> acquire();
    void                                        releaseConnection(std::optional<const std::unique_ptr<pqxx::connection>*>);

    std::optional<std::string> echoString(std::string str) override {
      static constexpr const char* fName = "PostgreSQL::echoString";

      if(!running_.load(std::memory_order_relaxed))
      {
        return std::nullopt;
      }

      auto connection_opt = this->acquire();

      try
      {
        if (connection_opt)
        {
          PostgreSQL::ConnectionWrapper& connection = connection_opt.value();

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

        // throw pqxx::broken_connection("Database connection unavailable - cannot acquire connection from pool");

      }
      catch (const pqxx::broken_connection& e)
      {
        // PostgreSQL::Pool::closeConnection(*(connection_opt.value().getRaw()));
        // CAOS_POSTGRESQL_CLOSE_CONNECTION()
        // spdlog::error("[{}] 2Broken connection: {}", fName, e.what());
        throw;
      }
      catch (const pqxx::sql_error& e)
      {
        spdlog::error("[{}] SQL error during connection creation: {}", fName, e.what());
      }
      catch (const std::exception& e)
      {
        spdlog::critical("[{}] Exception during connection creation: {}", fName, e.what());
      }
      catch(...)
      {
        spdlog::error("Can't execute echoString() query");
      }

      return std::nullopt;
    }

  private:
    class ConnectionWrapper
    {
      public:
        ConnectionWrapper(std::optional<const std::unique_ptr<pqxx::connection>*> conn_ptr,
                          std::function<void(std::optional<const std::unique_ptr<pqxx::connection>*>)> release_func)
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

        pqxx::connection& operator*() const
        {
          return *(connection_ptr.value()->get());
        }

        pqxx::connection* operator->() const
        {
          return connection_ptr.has_value() ? connection_ptr.value()->get() : nullptr;
        }

        [[nodiscard]] pqxx::connection* get() const
        {
          return connection_ptr.has_value() ? connection_ptr.value()->get() : nullptr;
        }

        [[nodiscard]] const std::unique_ptr<pqxx::connection>* getRaw() const
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
          return connection_ptr.has_value() && !released && connection_ptr.value()->get() != nullptr;
        }

      private:
        std::optional<const std::unique_ptr<pqxx::connection>*> connection_ptr;
        std::function<void(std::optional<const std::unique_ptr<pqxx::connection>*>)> release_func;
        bool released{false};
        std::function<void()> cleanup_handler_;
    };
};





