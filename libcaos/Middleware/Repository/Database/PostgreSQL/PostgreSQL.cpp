#include "PostgreSQL.hpp"
#include <arpa/inet.h>
#include <cctype> // for toupper
// #include <format>


constexpr const char* missingAlternativeDBUSER                =
  "Invalid CAOS_DEFAULT_DBUSER_ON_DEV_OR_TEST, using production DBUSER in {} environment";

constexpr const char* missingAlternativeDBPASS                =
  "Invalid CAOS_DEFAULT_DBPASS_ON_DEV_OR_TEST, using production DBPASS in {} environment";

constexpr const char* missingAlternativeDBHOST                =
  "Invalid CAOS_DEFAULT_DBHOST_ON_DEV_OR_TEST, using production DBHOST in {} environment";

constexpr const char* missingAlternativeDBPORT                =
  "Invalid CAOS_DEFAULT_DBPORT_ON_DEV_OR_TEST, using production DBPORT in {} environment";

constexpr const char* missingAlternativeDBNAME                =
  "Invalid CAOS_DEFAULT_DBNAME_ON_DEV_OR_TEST. So you have to take an hard decision tu use the database you use in {} environment";

constexpr const char* missingAlternativeDBPOOLSIZEMIN         =
  "Invalid CAOS_DEFAULT_DBPOOLSIZEMIN_ON_DEV_OR_TEST, using production DBPOOLSIZE_MIN in {} environment";

constexpr const char* missingAlternativeDBPOOLSIZEMAX         =
  "Invalid CAOS_DEFAULT_DBPOOLSIZEMAX_ON_DEV_OR_TEST, using production DBPOOLSIZE_MAX in {} environment";

constexpr const char* missingAlternativeDBPOOLWAIT            =
  "Invalid CAOS_DEFAULT_DBPOOLWAIT_ON_DEV_OR_TEST, using production DBPOOLWAIT in {} environment";

constexpr const char* missingAlternativeDBPOOLTIMEOUT         =
  "Invalid CAOS_DEFAULT_DBPOOLTIMEOUT_ON_DEV_OR_TEST, using production DBPOOLTIMEOUT in {} environment";

constexpr const char* missingAlternativeDBKEEPALIVES          =
  "Invalid CAOS_DEFAULT_DBKEEPALIVES_ON_DEV_OR_TEST, using production DBKEEPALIVES in {} environment";

constexpr const char* missingAlternativeDBKEEPALIVES_IDLE     =
  "Invalid CAOS_DEFAULT_DBKEEPALIVES_IDLE_ON_DEV_OR_TEST, using production DBKEEPALIVES_IDLE in {} environment";

constexpr const char* missingAlternativeDBKEEPALIVES_INTERVAL =
  "Invalid CAOS_DEFAULT_DBKEEPALIVES_INTERVAL_ON_DEV_OR_TEST, using production DBKEEPALIVES_INTERVAL in {} environment";

constexpr const char* missingAlternativeDBKEEPALIVES_COUNT    =
  "Invalid CAOS_DEFAULT_DBKEEPALIVES_COUNT_ON_DEV_OR_TEST, using production DBKEEPALIVES_COUNT in {} environment";

constexpr const char* missingAlternativeDBCONNECT_TIMEOUT    =
  "Invalid CAOS_DEFAULT_DBCONNECT_TIMEOUT_ON_DEV_OR_TEST, using production DBCONNECT_TIMEOUT in {} environment";

constexpr const char* missingAlternativeDBMAXWAIT    =
  "Invalid CAOS_DEFAULT_DBMAXWAIT_ON_DEV_OR_TEST, using production DBMAXWAIT in {} environment";

constexpr const char* missingAlternativeDBHEALTHCHECKINTERVAL    =
  "Invalid CAOS_DEFAULT_DBHEALTHCHECKINTERVAL_ON_DEV_OR_TEST, using production DBHEALTHCHECKINTERVAL in {} environment";

constexpr const char* defaultFinal                            =
  "{} : Setting database {} to {} in {}  environment";







// PostgreSQL::ConnectionWrapper::ConnectionWrapper(/*std::optional<std::unique_ptr<pqxx::connection>>*/ std::optional<std::reference_wrapper<std::unique_ptr<pqxx::connection>>> connection,
//                                                  releaseF release_func)
// {
//   if (connection)
//   {
//     this->connection    = connection->get();
//     this->release_func  = std::move(release_func);
//   }
// }

// PostgreSQL::ConnectionWrapper::~ConnectionWrapper()
// {
//   if (!this->released && this->connection && this->release_func)
//   {
//     this->release_func(std::move(this->connection));
//   }
// }

// PostgreSQL::ConnectionWrapper::ConnectionWrapper(PostgreSQL::ConnectionWrapper&& other) noexcept
//   : connection(std::move(other.connection)),
//     release_func(std::move(other.release_func)),
//     released(other.released)
// {
//   other.released = true;
// }

// PostgreSQL::ConnectionWrapper& PostgreSQL::ConnectionWrapper::ConnectionWrapper::operator=(PostgreSQL::ConnectionWrapper&& other) noexcept
// {
//   if (this != &other)
//   {
//     // Release current connection
//     if (!this->released && this->connection && this->release_func)
//     {
//       this->release_func(std::move(this->connection));
//     }

//     this->connection    = std::move(other.connection);
//     this->release_func  = std::move(other.release_func);
//     this->released      = other.released;
//     other.released      = true;
//   }

//   return *this;
// }

// void PostgreSQL::ConnectionWrapper::release()
// {
//   if (!this->released && this->connection && this->release_func)
//   {
//     this->release_func(std::move(this->connection));
//     this->released = true;
//   }
// }


/***************************************************************************************************
 *
 *
 * PostgreSQL::Pool() Constructor/Destructor
 *
 *
 **************************************************************************************************/
std::mutex                        PostgreSQL::Pool::shutdown_mutex_{};
std::mutex                        PostgreSQL::Pool::connections_mutex{};
std::condition_variable           PostgreSQL::Pool::condition{}                      ;
std::condition_variable           PostgreSQL::Pool::shutdown_cv_                    ;
// PostgreSQL::Pool::Stats PostgreSQL::Pool::stats{};

// std::unordered_map<std::unique_ptr<pqxx::connection>,
//                    PostgreSQL::Pool::ConnectionMetrics,
//                    PostgreSQL::Pool::UniquePtrHash,
//                    PostgreSQL::Pool::UniquePtrEqual> PostgreSQL::Pool::connections{};
std::unordered_map<std::unique_ptr<pqxx::connection>, PostgreSQL::Pool::ConnectionMetrics, PostgreSQL::Pool::UniquePtrHash, PostgreSQL::Pool::UniquePtrEqual> PostgreSQL::Pool::connections{};

PostgreSQL::Pool::config_s PostgreSQL::Pool::config = {
    .user = "",
    .pass = "",
    .host = "",
    .port = 0,
    .name = "",
    .poolsizemin = 0,
    .poolsizemax = 0,
    .poolwait = 0,
    .pooltimeout = std::chrono::milliseconds(0),
    .keepalives = 0,
    .keepalives_idle = 0,
    .keepalives_interval = 0,
    .keepalives_count = 0,
    .connect_timeout = 0,
    .connection_string = "",
    .max_wait = std::chrono::milliseconds(0),
    .health_check_interval = std::chrono::milliseconds(0)
};

std::thread PostgreSQL::Pool::healthCheckThread_{};

std::atomic<bool>                PostgreSQL::Pool::running_{true};
std::atomic<bool>               PostgreSQL::Pool::offline_              {false}     ;

PostgreSQL::Pool::Pool()
{
  PostgreSQL::Pool::terminalPtr     = &TerminalOptions::get_instance();
  PostgreSQL::Pool::environmentRef  = &Environment::get_instance();

  setUser()                 ;
  setPass()                 ;
  setHost()                 ;
  setPort()                 ;
  setName()                 ;
  setPoolSizeMin()          ;
  setPoolSizeMax()          ;
  setPoolWait()             ;
  setPoolTimeout()          ;
  setKeepAlives()           ;
  setKeepAlivesIdle()       ;
  setKeepAlivesInterval()   ;
  setKeepAlivesCount()      ;
  setConnectTimeout()       ;
  setMaxWait()              ;
  setHealthCheckInterval()  ;
  setConnectStr()           ;

  PostgreSQL::Pool::healthCheckThread_ = std::thread(&PostgreSQL::Pool::healthCheckLoop);
}





