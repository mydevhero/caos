/*
 * libcaos.cpp
 *
 *  Created on: 21 lug 2025
 *      Author: mrbi314
 */

#include "include/libcaos.hpp"
#include <arpa/inet.h>                                                                              // Validate IP address
#include <optional>
#include <sstream>








/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * class caos::Server::Network
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
constexpr const char* caos::Server::Network::IPVersionName(IPVersion ipv)
{
  if (ipv < IPVersion::EOE)
  {
    return IPVersionChar[static_cast<size_t>(ipv)];
  }

  throw std::out_of_range("IP protocol is unmanaged");
}



caos::Server::Network::Network()
  : port(0),
    ipversion(IPVersion::unmanaged)
{
  spdlog::trace("Running Network constructor");

  this->setAddress(); // Set ipversion as well
  this->setPort();
}



std::string caos::Server::Network::getAddress()       const noexcept                                // Getter Network Address
{
  return this->address;
}



std::uint16_t caos::Server::Network::getPort()        const noexcept                                // Getter Network Port
{
  return this->port;
}



caos::Server::Network::IPVersion caos::Server::Network::getIPVersion()       const noexcept         // Getter IP Version
{
  return this->ipversion;
}



const char* caos::Server::Network::getIPVersionName() const                                         // Getter IP Version Name
{
  return IPVersionName(this->ipversion);
}



void caos::Server::Network::setAddress()                                                            // Setter Network Address & IP Version

{
  spdlog::trace("Setting network address");

  static constexpr const char* fName = "caos::Server::Network::setAddress";

  try
  {
    const char* envAddrStr = std::getenv(CAOS_ENV_BINDTOADDRESS_NAME);

    if (TerminalOptions::get_instance().has(CAOS_OPT_BINDTOADDRESS_NAME))
    {
      this->address = TerminalOptions::get_instance().get<std::string>(CAOS_OPT_BINDTOADDRESS_NAME);
    }
    else if (envAddrStr!=nullptr)
    {
      this->address = envAddrStr;
    }
    else
    {
      this->address = CAOS_DEFAULT_BINDTOADDRESS_VALUE;
    }

    struct sockaddr_in sa4={};
    struct sockaddr_in6 sa6={};

    if (this->address.empty())
    {
      throw std::invalid_argument("BINDTOADDRESS empty!");
    }

    // Validate ipV4
    if (inet_pton(AF_INET, this->address.c_str(), &(sa4.sin_addr)) == 1)
    {
      this->ipversion = IPVersion::IPv4;

      spdlog::trace("Network address (IPv4) is: {}",this->address);

      return;
    }

    // Validate ipV6
    if (inet_pton(AF_INET6, this->address.c_str(), &(sa6.sin6_addr)) == 1)
    {
      this->ipversion = IPVersion::IPv6;

      spdlog::trace("Network address (IPv6) is: {}",this->address);

      return;
    }

    throw std::invalid_argument("Invalid Address: " + this->address);
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}", fName, e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}", fName, e.what());
    std::exit(1);
  }
}



void caos::Server::Network::setPort()                                                               // Setter Network Port
{
  spdlog::trace("Setting network port");

  static constexpr const char* fName = "caos::Server::Network::setPort";
  std::string userMsg;

  try
  {
    const char* envPortStr = std::getenv(CAOS_ENV_BINDTOPORT_NAME);

    if (TerminalOptions::get_instance().has(CAOS_OPT_BINDTOPORT_NAME))
    {
      this->port = TerminalOptions::get_instance().get<std::uint16_t>(CAOS_OPT_BINDTOPORT_NAME);
    }
    else if (envPortStr != nullptr)
    {
      this->port = std::stoi(envPortStr);
    }
    else
    {
      this->port = CAOS_DEFAULT_BINDTOPORT_VALUE;
    }

    userMsg = "TCP Port range from 1024 to 65535";

    if (this->port == 0)
    {
      throw std::invalid_argument("BINDTOPORT unset or 0!");
    }

    if(this->port < CAOS_UNPRIVILEGED_PORT_MIN || this->port > CAOS_UNPRIVILEGED_PORT_MAX)
    {
      throw std::out_of_range("TCP Port out of range: " + std::to_string(this->port));
    }

    this->port = static_cast<std::uint16_t>(this->port);

    spdlog::trace("Network port is: {}", this->port);
  }
  catch(const std::out_of_range& e)
  {
    spdlog::critical("[{}] : [{}] : {}", fName, userMsg, e.what());
    std::exit(1);
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : [{}] : {}", fName, userMsg, e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}", fName, e.what());
    std::exit(1);
  }
}
/* -------------------------------------------------------------------------------------------------
 * class caos::Server::Network
 * -----------------------------------------------------------------------------------------------*/






/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * class caos::Server::Thread
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
caos::Server::Thread::Thread() : threads(0)
{
  this->set();
}



std::uint16_t caos::Server::Thread::get() const noexcept                                            // Getter Thread count
{
  return this->threads;
}



