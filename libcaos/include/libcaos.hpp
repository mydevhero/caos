/*
 * libcaos.hpp
 *
 *  Created on: 12 lug 2025
 *      Author: mrbi314
 */

#pragma once

#include <libcaos/config.hpp>
#include <terminal_options.hpp>
#include <environment.hpp>

// Logger - https://github.com/gabime/spdlog/wiki/
#include "spdlog/async.h"                                                                           //support for async logging
#include "spdlog/sinks/stdout_color_sinks.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include "spdlog/sinks/syslog_sink.h"


// #include <include/repository.hpp>

#include <../Middleware/Middleware.hpp>



class caos
{
  public:

    /* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * Log configuration & log utilities manager
     * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    class Log
    {
      private:
        struct LoggerNameDestinationSeverity
        {
          LogSeverity level;
          std::string name;
        };

        template<typename T>
        struct LoggerNameDestination {
            std::shared_ptr<T> sink;
            LoggerNameDestinationSeverity severity;
            std::string pattern;
        };

        struct Logger
        {
            LoggerNameDestination<spdlog::sinks::stdout_color_sink_mt> terminal;
            LoggerNameDestination<spdlog::sinks::rotating_file_sink_mt> rotating;
            LoggerNameDestination<spdlog::sinks::syslog_sink_mt> syslog;
            LoggerNameDestination<spdlog::async_logger> combined;
        };

        Logger logger;

        void setSeverity();

        static constexpr std::array<const char*, static_cast<std::size_t>(LogSeverity::n_levels)>
          SeverityChar = { "trace", "debug", "info", "warn", "error", "critical", "off" };

        static const std::unordered_map<std::string, LogSeverity> LogSeverityMapToString;
        static constexpr inline const char* LogSeverity2String(LogSeverity severity);
        static inline LogSeverity String2LogSeverity(const std::string& severityStr);

        inline void init();
      public:
         Log();
    };
    /* ---------------------------------------------------------------------------------------------
     *
     * -------------------------------------------------------------------------------------------*/



    /** ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * @brief Server manager
     * @details Manage and own configuration about network, threads
     * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
    class Server {
      private:
        // Threads configuration manager -----------------------------------------------------------
        class Thread
        {
          private:
            std::uint16_t           threads;

            void                    set();

          public:
             Thread();

            [[nodiscard]] std::uint16_t get() const noexcept;
        };
        // -----------------------------------------------------------------------------------------



        // Network configuration manager -----------------------------------------------------------
        class Network
        {
          public:
            enum class IPVersion:std::uint8_t { IPv4=0, IPv6, unmanaged, EOE };                     // EOE = End Of Enum

          private:
            std::string   address;
            std::uint16_t port;
            IPVersion     ipversion;

            void setAddress();
            void setPort();

            static constexpr std::array<const char*, static_cast<std::size_t>(IPVersion::EOE)>
              IPVersionChar = { "IPv4", "IPv6", "unmanaged" };

            static constexpr inline const char* IPVersionName(IPVersion ipv);
          public:

             Network();

            [[nodiscard]] std::string   getAddress()        const noexcept;
            [[nodiscard]] std::uint16_t getPort()           const noexcept;
            [[nodiscard]] IPVersion     getIPVersion()      const noexcept;
            [[nodiscard]] const char*   getIPVersionName()  const;
        };
        // -----------------------------------------------------------------------------------------

      public:
        Server();

        [[nodiscard]] std::string         getAddress()        const noexcept;
        [[nodiscard]] std::uint16_t       getPort()           const noexcept;
        [[nodiscard]] std::uint16_t       getThreadCount()    const noexcept;
        [[nodiscard]] Network::IPVersion  getIPVersion()      const noexcept;
        [[nodiscard]] const char*         getIPVersionName()  const;

      private:
        const TerminalOptions*  terminalPtr;
        const Environment&      environmentRef;

        struct config_s
        {
          Network     network;
          Thread      thread;
        };

        config_s config;
    };
    /* ---------------------------------------------------------------------------------------------
     * End of Server
     * -------------------------------------------------------------------------------------------*/

    const TerminalOptions*  terminalPtr;
    const Environment&      environmentRef;
    const Log               log;
    const Server            server;
    std::unique_ptr<Cache>  repository;

    caos(int argc,
         char* argv[],
         std::unique_ptr<Database> db = nullptr
    );

    static void PRINT_LOGO()                  noexcept;
    static void PRINT_BUILD_COUNT()           noexcept;
    static void PRINT_BUILD_TIMESTAMP()       noexcept;
    static void PRINT_BUILD_WITH_TIMESTAMP()  noexcept;
    static void PRINT_HEADER()                noexcept;
    void readTerminalOption()           const noexcept;

    ~caos();
};
