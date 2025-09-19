#pragma once

#include "../IRepository.hpp"

#include <pqxx/pqxx>
#include <pqxx/except>

#include <spdlog/spdlog.h>
#include <terminal_options.hpp>
#include <environment.hpp>
#include <mutex>
#include <queue>
#include <functional>
#include <optional>
#include <memory>

enum class DatabaseType: std::uint8_t {PostgreSQL=0, EOE};                                          // End Of Enum

class Database : public IRepository
{
  public:
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
          return connection_ptr.has_value() && !released && connection_ptr.value() != nullptr;
        }

      private:
        std::optional<const std::unique_ptr<pqxx::connection>*> connection_ptr;
        std::function<void(std::optional<const std::unique_ptr<pqxx::connection>*>)> release_func;
        bool released{false};
        std::function<void()> cleanup_handler_;
    };

    class Pool
    {
      private:
        TerminalOptions*                  terminalPtr           = nullptr   ;
        Environment*                      environmentRef        = nullptr   ;
        std::condition_variable                  shutdown_cv_                      ;
        std::mutex                               shutdown_mutex_                   ;
        std::condition_variable                  condition                         ;
        std::atomic<bool>                        running_                          ;
        std::thread                              healthCheckThread_                ;
      protected:
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

        struct PoolData {
          std::unordered_map<std::unique_ptr<pqxx::connection>,
                             ConnectionMetrics,
                             UniquePtrHash,
                             UniquePtrEqual>            connections;

          std::vector<decltype(connections)::iterator>  connectionsToRemove;

          std::mutex                                    connections_mutex;

          PoolData(size_t capacity)
          {
            connections.reserve(capacity);
            connectionsToRemove.reserve(capacity);
          }
        };

        std::atomic<bool>                        connectionRefused                 ;
      protected:
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
        config_s config;
      private:
        // Setters ---------------------------------------------------------------------------------
        void                                     setUser()                         ;
        void                                     setPass()                         ;
        void                                     setHost()                         ;
        void                                     setPort()                         ;
        void                                     setName()                         ;
        void                                     setPoolSizeMin()                  ;
        void                                     setPoolSizeMax()                  ;
        void                                     setPoolWait()                     ;
        void                                     setPoolTimeout()                  ;
        void                                     setKeepAlives()                   ;
        void                                     setKeepAlivesIdle()               ;
        void                                     setKeepAlivesInterval()           ;
        void                                     setKeepAlivesCount()              ;
        void                                     setConnectTimeout()               ;
        void                                     setMaxWait()                      ;
        void                                     setHealthCheckInterval()          ;
        virtual void                                    setConnectStr()           noexcept;

      protected:
        // Getters ---------------------------------------------------------------------------------
        [[nodiscard]] std::string                getUser()                 noexcept;
        [[nodiscard]] std::string                getPass()                 noexcept;
        [[nodiscard]] std::string                getHost()                 noexcept;
        [[nodiscard]] std::uint16_t              getPort()                 noexcept;
        [[nodiscard]] std::string                getName()                 noexcept;
        [[nodiscard]] std::size_t                getPoolSizeMin()          noexcept;
        [[nodiscard]] std::size_t                getPoolSizeMax()          noexcept;
        [[nodiscard]] std::uint32_t              getPoolWait()             noexcept;
        [[nodiscard]] std::chrono::milliseconds  getPoolTimeout()          noexcept;
        [[nodiscard]] std::size_t                getKeepAlives()           noexcept;
        [[nodiscard]] std::size_t                getKeepAlivesIdle()       noexcept;
        [[nodiscard]] std::size_t                getKeepAlivesInterval()   noexcept;
        [[nodiscard]] std::size_t                getKeepAlivesCount()      noexcept;
        [[nodiscard]] std::size_t                getConnectTimeout()       noexcept;
        [[nodiscard]] std::string&               getConnectStr()           noexcept;
        [[nodiscard]] std::chrono::milliseconds  getMaxWait()              noexcept;
        [[nodiscard]] std::chrono::milliseconds  getHealthCheckInterval()  noexcept;
        [[nodiscard]] bool                       isDevOrTestEnv()          noexcept;
        [[nodiscard]] bool                checkPoolSize(std::size_t&) noexcept;


      private:
        inline std::size_t                       init(std::size_t = 0)             ;
      protected:
        [[nodiscard]] virtual bool                       validateConnection(const std::unique_ptr<pqxx::connection>& connection);
        [[nodiscard]] virtual bool                      createConnection(std::size_t&)    ;
      private:

        void                                     healthCheckLoop()                 ;
        std::optional<const std::unique_ptr<pqxx::connection>*> acquireConnection();
        void                                            handleInvalidConnection(const std::unique_ptr<pqxx::connection>&);
        void                                     cleanupMarkedConnections();
        // static inline void                              cleanupIdleConnections()          ;
        // void                                            startCleanupTask()                ;

        std::chrono::milliseconds                getTotalDuration(const std::unique_ptr<pqxx::connection>&)      ;
        std::chrono::milliseconds                getLastDuration(const std::unique_ptr<pqxx::connection>&)       ;
        std::chrono::milliseconds                calculateAverageDuration()        ;
        int                                      getUsageCount(const std::unique_ptr<pqxx::connection>&)         ;
        void                                     printConnectionStats()            ;


      public:
        virtual void                                    closeConnection(const std::unique_ptr<pqxx::connection>&)       ;
        virtual void                                    closeConnection(std::optional<Database::ConnectionWrapper>& connection);

        struct Metrics {
          std::size_t                                   available                         ;
          std::size_t                                   total_known                       ;
          std::size_t                                   creations                         ;
          std::size_t                                   failures                          ;
          std::size_t                                   validation_errors                 ;
        };

        Pool();
        ~Pool();

        [[nodiscard]] Metrics                    getMetrics()              noexcept;
        [[nodiscard]] std::size_t                getAvailableConnections() noexcept;
        [[nodiscard]] std::size_t                getTotalConnections()     noexcept;
        [[nodiscard]] PoolData&                  getPoolData()             noexcept;

        std::optional<Database::ConnectionWrapper> acquire()                     ;
        void                                     releaseConnection(std::optional<const std::unique_ptr<pqxx::connection>*>);
    };

  private:
    std::unique_ptr<IRepository> database;

    std::unique_ptr<IRepository> chooseDatabase(DatabaseType type);

  public:
    std::unique_ptr<Pool>                               pool                              ;

    Database();
    ~Database() override;

    std::optional<Database::ConnectionWrapper>          acquire()                         ;
    void                                                releaseConnection(std::optional<const std::unique_ptr<pqxx::connection>*>);

    std::optional<std::string> echoString(std::string str) override
    {
      return this->database->echoString(str);
    };
};