void caos::Server::Thread::set()                                                                    // Setter Thread count
{
  spdlog::trace("Setting threads count");

  static constexpr const char* fName = "caos::Server::Thread::set";
  std::string userMsg;

  try
  {
    const char* threadsStr = std::getenv(CAOS_ENV_THREADS_NAME);

    if (TerminalOptions::get_instance().has(CAOS_OPT_THREADS_NAME))
    {
      this->threads = TerminalOptions::get_instance().get<std::uint16_t>(CAOS_OPT_THREADS_NAME);
    }
    else if (threadsStr!=nullptr)
    {
      this->threads = std::stoi(threadsStr);
    }
    else
    {
      this->threads = CAOS_DEFAULT_THREADS_ON_UNSET_VALUE;
    }

    if (this->threads == 0)
    {
      // Adjust for ENV::dev or ENV::test
      if (Environment::get_instance().getEnv() == Environment::ENV::dev || Environment::get_instance().getEnv() == Environment::ENV::test)
      {
        this->threads = CAOS_DEFAULT_THREADS_ENV_DEV_OR_TEST_VALUE;

        spdlog::info("CAOS_THREADS value changed because APP_ENV is dev or test and no value is sent by terminal nor env");
      }
    }

    std::uint16_t max_threads = std::thread::hardware_concurrency();

    userMsg = "Threads range is from 1 to " + std::to_string(max_threads);

    if (this->threads == 0 && max_threads > 0)                                                      // Threads isnt filled by env, opt nor default?
    {
      spdlog::trace("Setting max threads count");

      this->threads = max_threads;
    }

    if ( (this->threads == 0) || (max_threads > 0 && this->threads > max_threads) )
    {
      std::string techMsg = "Threads count invalid: " + std::to_string (this->threads);

      throw std::out_of_range(techMsg);
    }

    this->threads = static_cast<std::uint16_t>(this->threads);

    spdlog::trace("Threads count is: {}", this->threads);
  }
  catch(const std::out_of_range& e)
  {
    spdlog::critical("[{}] : [{}] : {}", fName, userMsg, e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}", fName, e.what());
    std::exit(1);
  }
}
/* -------------------------------------------------------------------------------------------------
 * class caos::Server::Thread
 * -----------------------------------------------------------------------------------------------*/





/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * class caos::Log
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
const std::unordered_map<std::string, LogSeverity> caos::Log::LogSeverityMapToString =
{
  { "trace"   , LogSeverity::trace    },
  { "debug"   , LogSeverity::debug    },
  { "info"    , LogSeverity::info     },
  { "warn"    , LogSeverity::warn     },
  { "error"   , LogSeverity::err      },
  { "critical", LogSeverity::critical },
  { "off"     , LogSeverity::off      }
};



constexpr const char* caos::Log::LogSeverity2String(LogSeverity severity)
{
  if (severity < LogSeverity::n_levels)
  {
    return SeverityChar.at(static_cast<size_t>(severity));
  }

  throw std::out_of_range("Log severity is unnknown");
}



inline LogSeverity caos::Log::String2LogSeverity(const std::string& severityStr)
{
  auto it = LogSeverityMapToString.find(severityStr);

  if (it != LogSeverityMapToString.end())
  {
    return it->second;
  }

  throw std::out_of_range("Invalid log severity string: " + severityStr);
}



inline void caos::Log::init()
{
  logger.terminal.pattern = CAOS_LOG_TERMINAL_PATTERN;
  logger.rotating.pattern = CAOS_LOG_ROTATING_PATTERN;
  logger.syslog.pattern   = CAOS_LOG_SYSLOG_PATTERN;

  try
  {
    SPDLOG_TRACE("Init logs");

    spdlog::init_thread_pool(CAOS_LOG_QUEUE, CAOS_LOG_THREAD_COUNT);

    // Terminal
    logger.terminal.sink= std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    logger.terminal.sink->set_pattern (logger.terminal.pattern);
    SPDLOG_TRACE("Terminal sink created");

    // Rotating file
    logger.rotating.sink =
      std::make_shared<spdlog::sinks::rotating_file_sink_mt>(CAOS_LOG_FILE,
                                                             CAOS_LOG_FILE_DIMENSION,
                                                             CAOS_LOG_NUMBER);
    logger.rotating.sink->set_pattern (logger.rotating.pattern);
    SPDLOG_TRACE("Rotating file sink created");

    // Syslog
    logger.syslog.sink= std::make_shared<spdlog::sinks::syslog_sink_mt>(libname, LOG_PID, LOG_USER, true);
    logger.syslog.sink->set_pattern (logger.syslog.pattern);
    SPDLOG_TRACE("Syslog sink created");

    // Combined
    logger.combined.sink =
      std::make_shared<spdlog::async_logger>(
        libname,
        spdlog::sinks_init_list{logger.terminal.sink,logger.rotating.sink,logger.syslog.sink},
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block
      );

    logger.combined.sink->set_level(logger.combined.severity.level);

    spdlog::register_logger(logger.combined.sink);
    spdlog::set_default_logger(logger.combined.sink);

    spdlog::flush_every(std::chrono::seconds(3));

    SPDLOG_TRACE("Combined sink created and set it as default");
  }
  catch (const spdlog::spdlog_ex &ex)
  {
    SPDLOG_CRITICAL("Errore nell'inizializzazione del log: {}", ex.what());
    std::exit(1);
  }
}