PostgreSQL::Pool::~Pool()
{
  PostgreSQL::Pool::printConnectionStats();

  PostgreSQL::Pool::running_.store(false, std::memory_order_release);

  PostgreSQL::Pool::condition.notify_all();

  if (PostgreSQL::Pool::healthCheckThread_.joinable()) {
      PostgreSQL::Pool::healthCheckThread_.join();
  }

  // std::lock_guard<std::mutex> lock(PostgreSQL::Pool::available_mutex_);

  // while (!PostgreSQL::Pool::available_.empty())
  // {
  //   auto connection = PostgreSQL::Pool::available_.front();

  //   if (connection)
  //   {
  //     try
  //     {
  //       if (connection->is_open())
  //       {
  //         connection->close();
  //       }
  //     }
  //     catch (...)
  //     {
  //       // Ignore errors while cleanup
  //     }
  //   }

  //   PostgreSQL::Pool::available_.pop();
  // }
}
/***************************************************************************************************
 *
 *
 *
 *
 *
 **************************************************************************************************/










// Utilities +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
bool PostgreSQL::Pool::isDevOrTestEnv() noexcept
{
  return PostgreSQL::Pool::environmentRef->getEnv() == Environment::ENV::dev
         || PostgreSQL::Pool::environmentRef->getEnv() == Environment::ENV::test;
}
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setUser()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setUser()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setUser";



  try
  {
    // Set primary database user for production environment ----------------------------------------
    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBUSER_NAME))
    {
      PostgreSQL::Pool::config.user = PostgreSQL::Pool::terminalPtr->get<std::string>(CAOS_OPT_DBUSER_NAME);
    }
    else if (const char* env_addr = std::getenv(CAOS_ENV_DBUSER_NAME))
    {
      PostgreSQL::Pool::config.user = env_addr;
    }
    else
    {
      PostgreSQL::Pool::config.user = CAOS_DEFAULT_DBUSER;
    }
    // ---------------------------------------------------------------------------------------------


    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.user.empty())
    {
      throw std::invalid_argument("DBUSER empty!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database user for test or dev environment -----------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::string alternative_user;

      #if defined(CAOS_DEFAULT_DBUSER_ON_DEV_OR_TEST)
        alternative_user = "" CAOS_DEFAULT_DBUSER_ON_DEV_OR_TEST;
      #endif

      if (alternative_user.empty())
      {
        // Alternative database user undefined
        spdlog::warn(missingAlternativeDBUSER, PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database user defined
        PostgreSQL::Pool::config.user = alternative_user;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }




  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "user",
               PostgreSQL::Pool::config.user,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setUser()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setPass()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setPass()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setPass";



  try
  {
    // Set primary database password for production environment ------------------------------------
    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBPASS_NAME))
    {
      PostgreSQL::Pool::config.pass = PostgreSQL::Pool::terminalPtr->get<std::string>(CAOS_OPT_DBPASS_NAME);
    }
    else if (const char* env_addr = std::getenv(CAOS_ENV_DBPASS_NAME))
    {
      PostgreSQL::Pool::config.pass = env_addr;
    }
    else
    {
      PostgreSQL::Pool::config.pass = CAOS_DEFAULT_DBPASS;
    }
    // ---------------------------------------------------------------------------------------------


    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.pass.empty())
    {
      throw std::invalid_argument("DBPASS empty!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database password for test or dev environment -------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::string alternative_pass;

      #if defined(CAOS_DEFAULT_DBPASS_ON_DEV_OR_TEST)
        alternative_pass = "" CAOS_DEFAULT_DBPASS_ON_DEV_OR_TEST;
      #endif

      if (alternative_pass.empty())
      {
        // Alternative database password undefined
        spdlog::warn(missingAlternativeDBUSER, PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database password defined
        PostgreSQL::Pool::config.pass = alternative_pass;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
                  fName,
                  "password",
                  "*******",
                  PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setPass()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setHost()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setHost()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setHost";



  try
  {
    // Set primary database host for production environment ----------------------------------------
    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBHOST_NAME))
    {
      PostgreSQL::Pool::config.host = PostgreSQL::Pool::terminalPtr->get<std::string>(CAOS_OPT_DBHOST_NAME);
    }
    else if (const char* env_addr = std::getenv(CAOS_ENV_DBHOST_NAME))
    {
      PostgreSQL::Pool::config.host = env_addr;
    }
    else
    {
      PostgreSQL::Pool::config.host = CAOS_DEFAULT_DBHOST;
    }
    // ---------------------------------------------------------------------------------------------


    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.host.empty())
    {
      throw std::invalid_argument("DBHOST empty!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database host for test or dev environment -----------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::string alternative_host;

      #if defined(CAOS_DEFAULT_DBHOST_ON_DEV_OR_TEST)
        alternative_host = "" CAOS_DEFAULT_DBHOST_ON_DEV_OR_TEST;
      #endif

      if (alternative_host.empty())
      {
        // Alternative database host undefined
        spdlog::warn(missingAlternativeDBHOST,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database host defined
        PostgreSQL::Pool::config.host = alternative_host;
      }
    }
    // ---------------------------------------------------------------------------------------------



    // Validation ----------------------------------------------------------------------------------
    struct sockaddr_in sa4={};
    struct sockaddr_in6 sa6={};

    if (inet_pton(AF_INET, PostgreSQL::Pool::config.host.c_str(), &(sa4.sin_addr)) == 0
        && inet_pton(AF_INET6, PostgreSQL::Pool::config.host.c_str(), &(sa6.sin6_addr)) == 0)
    {
      throw std::invalid_argument("Invalid Address: " + PostgreSQL::Pool::config.host);
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "host",
               PostgreSQL::Pool::config.host,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setHost()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setPort()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setPort()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setPort";



  try
  {
    // Set primary database port for production environment ----------------------------------------
    const char* port_str = std::getenv(CAOS_ENV_DBPORT_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBPORT_NAME))
    {
      PostgreSQL::Pool::config.port = PostgreSQL::Pool::terminalPtr->get<std::uint16_t>(CAOS_OPT_DBPORT_NAME);
    }
    else if (port_str != nullptr)
    {
      PostgreSQL::Pool::config.port = static_cast<std::uint16_t>(std::stoi(port_str));
    }
    else
    {
      PostgreSQL::Pool::config.port = CAOS_DEFAULT_DBPORT;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.port == 0)
    {
      throw std::invalid_argument("DBPORT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database port for test or dev environment -----------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::uint16_t alternative_port=0;

      #if defined(CAOS_DEFAULT_DBPORT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPORT_ON_DEV_OR_TEST + 0) > 0
      alternative_port = static_cast<std::uint16_t>(CAOS_DEFAULT_DBPORT_ON_DEV_OR_TEST);
      #endif

      if (alternative_port == 0)
      {
        // Alternative database port undefined
        spdlog::warn(missingAlternativeDBPORT,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database port defined
        PostgreSQL::Pool::config.port = alternative_port;
      }
    }
    // ---------------------------------------------------------------------------------------------



    // Validation ----------------------------------------------------------------------------------
    if(PostgreSQL::Pool::config.port < CAOS_UNPRIVILEGED_PORT_MIN || PostgreSQL::Pool::config.port > CAOS_UNPRIVILEGED_PORT_MAX)
    {
      throw std::out_of_range("TCP Port out of range: " + std::to_string(PostgreSQL::Pool::config.port));
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::out_of_range& e)
  {
    spdlog::critical("{} : TCP Port range from 1024 to 65535 - {}",fName, e.what());
    std::exit(1);
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "port",
               PostgreSQL::Pool::config.port,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setPort()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setName()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setName()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setName";



  try
  {
    // Set primary database name for production environment ----------------------------------------
    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBNAME_NAME))
    {
      PostgreSQL::Pool::config.name = PostgreSQL::Pool::terminalPtr->get<std::string>(CAOS_OPT_DBNAME_NAME);
    }
    else if (const char* env_addr = std::getenv(CAOS_ENV_DBNAME_NAME))
    {
      PostgreSQL::Pool::config.name = env_addr;
    }
    else
    {
      PostgreSQL::Pool::config.name = CAOS_DEFAULT_DBNAME;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.name.empty())
    {
      throw std::invalid_argument("DBNAME empty!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database name for test or dev environment -----------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::string alternative_name;

      #if defined(CAOS_DEFAULT_DBNAME_ON_DEV_OR_TEST)
        alternative_name = "" CAOS_DEFAULT_DBNAME_ON_DEV_OR_TEST;
      #endif

      if (alternative_name.empty())
      {
        // Alternative database name undefined
        spdlog::warn(missingAlternativeDBNAME,PostgreSQL::Pool::environmentRef->getName());

        char defaultAnswer = 'N';

        std::cout << "Do you want to use production database in " << PostgreSQL::Pool::environmentRef->getName() << "? [y/N] ";
        std::string input;
        std::getline(std::cin, input);

        if (!input.empty())
        {
          // defaultAnswer = toupper(input[0]);
          defaultAnswer = static_cast<char>(std::toupper(static_cast<unsigned char>(input[0])));
        }

        if (defaultAnswer == 'Y')
        {
          std::cout << "It's terrible answer! Good luck!\n";
        }
        else
        {
          std::cout << "Wise decision, caos is terminating now.\n";
          std::exit(1);
        }

      }
      else
      {
        // Alternative database name defined
        PostgreSQL::Pool::config.name = alternative_name;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "name",
               PostgreSQL::Pool::config.name,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setName()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setPoolSizeMin()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setPoolSizeMin()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setPoolSizeMin";



  try
  {
    // Set primary database poolsizemin for production environment ------------------------------------
    const char* poolsizemin = std::getenv(CAOS_ENV_DBPOOLSIZEMIN_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBPOOLSIZEMIN_NAME))
    {
      PostgreSQL::Pool::config.poolsizemin = PostgreSQL::Pool::terminalPtr->get<std::size_t>(CAOS_OPT_DBPOOLSIZEMIN_NAME);
    }
    else if (poolsizemin != nullptr)
    {
      PostgreSQL::Pool::config.poolsizemin = static_cast<std::size_t>(std::stoi(poolsizemin));
    }
    else
    {
      PostgreSQL::Pool::config.poolsizemin = CAOS_DEFAULT_DBPOOLSIZEMIN;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.poolsizemin == 0)
    {
      throw std::invalid_argument("DBPOOLSIZEMIN unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database poolsizemin for test or dev environment -------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::size_t alternative_poolsizemin = 0;

      #if defined(CAOS_DEFAULT_DBPOOLSIZEMIN_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPOOLSIZEMIN_ON_DEV_OR_TEST + 0) > 0
      alternative_poolsizemin = static_cast<std::size_t>(CAOS_DEFAULT_DBPOOLSIZEMIN_ON_DEV_OR_TEST);
      #endif

      if (alternative_poolsizemin == 0)
      {
        // Alternative database poolsizemin undefined
        spdlog::warn(missingAlternativeDBPOOLSIZEMIN,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database poolsizemin defined
        PostgreSQL::Pool::config.poolsizemin = alternative_poolsizemin;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "poolsizemin",
               PostgreSQL::Pool::config.poolsizemin,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setPoolSizeMin()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------









// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setPoolSizeMax()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setPoolSizeMax()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setPoolSizeMax";



  try
  {
    // Set primary database poolsizemax for production environment ------------------------------------
    const char* poolsizemax = std::getenv(CAOS_ENV_DBPOOLSIZEMAX_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBPOOLSIZEMAX_NAME))
    {
      PostgreSQL::Pool::config.poolsizemax = PostgreSQL::Pool::terminalPtr->get<std::size_t>(CAOS_OPT_DBPOOLSIZEMAX_NAME);
    }
    else if (poolsizemax != nullptr)
    {
      PostgreSQL::Pool::config.poolsizemax = static_cast<std::size_t>(std::stoi(poolsizemax));
    }
    else
    {
      PostgreSQL::Pool::config.poolsizemax = CAOS_DEFAULT_DBPOOLSIZEMAX;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.poolsizemax == 0)
    {
      throw std::invalid_argument("DBPOOLSIZEMAX unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database poolsizemax for test or dev environment -------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::size_t alternative_poolsizemax = 0;

      #if defined(CAOS_DEFAULT_DBPOOLSIZEMAX_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPOOLSIZEMAX_ON_DEV_OR_TEST + 0) > 0
      alternative_poolsizemax = static_cast<std::size_t>(CAOS_DEFAULT_DBPOOLSIZEMAX_ON_DEV_OR_TEST);
      #endif

      if (alternative_poolsizemax == 0)
      {
        // Alternative database poolsizemax undefined
        spdlog::warn(missingAlternativeDBPOOLSIZEMAX,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database poolsizemax defined
        PostgreSQL::Pool::config.poolsizemax = alternative_poolsizemax;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "poolsizemax",
               PostgreSQL::Pool::config.poolsizemax,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setPoolSizeMax()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setPoolWait()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setPoolWait()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setPoolWait";

  try
  {
    // Set primary database poolwait for production environment ------------------------------------
    const char* poolwait = std::getenv(CAOS_ENV_DBPOOLWAIT_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBPOOLWAIT_NAME))
    {
      PostgreSQL::Pool::config.poolwait = PostgreSQL::Pool::terminalPtr->get<std::uint32_t>(CAOS_OPT_DBPOOLWAIT_NAME);
    }
    else if (poolwait != nullptr)
    {
      PostgreSQL::Pool::config.poolwait = static_cast<std::uint32_t>(std::stoi(poolwait));
    }
    else
    {
      PostgreSQL::Pool::config.poolwait = CAOS_DEFAULT_DBPOOLWAIT;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.poolwait == 0)
    {
      throw std::invalid_argument("DBPOOLWAIT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------


    // Set alternative database poolwait for test or dev environment -------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::uint32_t alternative_poolwait = 0;

      #if defined(CAOS_DEFAULT_DBPOOLWAIT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPOOLWAIT_ON_DEV_OR_TEST + 0) > 0
      alternative_poolwait = static_cast<std::uint32_t>(CAOS_DEFAULT_DBPOOLWAIT_ON_DEV_OR_TEST);
      #endif

      if (alternative_poolwait == 0)
      {
        // Alternative database poolwait undefined
        spdlog::warn(missingAlternativeDBPOOLWAIT,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database poolwait defined
        PostgreSQL::Pool::config.poolwait = alternative_poolwait;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "poolwait",
               PostgreSQL::Pool::config.poolwait,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setPoolWait()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setPoolTimeout()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setPoolTimeout()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setPoolTimeout()";

  try
  {
    // Set primary database pooltimeout for production environment ------------------------------------
    const char* pooltimeout = std::getenv(CAOS_ENV_DBPOOLTIMEOUT_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBPOOLTIMEOUT_NAME))
    {
      PostgreSQL::Pool::config.pooltimeout = std::chrono::milliseconds(PostgreSQL::Pool::terminalPtr->get<std::uint32_t>(CAOS_OPT_DBPOOLTIMEOUT_NAME));
    }
    else if (pooltimeout != nullptr)
    {
      PostgreSQL::Pool::config.pooltimeout = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(pooltimeout)));
    }
    else
    {
      PostgreSQL::Pool::config.pooltimeout = std::chrono::milliseconds(CAOS_DEFAULT_DBPOOLTIMEOUT);
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.pooltimeout == std::chrono::milliseconds(0))
    {
      throw std::invalid_argument("DBPOOLTIMEOUT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database pooltimeout for test or dev environment ----------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::chrono::milliseconds alternative_pooltimeout(0);

      #if defined(CAOS_DEFAULT_DBPOOLTIMEOUT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPOOLTIMEOUT_ON_DEV_OR_TEST + 0) > 0
      alternative_pooltimeout = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(CAOS_DEFAULT_DBPOOLTIMEOUT_ON_DEV_OR_TEST)));
      #endif

      if (alternative_pooltimeout == std::chrono::milliseconds(0))
      {
        // Alternative database pooltimeout undefined
        spdlog::warn(missingAlternativeDBPOOLTIMEOUT,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database pooltimeout defined
        PostgreSQL::Pool::config.pooltimeout = alternative_pooltimeout;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "pooltimeout",
               static_cast<uint32_t>(PostgreSQL::Pool::config.pooltimeout.count()),
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setPoolTimeout()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setKeepAlives()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setKeepAlives()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setKeepAlives";

  try
  {
    // Set primary database keepalives for production environment ----------------------------------
    const char* keepalives = std::getenv(CAOS_ENV_DBKEEPALIVES_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBKEEPALIVES_NAME))
    {
      PostgreSQL::Pool::config.keepalives = PostgreSQL::Pool::terminalPtr->get<std::size_t>(CAOS_OPT_DBKEEPALIVES_NAME);
    }
    else if (keepalives != nullptr)
    {
      PostgreSQL::Pool::config.keepalives = static_cast<std::size_t>(std::stoi(keepalives));
    }
    else
    {
      PostgreSQL::Pool::config.keepalives = CAOS_DEFAULT_DBKEEPALIVES;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.keepalives == 0)
    {
      throw std::invalid_argument("DBKEEPALIVES unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database keepalives for test or dev environment -----------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::size_t alternative_keepalives = 0;

      #if defined(CAOS_DEFAULT_DBKEEPALIVES_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBKEEPALIVES_ON_DEV_OR_TEST + 0) > 0
      alternative_keepalives = static_cast<std::size_t>(CAOS_DEFAULT_DBKEEPALIVES_ON_DEV_OR_TEST);
      #endif

      if (alternative_keepalives == 0)
      {
        // Alternative database keepalives undefined
        spdlog::warn(missingAlternativeDBKEEPALIVES,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database keepalives defined
        PostgreSQL::Pool::config.keepalives = alternative_keepalives;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "keepalives",
               PostgreSQL::Pool::config.keepalives,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setKeepAlives()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setKeepAlivesIdle()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setKeepAlivesIdle()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setKeepAlivesIdle";

  try
  {
    // Set primary database keepalives_idle for production environment -----------------------------
    const char* keepalives_idle = std::getenv(CAOS_ENV_DBKEEPALIVES_IDLE_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBKEEPALIVES_IDLE_NAME))
    {
      PostgreSQL::Pool::config.keepalives_idle = PostgreSQL::Pool::terminalPtr->get<std::size_t>(CAOS_OPT_DBKEEPALIVES_IDLE_NAME);
    }
    else if (keepalives_idle != nullptr)
    {
      PostgreSQL::Pool::config.keepalives_idle = static_cast<std::size_t>(std::stoi(keepalives_idle));
    }
    else
    {
      PostgreSQL::Pool::config.keepalives_idle = CAOS_DEFAULT_DBKEEPALIVES_IDLE;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.keepalives_idle == 0)
    {
      throw std::invalid_argument("DBKEEPALIVES_IDLE unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database keepalives_idle for test or dev environment ------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::size_t alternative_keepalives_idle = 0;

      #if defined(CAOS_DEFAULT_DBKEEPALIVES_IDLE_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBKEEPALIVES_IDLE_ON_DEV_OR_TEST + 0) > 0
      alternative_keepalives_idle = static_cast<std::size_t>(CAOS_DEFAULT_DBKEEPALIVES_IDLE_ON_DEV_OR_TEST);
      #endif

      if (alternative_keepalives_idle == 0)
      {
        // Alternative database keepalives_idle undefined
        spdlog::warn(missingAlternativeDBKEEPALIVES_IDLE,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database keepalives_idle defined
        PostgreSQL::Pool::config.keepalives_idle = alternative_keepalives_idle;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "keepalives_idle",
               PostgreSQL::Pool::config.keepalives_idle,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setKeepAlivesIdle()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setKeepAlives_Interval()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setKeepAlivesInterval()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setKeepAlivesInterval";

  try
  {
    // Set primary database keepalives_interval for production environment -------------------------
    const char* keepalives_interval = std::getenv(CAOS_ENV_DBKEEPALIVES_INTERVAL_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBKEEPALIVES_INTERVAL_NAME))
    {
      PostgreSQL::Pool::config.keepalives_interval = PostgreSQL::Pool::terminalPtr->get<std::size_t>(CAOS_OPT_DBKEEPALIVES_INTERVAL_NAME);
    }
    else if (keepalives_interval != nullptr)
    {
      PostgreSQL::Pool::config.keepalives_interval = static_cast<std::size_t>(std::stoi(keepalives_interval));
    }
    else
    {
      PostgreSQL::Pool::config.keepalives_interval = CAOS_DEFAULT_DBKEEPALIVES_INTERVAL;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.keepalives_interval == 0)
    {
      throw std::invalid_argument("DBKEEPALIVES_INTERVAL unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database keepalives_interval for test or dev environment --------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::size_t alternative_keepalives_interval = 0;

      #if defined(CAOS_DEFAULT_DBKEEPALIVES_INTERVAL_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBKEEPALIVES_INTERVAL_ON_DEV_OR_TEST + 0) > 0
      alternative_keepalives_interval = static_cast<std::size_t>(CAOS_DEFAULT_DBKEEPALIVES_INTERVAL_ON_DEV_OR_TEST);
      #endif

      if (alternative_keepalives_interval == 0)
      {
        // Alternative database keepalives_interval undefined
        spdlog::warn(missingAlternativeDBKEEPALIVES_INTERVAL,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database keepalives_interval defined
        PostgreSQL::Pool::config.keepalives_interval = alternative_keepalives_interval;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "keepalives_interval",
               PostgreSQL::Pool::config.keepalives_interval,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setKeepAlivesInterval()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::setKeepAlivesCount()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setKeepAlivesCount()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setKeepAlivesCount";

  try
  {
    // Set primary database keepalives_count for production environment ----------------------------
    const char* keepalives_count = std::getenv(CAOS_ENV_DBKEEPALIVES_COUNT_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBKEEPALIVES_COUNT_NAME))
    {
      PostgreSQL::Pool::config.keepalives_count = PostgreSQL::Pool::terminalPtr->get<std::size_t>(CAOS_OPT_DBKEEPALIVES_COUNT_NAME);
    }
    else if (keepalives_count != nullptr)
    {
      PostgreSQL::Pool::config.keepalives_count = static_cast<std::size_t>(std::stoi(keepalives_count));
    }
    else
    {
      PostgreSQL::Pool::config.keepalives_count = CAOS_DEFAULT_DBKEEPALIVES_COUNT;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.keepalives_count == 0)
    {
      throw std::invalid_argument("DBKEEPALIVES_COUNT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database keepalives_count for test or dev environment -----------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::size_t alternative_keepalives_count = 0;

      #if defined(CAOS_DEFAULT_DBKEEPALIVES_COUNT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBKEEPALIVES_COUNT_ON_DEV_OR_TEST + 0) > 0
      alternative_keepalives_count = static_cast<std::size_t>(CAOS_DEFAULT_DBKEEPALIVES_COUNT_ON_DEV_OR_TEST);
      #endif

      if (alternative_keepalives_count == 0)
      {
        // Alternative database keepalives_count undefined
        spdlog::warn(missingAlternativeDBKEEPALIVES_COUNT,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database keepalives_count defined
        PostgreSQL::Pool::config.keepalives_count = alternative_keepalives_count;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "keepalives_count",
               PostgreSQL::Pool::config.keepalives_count,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setKeepAlivesCount()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setConnectTimeout()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setConnectTimeout()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setConnectTimeout";

  try
  {
    // Set primary database connect_timeout for production environment -----------------------------
    const char* connect_timeout = std::getenv(CAOS_ENV_DBCONNECT_TIMEOUT_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBCONNECT_TIMEOUT_NAME))
    {
      PostgreSQL::Pool::config.connect_timeout = PostgreSQL::Pool::terminalPtr->get<std::size_t>(CAOS_OPT_DBCONNECT_TIMEOUT_NAME);
    }
    else if (connect_timeout != nullptr)
    {
      PostgreSQL::Pool::config.connect_timeout = static_cast<std::size_t>(std::stoi(connect_timeout));
    }
    else
    {
      PostgreSQL::Pool::config.connect_timeout = CAOS_DEFAULT_DBCONNECT_TIMEOUT;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.connect_timeout == 0)
    {
      throw std::invalid_argument("DBCONNECT_TIMEOUT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database connect_timeout for test or dev environment ------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::size_t alternative_connect_timeout = 0;

      #if defined(CAOS_DEFAULT_DBCONNECT_TIMEOUT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBCONNECT_TIMEOUT_ON_DEV_OR_TEST + 0) > 0
      alternative_connect_timeout = static_cast<std::size_t>(CAOS_DEFAULT_DBCONNECT_TIMEOUT_ON_DEV_OR_TEST);
      #endif

      if (alternative_connect_timeout == 0)
      {
        // Alternative database connect_timeout undefined
        spdlog::warn(missingAlternativeDBCONNECT_TIMEOUT,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database connect_timeout defined
        PostgreSQL::Pool::config.connect_timeout = alternative_connect_timeout;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "connect_timeout",
               PostgreSQL::Pool::config.connect_timeout,
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setConnectTimeout()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------









// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setConnectStr()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void   PostgreSQL::Pool::setConnectStr() noexcept
{
  std::ostringstream oss;

  oss << "host="                  << PostgreSQL::Pool::config.host
      << " port="                 << PostgreSQL::Pool::config.port
      << " dbname="               << PostgreSQL::Pool::config.name
      << " user="                 << PostgreSQL::Pool::config.user
      << " password="             << PostgreSQL::Pool::config.pass
      << " connect_timeout="      << PostgreSQL::Pool::config.connect_timeout
      << " keepalives="           << PostgreSQL::Pool::config.keepalives
      << " keepalives_interval="  << PostgreSQL::Pool::config.keepalives_interval
      << " keepalives_idle="      << PostgreSQL::Pool::config.keepalives_idle
      << " keepalives_count="     << PostgreSQL::Pool::config.keepalives_count;

  PostgreSQL::Pool::config.connection_string = oss.str();
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setConnectStr()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setMaxWait()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setMaxWait()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setMaxWait";

  try
  {
    // Set primary database poolwait for production environment ------------------------------------
    const char* max_wait = std::getenv(CAOS_ENV_DBMAXWAIT_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBMAXWAIT_NAME))
    {
      PostgreSQL::Pool::config.max_wait = std::chrono::milliseconds(PostgreSQL::Pool::terminalPtr->get<std::uint32_t>(CAOS_OPT_DBMAXWAIT_NAME));
    }
    else if (max_wait != nullptr)
    {
      PostgreSQL::Pool::config.max_wait = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(max_wait)));
    }
    else
    {
      PostgreSQL::Pool::config.max_wait = std::chrono::milliseconds(CAOS_DEFAULT_DBMAXWAIT);
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.max_wait == std::chrono::milliseconds(0))
    {
      throw std::invalid_argument("DBMAXWAIT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database max_wait for test or dev environment -------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::chrono::milliseconds alternative_max_wait(0);

      #if defined(CAOS_DEFAULT_DBMAXWAIT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBMAXWAIT_ON_DEV_OR_TEST + 0) > 0
      alternative_max_wait = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(CAOS_DEFAULT_DBMAXWAIT_ON_DEV_OR_TEST)));
      #endif

      if (alternative_max_wait == std::chrono::milliseconds(0))
      {
        // Alternative database max_wait undefined
        spdlog::warn(missingAlternativeDBMAXWAIT,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database max_wait defined
        PostgreSQL::Pool::config.max_wait = alternative_max_wait;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "maxwait",
               static_cast<uint32_t>(PostgreSQL::Pool::config.max_wait.count()),
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setMaxWait()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------











// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::setHealthCheckInterval()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::setHealthCheckInterval()
{
  static constexpr const char* fName = "PostgreSQL::Pool::setHealthCheckInterval";

  try
  {
    // Set primary database health_check_interval for production environment ------------------------------------
    const char* health_check_interval = std::getenv(CAOS_ENV_DBHEALTHCHECKINTERVAL_NAME);

    if (PostgreSQL::Pool::terminalPtr->has(CAOS_OPT_DBHEALTHCHECKINTERVAL_NAME))
    {
      PostgreSQL::Pool::config.health_check_interval = std::chrono::milliseconds(PostgreSQL::Pool::terminalPtr->get<std::uint32_t>(CAOS_OPT_DBHEALTHCHECKINTERVAL_NAME));
    }
    else if (health_check_interval != nullptr)
    {
      PostgreSQL::Pool::config.health_check_interval = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(health_check_interval)));
    }
    else
    {
      PostgreSQL::Pool::config.health_check_interval= std::chrono::milliseconds(CAOS_DEFAULT_DBHEALTHCHECKINTERVAL);
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (PostgreSQL::Pool::config.health_check_interval == std::chrono::milliseconds(0))
    {
      throw std::invalid_argument("DBHEALTHCHECKINTERVAL unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database poolwait for test or dev environment -------------------------------
    if (PostgreSQL::Pool::isDevOrTestEnv())
    {
      std::chrono::milliseconds alternative_health_check_interval(0);

      #if defined(CAOS_DEFAULT_DBHEALTHCHECKINTERVAL_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBHEALTHCHECKINTERVAL_ON_DEV_OR_TEST + 0) > 0
      alternative_health_check_interval = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(CAOS_DEFAULT_DBHEALTHCHECKINTERVAL_ON_DEV_OR_TEST)));
      #endif

      if (alternative_health_check_interval == std::chrono::milliseconds(0))
      {
        // Alternative database health_check_interval undefined
        spdlog::warn(missingAlternativeDBHEALTHCHECKINTERVAL,PostgreSQL::Pool::environmentRef->getName());
      }
      else
      {
        // Alternative database health_check_interval defined
        PostgreSQL::Pool::config.health_check_interval = alternative_health_check_interval;
      }
    }
    // ---------------------------------------------------------------------------------------------
  }
  catch(const std::invalid_argument& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] : {}",fName,e.what());
    std::exit(1);
  }



  // Final -----------------------------------------------------------------------------------------
  spdlog::info(defaultFinal,
               fName,
               "health_check_interval",
               static_cast<uint32_t>(PostgreSQL::Pool::config.health_check_interval.count()),
               PostgreSQL::Pool::environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::setHealthCheckInterval()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










std::string               PostgreSQL::Pool::getUser()                 noexcept { return PostgreSQL::Pool::config.user;                  }
std::string               PostgreSQL::Pool::getPass()                 noexcept { return PostgreSQL::Pool::config.pass;                  }
std::string               PostgreSQL::Pool::getHost()                 noexcept { return PostgreSQL::Pool::config.host;                  }
std::uint16_t             PostgreSQL::Pool::getPort()                 noexcept { return PostgreSQL::Pool::config.port;                  }
std::string               PostgreSQL::Pool::getName()                 noexcept { return PostgreSQL::Pool::config.name;                  }
std::size_t               PostgreSQL::Pool::getPoolSizeMin()          noexcept { return PostgreSQL::Pool::config.poolsizemin;           }
std::size_t               PostgreSQL::Pool::getPoolSizeMax()          noexcept { return PostgreSQL::Pool::config.poolsizemax;           }
std::uint32_t             PostgreSQL::Pool::getPoolWait()             noexcept { return PostgreSQL::Pool::config.poolwait;              }
std::chrono::milliseconds PostgreSQL::Pool::getPoolTimeout()          noexcept { return PostgreSQL::Pool::config.pooltimeout;           }
std::size_t               PostgreSQL::Pool::getKeepAlives()           noexcept { return PostgreSQL::Pool::config.keepalives;            }
std::size_t               PostgreSQL::Pool::getKeepAlivesIdle()       noexcept { return PostgreSQL::Pool::config.keepalives_idle;       }
std::size_t               PostgreSQL::Pool::getKeepAlivesInterval()   noexcept { return PostgreSQL::Pool::config.keepalives_interval;   }
std::size_t               PostgreSQL::Pool::getKeepAlivesCount()      noexcept { return PostgreSQL::Pool::config.keepalives_count;      }
std::size_t               PostgreSQL::Pool::getConnectTimeout()       noexcept { return PostgreSQL::Pool::config.connect_timeout;       }
std::string&              PostgreSQL::Pool::getConnectStr()           noexcept { return PostgreSQL::Pool::config.connection_string;     }
std::chrono::milliseconds PostgreSQL::Pool::getMaxWait()              noexcept { return PostgreSQL::Pool::config.max_wait;              }
std::chrono::milliseconds PostgreSQL::Pool::getHealthCheckInterval()  noexcept { return PostgreSQL::Pool::config.health_check_interval; }










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::get_metrics()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// PostgreSQL::Pool::Metrics PostgreSQL::Pool::get_metrics() const noexcept
// {
//   std::lock_guard<std::mutex> lock(PostgreSQL::Pool::available_mutex_);

//   return Metrics
//   {
//     PostgreSQL::Pool::available_.size(),
//     total_known_.load(std::memory_order_relaxed),
//     creations_.load(std::memory_order_relaxed),
//     failures_.load(std::memory_order_relaxed),
//   };
// }
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::get_metrics()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::getAvailableConnections()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::size_t PostgreSQL::Pool::getAvailableConnections() noexcept
{
  std::size_t count = 0;
std::cout << "I\n";
  std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);
std::cout << "L\n";
  // Cerca la unique_ptr corrispondente nella mappa
  for (auto it = PostgreSQL::Pool::connections.begin(); it != PostgreSQL::Pool::connections.end(); ++it)
  {
    auto& metrics = it->second;

    if (!metrics.is_acquired)
    {
      count++;
    }
  }

  return count;
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::getAvailableConnections()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::getTotalConnections()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::size_t PostgreSQL::Pool::getTotalConnections() noexcept
{
  std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);
  return PostgreSQL::Pool::connections.size();
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::getTotalConnections()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------


bool inline PostgreSQL::Pool::checkPoolSize(std::size_t& askingPoolSize)
{
  static constexpr const char* fName = "PostgreSQL::Pool::checkPoolSize";

  std::size_t totalConnections  = PostgreSQL::Pool::getTotalConnections();
  std::size_t poolSizeMax       = PostgreSQL::Pool::getPoolSizeMax();

  if (totalConnections >= poolSizeMax)  // Don't saturate PostgreSQL connections
  {
    spdlog::warn("[{}] Connection pool limit reached, verify CAOS_DEFAULT_DBPOOLSIZEMAX variable, current poolsize is {}, asked for {} new connections, max poolsize is {}",
                 fName,
                 totalConnections,
                 askingPoolSize,
                 poolSizeMax);

    return false;
  }

  return true;
}





// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::init()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::size_t PostgreSQL::Pool::init(std::size_t count)
{
  static constexpr const char* fName = "PostgreSQL::Pool::init";

  std::size_t i = 0;
  std::size_t pool_size = (count>0) ? count : PostgreSQL::Pool::getPoolSizeMin();
  std::atomic<std::size_t> offlineCounter{0};

  spdlog::warn("[{}] New pool building", fName);

  // std::cout << "running "   << PostgreSQL::Pool::connection_.count.load(std::memory_order_acquire)
  //           << " building " << pool_size
  //           << "\n";


  while (i < pool_size                                                                              // Create connections until the requested size is reached
         && running_.load(std::memory_order_acquire))                                               // Stop if a signal is detect
  {
    if (createConnection(pool_size))                                                                // Create and validate connection
    {
      i++;

      offlineCounter.store(0,std::memory_order_release);
      PostgreSQL::Pool::offline_.store(false, std::memory_order_release);

      spdlog::trace("[{}] New PostgreSQL connection established", fName);
    }
    else
    {
      offlineCounter.fetch_add(1);

      if (offlineCounter.load(std::memory_order_acquire) > 5)
      {
        spdlog::error("[{}] PostgreSQL is unreachable", fName);
        PostgreSQL::Pool::offline_.store(true, std::memory_order_release);
        break;
      }

      /*
       * Don't flood the database with connection attempts when it's unreachable
       * Just wait a little before retrying
       */
      {
        std::unique_lock<std::mutex> waitlock(PostgreSQL::Pool::shutdown_mutex_);
        PostgreSQL::Pool::shutdown_cv_.wait_for(
          waitlock,
          PostgreSQL::Pool::getMaxWait(),
          [] { return !PostgreSQL::Pool::running_.load(std::memory_order_acquire); }
        );
      }
    }
  }

  return i;
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::init()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::reinit()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// void PostgreSQL::Pool::reinit()
// {
//   static constexpr const char* fName = "PostgreSQL::Pool::reinit";

//   // Save current connection in a queue ----------------------------------------------------------
//   std::queue<pqxxShPtr> validConnections;
//   // ---------------------------------------------------------------------------------------------

//   std::lock_guard<std::mutex> lock(PostgreSQL::Pool::available_mutex_);

//   while (!PostgreSQL::Pool::available_.empty() && running_.load(std::memory_order_acquire))
//   {
//     spdlog::trace("[{}] Checking current connection", fName);

//     auto connection = std::move(PostgreSQL::Pool::available_.front());
//     if (PostgreSQL::Pool::validateConnection(connection))
//     {
//       spdlog::trace("[{}] Connection is valid", fName);

//       validConnections.push(std::move(connection));
//       PostgreSQL::Pool::available_.pop();

//       continue;
//     }

//     auto recreatedConnection = this->init(true);                                                  // Existing connection is invalid, make a brand new one
//     if (recreatedConnection.has_value())
//     {
//       validConnections.push(std::move(*recreatedConnection));
//       PostgreSQL::Pool::available_.pop();
//     }
//     else
//     {
//       if (running_.load(std::memory_order_acquire))
//       {
//         std::this_thread::sleep_for(PostgreSQL::Pool::getMaxWait());
//       }
//     }
//   }

//   // All connections processed, make them available again
//   if (PostgreSQL::Pool::available_.empty())
//   {
//     while (!validConnections.empty())
//     {
//       PostgreSQL::Pool::available_.push(std::move(validConnections.front()));
//       validConnections.pop();
//     }

//     spdlog::trace("[{}] Connections is alive and working", fName);
//   }
// }
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::reinit()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::validateConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
bool PostgreSQL::Pool::validateConnection(const std::unique_ptr<pqxx::connection>& connection, std::unique_lock<std::mutex>* existing_lock)
{
  static constexpr const char* fName = "PostgreSQL::Pool::validateConnection";

  std::unique_lock<std::mutex> internal_lock;

  if (existing_lock == nullptr || !existing_lock->owns_lock())
  {
    internal_lock = std::unique_lock<std::mutex>(PostgreSQL::Pool::connections_mutex);
  }

  if (!connection)
  {
    return false;
  }

  try
  {
    if (connection->is_open())
    {
      spdlog::trace("[{}] Health check query", fName);

      pqxx::result result;

      {
        pqxx::nontransaction nontx(*connection);
        result = nontx.exec("SELECT 1");

        // pqxx::work tx(*connection);
        // result=tx.exec("SELECT 1");
        // tx.commit();
      }

      if (!result.empty())
      {
        return true;
      }
    }
  }
  catch (const pqxx::broken_connection& e)
  {
    spdlog::error("[{}] 3Broken connection: {}", fName, e.what());
  }
  catch (const std::exception& e)
  {
    spdlog::error("[{}] : {}", fName, e.what());
  }
  catch (...)
  {
    spdlog::error("[pool] validate: unknown exception");
  }

  PostgreSQL::Pool::closeConnection(connection);

  return false;
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::validateConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::createConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
bool PostgreSQL::Pool::createConnection(std::size_t& pool_size)
{
  static constexpr const char* fName = "PostgreSQL::Pool::createConnection";

  try
  {
    spdlog::debug("[{}] Creating new connection", fName);

    if(!PostgreSQL::Pool::checkPoolSize(pool_size))                                                 // Don't saturate PostgreSQL connections
    {
      return false;
    }

    auto connection = std::make_unique<pqxx::connection>(PostgreSQL::Pool::getConnectStr());

    if (PostgreSQL::Pool::validateConnection(connection))
    {
      ConnectionMetrics metrics;

      {
          std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

          PostgreSQL::Pool::connections.emplace(
              std::piecewise_construct,
              std::forward_as_tuple(std::move(connection)),
              std::forward_as_tuple(std::move(metrics))
          );
      }

      spdlog::info("[{}] New valid connection created (total: {})",
                  fName, PostgreSQL::Pool::getTotalConnections());

      return true;
    }
  }
  catch (const pqxx::broken_connection& e)
  {
    spdlog::error("[{}] 1Broken connection: {}", fName, e.what());
  }
  catch (const pqxx::sql_error& e)
  {
    spdlog::error("[{}] SQL error during connection creation: {}", fName, e.what());
  }
  catch (const std::exception& e)
  {
    spdlog::critical("[{}] Exception during connection creation: {}", fName, e.what());
  }
  catch (...)
  {
    spdlog::critical("[{}] Unknown exception during connection creation", fName);
  }

  return false;
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::createConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::healthCheckLoop()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// void PostgreSQL::Pool::healthCheckLoop()
// {
//   static constexpr const char* fName = "PostgreSQL::Pool::healthCheckLoop";
//   static std::atomic<bool> start;

//   while (running_.load(std::memory_order_acquire))
//   {


//     spdlog::trace("Running {}",fName);

//     /*
//      * Scenario 1: If the repository launches while the database is down/unreachable, available_
//      * remains empty, requiring the creation of brand-new connections.
//      */
//     if(PostgreSQL::Pool::available_.empty())
//     {
//       if (start)
//       {
//         this->init();
//       }
//       else
//       {
//         start=true;

//         // auto average_duration = PostgreSQL::Pool::calculateAverageDuration();
//         // if (average_duration>this->getPoolTimeout())
//         // {

//         this->init();


//       }
//     }

//     /*
//      * Scenario 2: The repository is already running with established working connections, and must
//      * perform an health check—either validating the existing connections or replacing them if they
//      * fail.
//      */
//     else
//     {
//       this->reinit();
//     }

//     this->condition_.notify_all();

//     PostgreSQL::Pool::cleanupIdleConnections();

//     if (running_.load(std::memory_order_acquire))
//     {
//       std::this_thread::sleep_for(PostgreSQL::Pool::getHealthCheckInterval());
//     }
//   }
// }

void PostgreSQL::Pool::healthCheckLoop()
{
  static constexpr const char* fName = "PostgreSQL::Pool::healthCheckLoop";
  static std::atomic<bool> first_start{true};

  while (PostgreSQL::Pool::running_.load(std::memory_order_acquire))
  {
    if (!first_start.load(std::memory_order_acquire) &&
        !PostgreSQL::Pool::offline_.load(std::memory_order_acquire))
    {
      std::unique_lock<std::mutex> waitlock(PostgreSQL::Pool::shutdown_mutex_);
      shutdown_cv_.wait_for(
            waitlock,
            PostgreSQL::Pool::getHealthCheckInterval(),
            [] { return !PostgreSQL::Pool::running_.load(std::memory_order_acquire); }
      );
    }

    bool isConnectionAvailable = false;
    {
      std::unique_lock<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

      isConnectionAvailable = condition.wait_for(lock,
                                                 PostgreSQL::Pool::getMaxWait(),
                                                 [] {
                                                   return !PostgreSQL::Pool::connections.empty()
                                                       || !PostgreSQL::Pool::running_.load(std::memory_order_acquire);
                                                 });
    }
std::cout << (isConnectionAvailable?"TRUE":"FALSE") << "\n";
    if (!PostgreSQL::Pool::running_.load(std::memory_order_acquire))
    {
      break;
    }

    spdlog::trace("Running {}", fName);

    // -------------------------
    // Ora il mutex available_ è libero:
    // puoi fare operazioni costose senza bloccare altri thread
    // -------------------------

    if (isConnectionAvailable)
    {
      spdlog::debug("Performing health check on existing connections");

      // std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);
      std::unique_lock<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

      std::size_t totalConnections = PostgreSQL::Pool::connections.size();

      for (auto it = PostgreSQL::Pool::connections.begin(); it != PostgreSQL::Pool::connections.end(); )
      {
        const auto& [connection, metrics] = *it;

        if (!metrics.is_acquired)
        {
          if (PostgreSQL::Pool::validateConnection(connection, &lock))
          {
            spdlog::trace("[{}] Connection is valid", fName);
            ++it;
            continue;
          }

          spdlog::info("[{}] Connection is invalid", fName);

        }

        ++it;
      }

      // PostgreSQL::Pool::cleanupIdleConnections();

      int connectionDiff = static_cast<int>(totalConnections - PostgreSQL::Pool::getPoolSizeMin());

      if (connectionDiff < 0)
      {
        PostgreSQL::Pool::init(static_cast<std::size_t>(-connectionDiff));
      }
    }
    else
    {
      if (first_start.load(std::memory_order_acquire))
      {
        first_start.store(false, std::memory_order_release);
        spdlog::debug("First initialization - no connections available");
        PostgreSQL::Pool::init();
      }
      else
      {
        spdlog::debug("Health check timeout - reinitializing connections");
        PostgreSQL::Pool::init();
      }
    }

    condition.notify_all();
  }
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::healthCheckLoop()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::calculateAverageDuration()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::chrono::milliseconds PostgreSQL::Pool::calculateAverageDuration()
{
  std::chrono::milliseconds total_duration{0};
  int total_operations = 0;

  {
    std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

    if (PostgreSQL::Pool::connections.empty())
    {
      return std::chrono::milliseconds(0);
    }

    for (const auto& [connection, metrics] : PostgreSQL::Pool::connections)
    {
      total_duration += metrics.total_duration;
      total_operations += metrics.usage_count;
    }
  }

  if (total_operations == 0)
  {
    return std::chrono::milliseconds(0);
  }

  return std::chrono::milliseconds(total_duration / total_operations);
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::calculateAverageDuration()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::acquireConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::optional<const std::unique_ptr<pqxx::connection>*> PostgreSQL::Pool::acquireConnection()
{
  static constexpr const char* fName = "PostgreSQL::Pool::acquireConnection";

  std::unique_lock lock(PostgreSQL::Pool::connections_mutex);

  bool success = PostgreSQL::Pool::condition.wait_for(lock,
                                                      getMaxWait(),
                                                      []{
                                                        return !PostgreSQL::Pool::connections.empty()
                                                            || !PostgreSQL::Pool::running_.load(std::memory_order_acquire);
                                                      });

  if (!success || !PostgreSQL::Pool::running_.load(std::memory_order_acquire))
  {
    return std::nullopt;
  }

  for (auto it = PostgreSQL::Pool::connections.begin(); it != PostgreSQL::Pool::connections.end(); ++it)
  {
    const auto& connection  = it->first                                                   ;
    auto&       metrics     = it->second                                                  ;

    if (!metrics.is_acquired)
    {
      auto now = std::chrono::steady_clock::now()                                         ;
      metrics.start_time    = now                                                         ;
      metrics.last_acquired = now                                                         ;
      metrics.is_acquired   = true                                                        ;
      metrics.usage_count++                                                               ;

      return &connection                                                                  ;
    }
  }

  lock.unlock();

  static std::atomic<std::size_t> limiter                                     {0}         ;

  std::size_t current = limiter.fetch_add(1, std::memory_order_acq_rel) + 1               ;

  if (current >= CAOS_LOG_THRESHOLD_CONNECTION_LIMIT_EXCEEDED)
  {
    std::size_t expected = current;

    if (limiter.compare_exchange_strong(expected, 0,
                                        std::memory_order_release,
                                        std::memory_order_relaxed))
    {
      spdlog::warn("Unable to acquire PostgreSQL connection - {} failed requests (pool busy)", current);
      spdlog::info("PostgreSQL connection limit exceeded: reduce thread pool size or increase max_connections parameter");
    }
  }

  return std::nullopt;
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::acquireConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::closeConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::closeConnection(const std::unique_ptr<pqxx::connection>& connection)
{
  static constexpr const char* fName = "PostgreSQL::Pool::closeConnection(1)";

  try
  {
    if (!connection)
    {
      spdlog::warn("[{}] Attempted to close null connection", fName);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

      if (auto it = PostgreSQL::Pool::connections.find(connection); it != PostgreSQL::Pool::connections.end())
      {
        connection->close();
        spdlog::debug("[{}] Removed metrics for connection (used {} times)", fName, it->second.usage_count);
        PostgreSQL::Pool::connections.erase(it);

        return;
      }
    }
  }
  catch (const pqxx::broken_connection& e)
  {
    spdlog::debug("[{}] Connection already broken: {}", fName, e.what());
  }
  catch (const pqxx::sql_error& e)
  {
    spdlog::warn("[{}] SQL error during connection close: {}", fName, e.what());
  }
  catch (const std::exception& e)
  {
    spdlog::error("[{}] Exception while closing connection: {}", fName, e.what());
  }
  catch (...)
  {
    spdlog::critical("[{}] Unknown exception while closing connection", fName);
  }
}

void PostgreSQL::Pool::closeConnection(std::optional<PostgreSQL::ConnectionWrapper>& connection)
{
  static constexpr const char* fName = "PostgreSQL::Pool::closeConnection(2)";

  try
  {
    if (!connection)
    {
      spdlog::warn("[{}] Attempted to close null connection", fName);
      return;
    }

    {
      std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

      auto* raw = connection.value().get ();
      auto it = std::find_if(
          PostgreSQL::Pool::connections.begin(),
          PostgreSQL::Pool::connections.end(),
          [raw](auto const& pair) {
              return pair.first.get() == raw;
          }
      );

      if (it != PostgreSQL::Pool::connections.end())
      {
        raw->close();
        spdlog::debug("[{}] Removed metrics for connection (used {} times)", fName, it->second.usage_count);
        PostgreSQL::Pool::connections.erase(it);

        return;
      }
    }
  }
  catch (const pqxx::broken_connection& e)
  {
    spdlog::debug("[{}] Connection already broken: {}", fName, e.what());
  }
  catch (const pqxx::sql_error& e)
  {
    spdlog::warn("[{}] SQL error during connection close: {}", fName, e.what());
  }
  catch (const std::exception& e)
  {
    spdlog::error("[{}] Exception while closing connection: {}", fName, e.what());
  }
  catch (...)
  {
    spdlog::critical("[{}] Unknown exception while closing connection", fName);
  }
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::closeConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::handleInvalidConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::handleInvalidConnection(const std::unique_ptr<pqxx::connection>& connection)
{
  std::size_t newConnection = PostgreSQL::Pool::init(1);

  if (newConnection==1)
  {
    PostgreSQL::Pool::condition.notify_all();
  }

}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::handleInvalidConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::cleanupIdleConnections()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::cleanupIdleConnections()
{
  std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);
  auto now = std::chrono::steady_clock::now();

  for (auto it = PostgreSQL::Pool::connections.begin(); it != PostgreSQL::Pool::connections.end(); )
  {
    if (PostgreSQL::Pool::connections.size()<=PostgreSQL::Pool::getPoolSizeMin())
    {
      break;
    }
    const auto& [connection, metrics] = *it;

    // Convert weak_ptr into a shared_ptr

    // auto idle_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - metrics.end_time);

    // std::cout << (metrics.is_acquired.load(std::memory_order_acquire) ? "ACQUIRED" : "IDLE")
    //           << " - Idle time: " << idle_time.count() << "ms"
    //           << " - Timeout: " << PostgreSQL::Pool::getPoolTimeout().count() << "ms\n";

    // Se la connessione non è in uso ed è inattiva da troppo tempo
    if (metrics.is_acquired==false && ((now - metrics.end_time) > PostgreSQL::Pool::getPoolTimeout()))
    {
      // Log delle metriche prima di rimuovere
      auto total_usage_ms = std::chrono::duration_cast<std::chrono::milliseconds>(metrics.total_duration);
      std::cout << "Closing connection - Total usage: "
                << total_usage_ms.count() << "ms, "
                << "Used " << metrics.usage_count << " times\n";

      // USA LO shared_ptr OTTENUTO DAL LOCK
      PostgreSQL::Pool::closeConnection(connection);
    }
    else
    {
      ++it;
    }
  }
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::cleanupIdleConnections()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::startCleanupTask()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// void PostgreSQL::Pool::startCleanupTask()
// {
//   std::cout << "Start CleanUp task\n";

//   static std::atomic<bool> cleanup_running{false};

//   if (!cleanup_running.exchange(true))
//   {std::cout << "----------------------------------------------------------------------------------------------------------------\n";
//     std::thread([this] {
//       this->cleanupIdleConnections();
//       cleanup_running.store(false);
//     }).detach();
//   }
// }
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::startCleanupTask()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::acquire()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::optional<PostgreSQL::ConnectionWrapper> PostgreSQL::Pool::acquire()
{
    auto connection_opt = PostgreSQL::Pool::acquireConnection();

    if (connection_opt) {
        return ConnectionWrapper(
            connection_opt, // Estrai il pointer dall'optional
            [](std::optional<const std::unique_ptr<pqxx::connection>*> conn_ptr){
              PostgreSQL::Pool::releaseConnection(conn_ptr);
            });
    }

    return std::nullopt;
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::acquire()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::releaseConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::releaseConnection(std::optional<const std::unique_ptr<pqxx::connection>*> connection_opt)
{
  if (connection_opt.has_value())
  {
    // Estrai il pointer allo unique_ptr dall'optional
    const std::unique_ptr<pqxx::connection>* unique_ptr_ptr = connection_opt.value();

    std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

    // Cerca la unique_ptr corrispondente nella mappa
    for (auto it = PostgreSQL::Pool::connections.begin(); it != PostgreSQL::Pool::connections.end(); ++it)
    {
      // Confronta gli indirizzi degli unique_ptr
      if (&it->first == unique_ptr_ptr)
      {
        auto& metrics = it->second;
        auto now = std::chrono::steady_clock::now();
        metrics.end_time = now;
        metrics.last_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - metrics.start_time);
        metrics.total_duration += metrics.last_duration;
        metrics.is_acquired = false;
// std::cout << (metrics.is_acquired?"RELEASE-TRUE":"RELEASE-FALSE") << "\n";
        PostgreSQL::Pool::condition.notify_one();
        return;
      }
    }

    // Se arriviamo qui, la connessione non è stata trovata
    spdlog::warn("Connection not found in pool during release");
  }
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::releaseConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::getTotalDuration()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::chrono::milliseconds PostgreSQL::Pool::getTotalDuration(const std::unique_ptr<pqxx::connection>& connection)
{
  std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

  if (auto it = PostgreSQL::Pool::connections.find(connection); it != PostgreSQL::Pool::connections.end())
  {
    return it->second.total_duration;
  }

  return std::chrono::milliseconds(0);
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::getTotalDuration()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::getLastDuration()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::chrono::milliseconds PostgreSQL::Pool::getLastDuration(const std::unique_ptr<pqxx::connection>& connection)
{
  std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

  if (auto it = PostgreSQL::Pool::connections.find(connection); it != PostgreSQL::Pool::connections.end())
  {
    return it->second.last_duration;
  }

  return std::chrono::milliseconds(0);
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::getLastDuration()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// Contatore utilizzi per connessione
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::getUsageCount()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int PostgreSQL::Pool::getUsageCount(const std::unique_ptr<pqxx::connection>& connection)
{
  std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

  if (auto it = PostgreSQL::Pool::connections.find(connection); it != PostgreSQL::Pool::connections.end())
  {
    return it->second.usage_count;
  }

  return 0;
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::getUsageCount()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of PostgreSQL::Pool::printConnectionStats()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PostgreSQL::Pool::printConnectionStats()
{
  std::chrono::milliseconds total_duration{0};
  std::size_t total_uses = 0;
  std::size_t active_connections = 0;
  std::size_t idle_connections = 0;
  std::size_t size = 0;

  {
    std::lock_guard<std::mutex> lock(PostgreSQL::Pool::connections_mutex);

    size = PostgreSQL::Pool::connections.size();

    for (const auto& [conn, metrics] : PostgreSQL::Pool::connections)
    {
      total_duration += metrics.total_duration;
      total_uses += metrics.usage_count;

      if (metrics.is_acquired==true)
      {
        active_connections++;
      }
      else
      {
        idle_connections++;
      }
    }
  }

  std::cout << "=== Connection Pool Statistics ===\n"
            << "Total connections: "                  << size << "\n"
            << "Active: "                             << active_connections         << "\n"
            << "Idle: "                               << idle_connections           << "\n"
            << "Total usage time: "                   << std::chrono::duration_cast<std::chrono::milliseconds>(total_duration).count() << "ms\n"
            << "Total operations: "                   << total_uses                 << "\n";

  if (total_uses > 0)
  {
    auto avg_duration = total_duration / total_uses;
    std::cout << "Average operation time: "
              << avg_duration.count() << "μs\n";
  }
}
// -------------------------------------------------------------------------------------------------
// End of PostgreSQL::Pool::printConnectionStats()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------




















/***************************************************************************************************
 *
 *
 * PostgreSQL() Constructor/Destructor
 *
 *
 **************************************************************************************************/
PostgreSQL::PostgreSQL()
{
  spdlog::trace("PostgreSQL init");

  this->pool = std::make_unique<Pool>();

  spdlog::trace("PostgreSQL init done");
}



PostgreSQL::~PostgreSQL()
{
  running_.store(false, std::memory_order_release);
  // spdlog::trace("PostgreSQL pool cleanup complete");
  this->pool.reset();
}
/***************************************************************************************************
 *
 *
 *
 *
 *
 **************************************************************************************************/










std::optional<PostgreSQL::ConnectionWrapper>  PostgreSQL::acquire()                         { return this->pool->acquire(); }
void                                          PostgreSQL::releaseConnection(std::optional<const std::unique_ptr<pqxx::connection>*> connection) {  this->pool->releaseConnection(connection); }






