caos::Log::Log()
{
  SPDLOG_TRACE("Starting logger");

  this->setSeverity();                                                                              // Validazione a assegnazione Log

  this->init();

  SPDLOG_TRACE("Logger ready");
}



void caos::Log::setSeverity()                                                                       // Setter Log Severity & Severity Name
{
  SPDLOG_TRACE("Setting log severity");

  static constexpr const char* fName = "caos::Log::setSeverity";

  try
  {
    const char* severity_name = std::getenv(CAOS_ENV_LOG_SEVERITY_NAME);

    if (TerminalOptions::get_instance().has(CAOS_OPT_LOG_SEVERITY_NAME))
    {
      this->logger.combined.severity.name = TerminalOptions::get_instance().get<std::string>(CAOS_OPT_LOG_SEVERITY_NAME);
    }
    else if (severity_name!=nullptr)
    {
      this->logger.combined.severity.name = severity_name;
    }

    // Set by default based on environment
    if (this->logger.terminal.severity.name.empty() || this->logger.syslog.severity.name.empty())
    {
      if (Environment::get_instance().getEnv() == Environment::ENV::dev)
      {
        this->logger.combined.severity.name = CAOS_LOG_SEVERITY_ON_DEV_VALUE;
      }
      else if (Environment::get_instance().getEnv() == Environment::ENV::test)
      {
        this->logger.combined.severity.name = CAOS_LOG_SEVERITY_ON_TEST_VALUE;
      }
      else if (Environment::get_instance().getEnv() == Environment::ENV::production)
      {
        this->logger.combined.severity.name = CAOS_LOG_SEVERITY_ON_PRODUCTION_VALUE;
      }
    }

    SPDLOG_TRACE("Log severity is \"{}\"",this->logger.combined.severity.name);
    this->logger.combined.severity.level = String2LogSeverity(this->logger.combined.severity.name);
  }
  catch(const std::out_of_range& e)
  {
    std::string userMsg = R"(Allowed log severity is "trace", "debug", "info", "warn", "error", "critical", "off")";
    SPDLOG_CRITICAL("[{}] : [{}] : {}", fName, userMsg, e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    SPDLOG_CRITICAL("[{}] : {}", fName, e.what());
    std::exit(1);
  }
}
/* -------------------------------------------------------------------------------------------------
 * class caos::Log
 * -----------------------------------------------------------------------------------------------*/




/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * class caos::Server
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
caos::Server::Server()
  : terminalPtr(&TerminalOptions::get_instance()),
    environmentRef(Environment::get_instance()),
    config { Network(), Thread() }
{}



std::string   caos::Server::getAddress()      const noexcept
{
  return this->config.network.getAddress();
}



std::uint16_t caos::Server::getPort()         const noexcept
{
  return this->config.network.getPort();
}



std::uint16_t caos::Server::getThreadCount()  const noexcept
{
  return this->config.thread.get();
}



caos::Server::Network::IPVersion caos::Server::getIPVersion()      const noexcept
{
  return this->config.network.getIPVersion();
}



const char* caos::Server::getIPVersionName()  const
{
  return this->config.network.getIPVersionName();
}
/* -------------------------------------------------------------------------------------------------
 * class caos::Server
 * -----------------------------------------------------------------------------------------------*/





/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * class implementazione caos
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
void caos::PRINT_LOGO()                 noexcept
{
  std::cout << _logo << "\n";
}

void caos::PRINT_BUILD_COUNT()          noexcept
{
  std::cout << "BUILD COUNT: " << CAOS_BUILD_VERSION << "\n";
}

void caos::PRINT_BUILD_TIMESTAMP()      noexcept
{
  std::cout << "BUILD TIMESTAMP: " << CAOS_BUILD_TIMESTAMP << "\n";
}

void caos::PRINT_BUILD_WITH_TIMESTAMP() noexcept
{
  std::cout << "BUILD: " << CAOS_BUILD_VERSION_WITH_TIMESTAMP << "\n";
}

void caos::PRINT_HEADER()               noexcept
{
  caos::PRINT_LOGO();
  caos::PRINT_BUILD_COUNT();
  std::cout << "\n\n";
}

caos::~caos()
{
  // spdlog::trace("Destroying caos");
  this->repository.reset();
  spdlog::shutdown();
}


caos::caos(int argc, char* argv[], std::unique_ptr<Database> db)
:
  terminalPtr(&TerminalOptions::get_instance(argc, argv)),
  environmentRef(Environment::get_instance()),
  repository(std::make_unique<Cache>(std::make_unique<Database>())
)
{
  caos::PRINT_HEADER();
}
/* -------------------------------------------------------------------------------------------------
 * class caos
 * -----------------------------------------------------------------------------------------------*/
