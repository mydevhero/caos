#include "DatabaseFwd.hpp"
#include <arpa/inet.h>

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










/***************************************************************************************************
 *
 *
 * Database::Pool() Constructor/Destructor
 *
 *
 **************************************************************************************************/
// std::mutex                        Database::Pool::shutdown_mutex_{}                     ;

// std::condition_variable           Database::Pool::condition{}                           ;
// std::condition_variable           Database::Pool::shutdown_cv_                          ;

// std::atomic<bool>                 Database::Pool::connectionRefused = false             ;

// Database::Pool::config_s Database::Pool::config = {
//   .user                   = ""                          ,
//   .pass                   = ""                          ,
//   .host                   = ""                          ,
//   .port                   = 0                           ,
//   .name                   = ""                          ,
//   .poolsizemin            = 0                           ,
//   .poolsizemax            = 0                           ,
//   .poolwait               = 0                           ,
//   .pooltimeout            = std::chrono::milliseconds(0),
//   .keepalives             = 0                           ,
//   .keepalives_idle        = 0                           ,
//   .keepalives_interval    = 0                           ,
//   .keepalives_count       = 0                           ,
//   .connect_timeout        = 0                           ,
//   .connection_string      = ""                          ,
//   .max_wait               = std::chrono::milliseconds(0),
//   .health_check_interval  = std::chrono::milliseconds(0)
// };

// std::atomic<bool>                 Database::Pool::running_{true}                        ;





Database::Pool::Pool():running_(true),connectionRefused(false)
{
  this->config = {
    .user                   = ""                          ,
    .pass                   = ""                          ,
    .host                   = ""                          ,
    .port                   = 0                           ,
    .name                   = ""                          ,
    .poolsizemin            = 0                           ,
    .poolsizemax            = 0                           ,
    .poolwait               = 0                           ,
    .pooltimeout            = std::chrono::milliseconds(0),
    #ifdef CAOS_USE_DB_POSTGRESQL
    .keepalives             = 0                           ,
    .keepalives_idle        = 0                           ,
    .keepalives_interval    = 0                           ,
    .keepalives_count       = 0                           ,
    .connection_string      = ""                          ,
    #endif

    .connect_timeout        = 0                           ,

    #if (defined(CAOS_USE_DB_MYSQL)||defined(CAOS_USE_DB_MARIADB))
    .connection_options     = {}                          ,
    #endif

    .max_wait               = std::chrono::milliseconds(0),
    .health_check_interval  = std::chrono::milliseconds(0)
  };

  this->terminalPtr     = &TerminalOptions::get_instance();
  this->environmentRef  = &Environment::get_instance();

  setUser()                 ;
  setPass()                 ;
  setHost()                 ;
  setPort()                 ;
  setName()                 ;
  setPoolSizeMin()          ;
  setPoolSizeMax()          ;
  setPoolWait()             ;
  setPoolTimeout()          ;
  #ifdef CAOS_USE_DB_POSTGRESQL
  setKeepAlives()           ;
  setKeepAlivesIdle()       ;
  setKeepAlivesInterval()   ;
  setKeepAlivesCount()      ;
  #endif
  setConnectTimeout()       ;
  setMaxWait()              ;
  setHealthCheckInterval()  ;

  #ifdef CAOS_USE_DB_POSTGRESQL
  setConnectStr()           ;
  #endif

  #if (defined(CAOS_USE_DB_MYSQL)||defined(CAOS_USE_DB_MARIADB))
  setConnectOpt()           ;
  #endif

  this->healthCheckThread_ = std::thread([this]() {
    this->healthCheckLoop ();
  });
}





Database::Pool::~Pool()
{
  this->printConnectionStats();

  this->running_.store(false, std::memory_order_release);

  this->condition.notify_all();

  if (this->healthCheckThread_.joinable())
  {
    this->healthCheckThread_.join();
  }
}
/***************************************************************************************************
 *
 *
 *
 *
 *
 **************************************************************************************************/






















// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setUser()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setUser()
{
  static constexpr const char* fName = "Database::Pool::setUser";

  try
  {
    // Set primary database user for production environment ----------------------------------------
    if (this->terminalPtr->has(CAOS_OPT_DBUSER_NAME))
    {
      this->config.user = this->terminalPtr->get<std::string>(CAOS_OPT_DBUSER_NAME);
    }
    else if (const char* env_addr = std::getenv(CAOS_ENV_DBUSER_NAME))
    {
      this->config.user = env_addr;
    }
    else
    {
      this->config.user = CAOS_DEFAULT_DBUSER;
    }
    // ---------------------------------------------------------------------------------------------


    // Deadly check --------------------------------------------------------------------------------
    if (this->config.user.empty())
    {
      throw std::invalid_argument("DBUSER empty!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database user for test or dev environment -----------------------------------
    if (this->isDevOrTestEnv())
    {
      std::string alternative_user;

      #if defined(CAOS_DEFAULT_DBUSER_ON_DEV_OR_TEST)
        alternative_user = "" CAOS_DEFAULT_DBUSER_ON_DEV_OR_TEST;
      #endif

      if (alternative_user.empty())
      {
        // Alternative database user undefined
        spdlog::warn(missingAlternativeDBUSER, this->environmentRef->getName());
      }
      else
      {
        // Alternative database user defined
        this->config.user = alternative_user;
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
               this->config.user,
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setUser()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setPass()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setPass()
{
  static constexpr const char* fName = "Database::Pool::setPass";

  try
  {
    // Set primary database password for production environment ------------------------------------
    if (this->terminalPtr->has(CAOS_OPT_DBPASS_NAME))
    {
      this->config.pass = this->terminalPtr->get<std::string>(CAOS_OPT_DBPASS_NAME);
    }
    else if (const char* env_addr = std::getenv(CAOS_ENV_DBPASS_NAME))
    {
      this->config.pass = env_addr;
    }
    else
    {
      this->config.pass = CAOS_DEFAULT_DBPASS;
    }
    // ---------------------------------------------------------------------------------------------


    // Deadly check --------------------------------------------------------------------------------
    if (this->config.pass.empty())
    {
      throw std::invalid_argument("DBPASS empty!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database password for test or dev environment -------------------------------
    if (this->isDevOrTestEnv())
    {
      std::string alternative_pass;

      #if defined(CAOS_DEFAULT_DBPASS_ON_DEV_OR_TEST)
        alternative_pass = "" CAOS_DEFAULT_DBPASS_ON_DEV_OR_TEST;
      #endif

      if (alternative_pass.empty())
      {
        // Alternative database password undefined
        spdlog::warn(missingAlternativeDBUSER, this->environmentRef->getName());
      }
      else
      {
        // Alternative database password defined
        this->config.pass = alternative_pass;
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
                  this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setPass()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setHost()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setHost()
{
  static constexpr const char* fName = "Database::Pool::setHost";

  try
  {
    // Set primary database host for production environment ----------------------------------------
    if (this->terminalPtr->has(CAOS_OPT_DBHOST_NAME))
    {
      this->config.host = this->terminalPtr->get<std::string>(CAOS_OPT_DBHOST_NAME);
    }
    else if (const char* env_addr = std::getenv(CAOS_ENV_DBHOST_NAME))
    {
      this->config.host = env_addr;
    }
    else
    {
      this->config.host = CAOS_DEFAULT_DBHOST;
    }
    // ---------------------------------------------------------------------------------------------


    // Deadly check --------------------------------------------------------------------------------
    if (this->config.host.empty())
    {
      throw std::invalid_argument("DBHOST empty!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database host for test or dev environment -----------------------------------
    if (this->isDevOrTestEnv())
    {
      std::string alternative_host;

      #if defined(CAOS_DEFAULT_DBHOST_ON_DEV_OR_TEST)
        alternative_host = "" CAOS_DEFAULT_DBHOST_ON_DEV_OR_TEST;
      #endif

      if (alternative_host.empty())
      {
        // Alternative database host undefined
        spdlog::warn(missingAlternativeDBHOST,this->environmentRef->getName());
      }
      else
      {
        // Alternative database host defined
        this->config.host = alternative_host;
      }
    }
    // ---------------------------------------------------------------------------------------------



    // Validation ----------------------------------------------------------------------------------
    struct sockaddr_in sa4={};
    struct sockaddr_in6 sa6={};

    if (inet_pton(AF_INET, this->config.host.c_str(), &(sa4.sin_addr)) == 0
        && inet_pton(AF_INET6, this->config.host.c_str(), &(sa6.sin6_addr)) == 0)
    {
      throw std::invalid_argument("Invalid Address: " + this->config.host);
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
               this->config.host,
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setHost()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setPort()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setPort()
{
  static constexpr const char* fName = "Database::Pool::setPort";

  try
  {
    // Set primary database port for production environment ----------------------------------------
    const char* port_str = std::getenv(CAOS_ENV_DBPORT_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBPORT_NAME))
    {
      this->config.port = this->terminalPtr->get<std::uint16_t>(CAOS_OPT_DBPORT_NAME);
    }
    else if (port_str != nullptr)
    {
      this->config.port = static_cast<std::uint16_t>(std::stoi(port_str));
    }
    else
    {
      this->config.port = CAOS_DEFAULT_DBPORT;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.port == 0)
    {
      throw std::invalid_argument("DBPORT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database port for test or dev environment -----------------------------------
    if (this->isDevOrTestEnv())
    {
      std::uint16_t alternative_port=0;

      #if defined(CAOS_DEFAULT_DBPORT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPORT_ON_DEV_OR_TEST + 0) > 0
      alternative_port = static_cast<std::uint16_t>(CAOS_DEFAULT_DBPORT_ON_DEV_OR_TEST);
      #endif

      if (alternative_port == 0)
      {
        // Alternative database port undefined
        spdlog::warn(missingAlternativeDBPORT,this->environmentRef->getName());
      }
      else
      {
        // Alternative database port defined
        this->config.port = alternative_port;
      }
    }
    // ---------------------------------------------------------------------------------------------



    // Validation ----------------------------------------------------------------------------------
    if(this->config.port < CAOS_UNPRIVILEGED_PORT_MIN || this->config.port > CAOS_UNPRIVILEGED_PORT_MAX)
    {
      throw std::out_of_range("TCP Port out of range: " + std::to_string(this->config.port));
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
               this->config.port,
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setPort()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setName()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setName()
{
  static constexpr const char* fName = "Database::Pool::setName";

  try
  {
    // Set primary database name for production environment ----------------------------------------
    if (this->terminalPtr->has(CAOS_OPT_DBNAME_NAME))
    {
      this->config.name = this->terminalPtr->get<std::string>(CAOS_OPT_DBNAME_NAME);
    }
    else if (const char* env_addr = std::getenv(CAOS_ENV_DBNAME_NAME))
    {
      this->config.name = env_addr;
    }
    else
    {
      this->config.name = CAOS_DEFAULT_DBNAME;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.name.empty())
    {
      throw std::invalid_argument("DBNAME empty!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database name for test or dev environment -----------------------------------
    if (this->isDevOrTestEnv())
    {
      std::string alternative_name;

      #if defined(CAOS_DEFAULT_DBNAME_ON_DEV_OR_TEST)
        alternative_name = "" CAOS_DEFAULT_DBNAME_ON_DEV_OR_TEST;
      #endif

      if (alternative_name.empty())
      {
        // Alternative database name undefined
        spdlog::warn(missingAlternativeDBNAME,this->environmentRef->getName());

        char defaultAnswer = 'N';

        std::cout << "Do you want to use production database in " << this->environmentRef->getName() << "? [y/N] ";
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
        this->config.name = alternative_name;
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
               this->config.name,
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setName()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setPoolSizeMin()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setPoolSizeMin()
{
  static constexpr const char* fName = "Database::Pool::setPoolSizeMin";

  try
  {
    // Set primary database poolsizemin for production environment ------------------------------------
    const char* poolsizemin = std::getenv(CAOS_ENV_DBPOOLSIZEMIN_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBPOOLSIZEMIN_NAME))
    {
      this->config.poolsizemin = this->terminalPtr->get<std::size_t>(CAOS_OPT_DBPOOLSIZEMIN_NAME);
    }
    else if (poolsizemin != nullptr)
    {
      this->config.poolsizemin = static_cast<std::size_t>(std::stoi(poolsizemin));
    }
    else
    {
      this->config.poolsizemin = CAOS_DEFAULT_DBPOOLSIZEMIN;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.poolsizemin == 0)
    {
      throw std::invalid_argument("DBPOOLSIZEMIN unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database poolsizemin for test or dev environment -------------------------------
    if (this->isDevOrTestEnv())
    {
      std::size_t alternative_poolsizemin = 0;

      #if defined(CAOS_DEFAULT_DBPOOLSIZEMIN_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPOOLSIZEMIN_ON_DEV_OR_TEST + 0) > 0
      alternative_poolsizemin = static_cast<std::size_t>(CAOS_DEFAULT_DBPOOLSIZEMIN_ON_DEV_OR_TEST);
      #endif

      if (alternative_poolsizemin == 0)
      {
        // Alternative database poolsizemin undefined
        spdlog::warn(missingAlternativeDBPOOLSIZEMIN,this->environmentRef->getName());
      }
      else
      {
        // Alternative database poolsizemin defined
        this->config.poolsizemin = alternative_poolsizemin;
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
               this->config.poolsizemin,
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setPoolSizeMin()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------









// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setPoolSizeMax()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setPoolSizeMax()
{
  static constexpr const char* fName = "Database::Pool::setPoolSizeMax";

  try
  {
    // Set primary database poolsizemax for production environment ---------------------------------
    const char* poolsizemax = std::getenv(CAOS_ENV_DBPOOLSIZEMAX_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBPOOLSIZEMAX_NAME))
    {
      this->config.poolsizemax = this->terminalPtr->get<std::size_t>(CAOS_OPT_DBPOOLSIZEMAX_NAME);
    }
    else if (poolsizemax != nullptr)
    {
      this->config.poolsizemax = static_cast<std::size_t>(std::stoi(poolsizemax));
    }
    else
    {
      this->config.poolsizemax = CAOS_DEFAULT_DBPOOLSIZEMAX;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.poolsizemax == 0)
    {
      throw std::invalid_argument("DBPOOLSIZEMAX unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database poolsizemax for test or dev environment -------------------------------
    if (this->isDevOrTestEnv())
    {
      std::size_t alternative_poolsizemax = 0;

      #if defined(CAOS_DEFAULT_DBPOOLSIZEMAX_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPOOLSIZEMAX_ON_DEV_OR_TEST + 0) > 0
      alternative_poolsizemax = static_cast<std::size_t>(CAOS_DEFAULT_DBPOOLSIZEMAX_ON_DEV_OR_TEST);
      #endif

      if (alternative_poolsizemax == 0)
      {
        // Alternative database poolsizemax undefined
        spdlog::warn(missingAlternativeDBPOOLSIZEMAX,this->environmentRef->getName());
      }
      else
      {
        // Alternative database poolsizemax defined
        this->config.poolsizemax = alternative_poolsizemax;
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
               this->config.poolsizemax,
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setPoolSizeMax()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setPoolWait()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setPoolWait()
{
  static constexpr const char* fName = "Database::Pool::setPoolWait";

  try
  {
    // Set primary database poolwait for production environment ------------------------------------
    const char* poolwait = std::getenv(CAOS_ENV_DBPOOLWAIT_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBPOOLWAIT_NAME))
    {
      this->config.poolwait = this->terminalPtr->get<std::uint32_t>(CAOS_OPT_DBPOOLWAIT_NAME);
    }
    else if (poolwait != nullptr)
    {
      this->config.poolwait = static_cast<std::uint32_t>(std::stoi(poolwait));
    }
    else
    {
      this->config.poolwait = CAOS_DEFAULT_DBPOOLWAIT;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.poolwait == 0)
    {
      throw std::invalid_argument("DBPOOLWAIT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------


    // Set alternative database poolwait for test or dev environment -------------------------------
    if (this->isDevOrTestEnv())
    {
      std::uint32_t alternative_poolwait = 0;

      #if defined(CAOS_DEFAULT_DBPOOLWAIT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPOOLWAIT_ON_DEV_OR_TEST + 0) > 0
      alternative_poolwait = static_cast<std::uint32_t>(CAOS_DEFAULT_DBPOOLWAIT_ON_DEV_OR_TEST);
      #endif

      if (alternative_poolwait == 0)
      {
        // Alternative database poolwait undefined
        spdlog::warn(missingAlternativeDBPOOLWAIT,this->environmentRef->getName());
      }
      else
      {
        // Alternative database poolwait defined
        this->config.poolwait = alternative_poolwait;
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
               this->config.poolwait,
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setPoolWait()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setPoolTimeout()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setPoolTimeout()
{
  static constexpr const char* fName = "Database::Pool::setPoolTimeout()";

  try
  {
    // Set primary database pooltimeout for production environment ---------------------------------
    const char* pooltimeout = std::getenv(CAOS_ENV_DBPOOLTIMEOUT_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBPOOLTIMEOUT_NAME))
    {
      this->config.pooltimeout = std::chrono::milliseconds(this->terminalPtr->get<std::uint32_t>(CAOS_OPT_DBPOOLTIMEOUT_NAME));
    }
    else if (pooltimeout != nullptr)
    {
      this->config.pooltimeout = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(pooltimeout)));
    }
    else
    {
      this->config.pooltimeout = std::chrono::milliseconds(CAOS_DEFAULT_DBPOOLTIMEOUT);
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.pooltimeout == std::chrono::milliseconds(0))
    {
      throw std::invalid_argument("DBPOOLTIMEOUT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database pooltimeout for test or dev environment ----------------------------
    if (this->isDevOrTestEnv())
    {
      std::chrono::milliseconds alternative_pooltimeout(0);

      #if defined(CAOS_DEFAULT_DBPOOLTIMEOUT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBPOOLTIMEOUT_ON_DEV_OR_TEST + 0) > 0
      alternative_pooltimeout = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(CAOS_DEFAULT_DBPOOLTIMEOUT_ON_DEV_OR_TEST)));
      #endif

      if (alternative_pooltimeout == std::chrono::milliseconds(0))
      {
        // Alternative database pooltimeout undefined
        spdlog::warn(missingAlternativeDBPOOLTIMEOUT,this->environmentRef->getName());
      }
      else
      {
        // Alternative database pooltimeout defined
        this->config.pooltimeout = alternative_pooltimeout;
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
               static_cast<uint32_t>(this->config.pooltimeout.count()),
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setPoolTimeout()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setKeepAlives()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef CAOS_USE_DB_POSTGRESQL
void Database::Pool::setKeepAlives()
{
  static constexpr const char* fName = "Database::Pool::setKeepAlives";

  try
  {
    // Set primary database keepalives for production environment ----------------------------------
    const char* keepalives = std::getenv(CAOS_ENV_DBKEEPALIVES_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBKEEPALIVES_NAME))
    {
      this->config.keepalives = this->terminalPtr->get<std::size_t>(CAOS_OPT_DBKEEPALIVES_NAME);
    }
    else if (keepalives != nullptr)
    {
      this->config.keepalives = static_cast<std::size_t>(std::stoi(keepalives));
    }
    else
    {
      this->config.keepalives = CAOS_DEFAULT_DBKEEPALIVES;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.keepalives == 0)
    {
      throw std::invalid_argument("DBKEEPALIVES unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database keepalives for test or dev environment -----------------------------
    if (this->isDevOrTestEnv())
    {
      std::size_t alternative_keepalives = 0;

      #if defined(CAOS_DEFAULT_DBKEEPALIVES_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBKEEPALIVES_ON_DEV_OR_TEST + 0) > 0
      alternative_keepalives = static_cast<std::size_t>(CAOS_DEFAULT_DBKEEPALIVES_ON_DEV_OR_TEST);
      #endif

      if (alternative_keepalives == 0)
      {
        // Alternative database keepalives undefined
        spdlog::warn(missingAlternativeDBKEEPALIVES,this->environmentRef->getName());
      }
      else
      {
        // Alternative database keepalives defined
        this->config.keepalives = alternative_keepalives;
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
               this->config.keepalives,
               this->environmentRef->getName());
}
#endif
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setKeepAlives()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setKeepAlivesIdle()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef CAOS_USE_DB_POSTGRESQL
void Database::Pool::setKeepAlivesIdle()
{
  static constexpr const char* fName = "Database::Pool::setKeepAlivesIdle";

  try
  {
    // Set primary database keepalives_idle for production environment -----------------------------
    const char* keepalives_idle = std::getenv(CAOS_ENV_DBKEEPALIVES_IDLE_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBKEEPALIVES_IDLE_NAME))
    {
      this->config.keepalives_idle = this->terminalPtr->get<std::size_t>(CAOS_OPT_DBKEEPALIVES_IDLE_NAME);
    }
    else if (keepalives_idle != nullptr)
    {
      this->config.keepalives_idle = static_cast<std::size_t>(std::stoi(keepalives_idle));
    }
    else
    {
      this->config.keepalives_idle = CAOS_DEFAULT_DBKEEPALIVES_IDLE;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.keepalives_idle == 0)
    {
      throw std::invalid_argument("DBKEEPALIVES_IDLE unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database keepalives_idle for test or dev environment ------------------------
    if (this->isDevOrTestEnv())
    {
      std::size_t alternative_keepalives_idle = 0;

      #if defined(CAOS_DEFAULT_DBKEEPALIVES_IDLE_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBKEEPALIVES_IDLE_ON_DEV_OR_TEST + 0) > 0
      alternative_keepalives_idle = static_cast<std::size_t>(CAOS_DEFAULT_DBKEEPALIVES_IDLE_ON_DEV_OR_TEST);
      #endif

      if (alternative_keepalives_idle == 0)
      {
        // Alternative database keepalives_idle undefined
        spdlog::warn(missingAlternativeDBKEEPALIVES_IDLE,this->environmentRef->getName());
      }
      else
      {
        // Alternative database keepalives_idle defined
        this->config.keepalives_idle = alternative_keepalives_idle;
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
               this->config.keepalives_idle,
               this->environmentRef->getName());
}
#endif
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setKeepAlivesIdle()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setKeepAlivesInterval()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef CAOS_USE_DB_POSTGRESQL
void Database::Pool::setKeepAlivesInterval()
{
  static constexpr const char* fName = "Database::Pool::setKeepAlivesInterval";

  try
  {
    // Set primary database keepalives_interval for production environment -------------------------
    const char* keepalives_interval = std::getenv(CAOS_ENV_DBKEEPALIVES_INTERVAL_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBKEEPALIVES_INTERVAL_NAME))
    {
      this->config.keepalives_interval = this->terminalPtr->get<std::size_t>(CAOS_OPT_DBKEEPALIVES_INTERVAL_NAME);
    }
    else if (keepalives_interval != nullptr)
    {
      this->config.keepalives_interval = static_cast<std::size_t>(std::stoi(keepalives_interval));
    }
    else
    {
      this->config.keepalives_interval = CAOS_DEFAULT_DBKEEPALIVES_INTERVAL;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.keepalives_interval == 0)
    {
      throw std::invalid_argument("DBKEEPALIVES_INTERVAL unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database keepalives_interval for test or dev environment --------------------
    if (this->isDevOrTestEnv())
    {
      std::size_t alternative_keepalives_interval = 0;

      #if defined(CAOS_DEFAULT_DBKEEPALIVES_INTERVAL_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBKEEPALIVES_INTERVAL_ON_DEV_OR_TEST + 0) > 0
      alternative_keepalives_interval = static_cast<std::size_t>(CAOS_DEFAULT_DBKEEPALIVES_INTERVAL_ON_DEV_OR_TEST);
      #endif

      if (alternative_keepalives_interval == 0)
      {
        // Alternative database keepalives_interval undefined
        spdlog::warn(missingAlternativeDBKEEPALIVES_INTERVAL,this->environmentRef->getName());
      }
      else
      {
        // Alternative database keepalives_interval defined
        this->config.keepalives_interval = alternative_keepalives_interval;
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
               this->config.keepalives_interval,
               this->environmentRef->getName());
}
#endif
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setKeepAlivesInterval()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::setKeepAlivesCount()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef CAOS_USE_DB_POSTGRESQL
void Database::Pool::setKeepAlivesCount()
{
  static constexpr const char* fName = "Database::Pool::setKeepAlivesCount";

  try
  {
    // Set primary database keepalives_count for production environment ----------------------------
    const char* keepalives_count = std::getenv(CAOS_ENV_DBKEEPALIVES_COUNT_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBKEEPALIVES_COUNT_NAME))
    {
      this->config.keepalives_count = this->terminalPtr->get<std::size_t>(CAOS_OPT_DBKEEPALIVES_COUNT_NAME);
    }
    else if (keepalives_count != nullptr)
    {
      this->config.keepalives_count = static_cast<std::size_t>(std::stoi(keepalives_count));
    }
    else
    {
      this->config.keepalives_count = CAOS_DEFAULT_DBKEEPALIVES_COUNT;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.keepalives_count == 0)
    {
      throw std::invalid_argument("DBKEEPALIVES_COUNT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database keepalives_count for test or dev environment -----------------------
    if (this->isDevOrTestEnv())
    {
      std::size_t alternative_keepalives_count = 0;

      #if defined(CAOS_DEFAULT_DBKEEPALIVES_COUNT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBKEEPALIVES_COUNT_ON_DEV_OR_TEST + 0) > 0
      alternative_keepalives_count = static_cast<std::size_t>(CAOS_DEFAULT_DBKEEPALIVES_COUNT_ON_DEV_OR_TEST);
      #endif

      if (alternative_keepalives_count == 0)
      {
        // Alternative database keepalives_count undefined
        spdlog::warn(missingAlternativeDBKEEPALIVES_COUNT,this->environmentRef->getName());
      }
      else
      {
        // Alternative database keepalives_count defined
        this->config.keepalives_count = alternative_keepalives_count;
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
               this->config.keepalives_count,
               this->environmentRef->getName());
}
#endif
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setKeepAlivesCount()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setConnectTimeout()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setConnectTimeout()
{
  static constexpr const char* fName = "Database::Pool::setConnectTimeout";

  try
  {
    // Set primary database connect_timeout for production environment -----------------------------
    const char* connect_timeout = std::getenv(CAOS_ENV_DBCONNECT_TIMEOUT_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBCONNECT_TIMEOUT_NAME))
    {
      this->config.connect_timeout = this->terminalPtr->get<std::size_t>(CAOS_OPT_DBCONNECT_TIMEOUT_NAME);
    }
    else if (connect_timeout != nullptr)
    {
      this->config.connect_timeout = static_cast<std::size_t>(std::stoi(connect_timeout));
    }
    else
    {
      this->config.connect_timeout = CAOS_DEFAULT_DBCONNECT_TIMEOUT;
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.connect_timeout == 0)
    {
      throw std::invalid_argument("DBCONNECT_TIMEOUT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database connect_timeout for test or dev environment ------------------------
    if (this->isDevOrTestEnv())
    {
      std::size_t alternative_connect_timeout = 0;

      #if defined(CAOS_DEFAULT_DBCONNECT_TIMEOUT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBCONNECT_TIMEOUT_ON_DEV_OR_TEST + 0) > 0
      alternative_connect_timeout = static_cast<std::size_t>(CAOS_DEFAULT_DBCONNECT_TIMEOUT_ON_DEV_OR_TEST);
      #endif

      if (alternative_connect_timeout == 0)
      {
        // Alternative database connect_timeout undefined
        spdlog::warn(missingAlternativeDBCONNECT_TIMEOUT,this->environmentRef->getName());
      }
      else
      {
        // Alternative database connect_timeout defined
        this->config.connect_timeout = alternative_connect_timeout;
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
               this->config.connect_timeout,
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setConnectTimeout()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setMaxWait()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setMaxWait()
{
  static constexpr const char* fName = "Database::Pool::setMaxWait";

  try
  {
    // Set primary database poolwait for production environment ------------------------------------
    const char* max_wait = std::getenv(CAOS_ENV_DBMAXWAIT_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBMAXWAIT_NAME))
    {
      this->config.max_wait = std::chrono::milliseconds(this->terminalPtr->get<std::uint32_t>(CAOS_OPT_DBMAXWAIT_NAME));
    }
    else if (max_wait != nullptr)
    {
      this->config.max_wait = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(max_wait)));
    }
    else
    {
      this->config.max_wait = std::chrono::milliseconds(CAOS_DEFAULT_DBMAXWAIT);
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.max_wait == std::chrono::milliseconds(0))
    {
      throw std::invalid_argument("DBMAXWAIT unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database max_wait for test or dev environment -------------------------------
    if (this->isDevOrTestEnv())
    {
      std::chrono::milliseconds alternative_max_wait(0);

      #if defined(CAOS_DEFAULT_DBMAXWAIT_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBMAXWAIT_ON_DEV_OR_TEST + 0) > 0
      alternative_max_wait = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(CAOS_DEFAULT_DBMAXWAIT_ON_DEV_OR_TEST)));
      #endif

      if (alternative_max_wait == std::chrono::milliseconds(0))
      {
        // Alternative database max_wait undefined
        spdlog::warn(missingAlternativeDBMAXWAIT,this->environmentRef->getName());
      }
      else
      {
        // Alternative database max_wait defined
        this->config.max_wait = alternative_max_wait;
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
               static_cast<uint32_t>(this->config.max_wait.count()),
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setMaxWait()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------











// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setHealthCheckInterval()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setHealthCheckInterval()
{
  static constexpr const char* fName = "Database::Pool::setHealthCheckInterval";

  try
  {
    // Set primary database health_check_interval for production environment -----------------------
    const char* health_check_interval = std::getenv(CAOS_ENV_DBHEALTHCHECKINTERVAL_NAME);

    if (this->terminalPtr->has(CAOS_OPT_DBHEALTHCHECKINTERVAL_NAME))
    {
      this->config.health_check_interval = std::chrono::milliseconds(this->terminalPtr->get<std::uint32_t>(CAOS_OPT_DBHEALTHCHECKINTERVAL_NAME));
    }
    else if (health_check_interval != nullptr)
    {
      this->config.health_check_interval = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(health_check_interval)));
    }
    else
    {
      this->config.health_check_interval= std::chrono::milliseconds(CAOS_DEFAULT_DBHEALTHCHECKINTERVAL);
    }
    // ---------------------------------------------------------------------------------------------



    // Deadly check --------------------------------------------------------------------------------
    if (this->config.health_check_interval == std::chrono::milliseconds(0))
    {
      throw std::invalid_argument("DBHEALTHCHECKINTERVAL unset or 0!");
    }
    // ---------------------------------------------------------------------------------------------



    // Set alternative database poolwait for test or dev environment -------------------------------
    if (this->isDevOrTestEnv())
    {
      std::chrono::milliseconds alternative_health_check_interval(0);

      #if defined(CAOS_DEFAULT_DBHEALTHCHECKINTERVAL_ON_DEV_OR_TEST) && (CAOS_DEFAULT_DBHEALTHCHECKINTERVAL_ON_DEV_OR_TEST + 0) > 0
      alternative_health_check_interval = std::chrono::milliseconds(static_cast<std::chrono::milliseconds>(std::stoi(CAOS_DEFAULT_DBHEALTHCHECKINTERVAL_ON_DEV_OR_TEST)));
      #endif

      if (alternative_health_check_interval == std::chrono::milliseconds(0))
      {
        // Alternative database health_check_interval undefined
        spdlog::warn(missingAlternativeDBHEALTHCHECKINTERVAL,this->environmentRef->getName());
      }
      else
      {
        // Alternative database health_check_interval defined
        this->config.health_check_interval = alternative_health_check_interval;
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
               static_cast<uint32_t>(this->config.health_check_interval.count()),
               this->environmentRef->getName());
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setHealthCheckInterval()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










const std::string               Database::Pool::getUser()                 const noexcept { return this->config.user;                  }
const std::string               Database::Pool::getPass()                 const noexcept { return this->config.pass;                  }
const std::string               Database::Pool::getHost()                 const noexcept { return this->config.host;                  }
const std::uint16_t             Database::Pool::getPort()                 const noexcept { return this->config.port;                  }
const std::string               Database::Pool::getName()                 const noexcept { return this->config.name;                  }
const std::size_t               Database::Pool::getPoolSizeMin()          const noexcept { return this->config.poolsizemin;           }
const std::size_t               Database::Pool::getPoolSizeMax()          const noexcept { return this->config.poolsizemax;           }
const std::uint32_t             Database::Pool::getPoolWait()             const noexcept { return this->config.poolwait;              }
const std::chrono::milliseconds Database::Pool::getPoolTimeout()          const noexcept { return this->config.pooltimeout;           }
#ifdef CAOS_USE_DB_POSTGRESQL
const std::size_t               Database::Pool::getKeepAlives()           const noexcept { return this->config.keepalives;            }
const std::size_t               Database::Pool::getKeepAlivesIdle()       const noexcept { return this->config.keepalives_idle;       }
const std::size_t               Database::Pool::getKeepAlivesInterval()   const noexcept { return this->config.keepalives_interval;   }
const std::size_t               Database::Pool::getKeepAlivesCount()      const noexcept { return this->config.keepalives_count;      }
const std::string&              Database::Pool::getConnectStr()           const noexcept { return this->config.connection_string;     }
#endif
const std::size_t               Database::Pool::getConnectTimeout()       const noexcept { return this->config.connect_timeout;       }

#if (defined(CAOS_USE_DB_MYSQL)||defined(CAOS_USE_DB_MARIADB))
sql::ConnectOptionsMap&         Database::Pool::getConnectOpt()                 noexcept { return this->config.connection_options;    }
#endif

const std::chrono::milliseconds Database::Pool::getMaxWait()              const noexcept { return this->config.max_wait;              }
const std::chrono::milliseconds Database::Pool::getHealthCheckInterval()  const noexcept { return this->config.health_check_interval; }
const bool                      Database::Pool::isDevOrTestEnv()          const noexcept { return this->environmentRef->getEnv() == Environment::ENV::dev || this->environmentRef->getEnv() == Environment::ENV::test; }










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::get_metrics()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Database::Pool::Metrics Database::Pool::get_metrics() const noexcept
// {
//   std::lock_guard<std::mutex> lock(Database::Pool::available_mutex_);

//   return Metrics
//   {
//     Database::Pool::available_.size(),
//     total_known_.load(std::memory_order_relaxed),
//     creations_.load(std::memory_order_relaxed),
//     failures_.load(std::memory_order_relaxed),
//   };
// }
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::get_metrics()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::getAvailableConnections()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const std::size_t Database::Pool::getAvailableConnections()  noexcept
{
  std::size_t count = 0;

  Database::Pool::PoolData& pool = this->getPoolData();

  std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

  // Cerca la unique_ptr corrispondente nella mappa
  for (auto it = pool.connections.begin(); it != pool.connections.end(); ++it)
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
// End of Database::Pool::getAvailableConnections()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::getTotalConnections()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const std::size_t Database::Pool::getTotalConnections() noexcept
{
  Database::Pool::PoolData& pool = this->getPoolData();

  std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

  return pool.connections.size();
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::getTotalConnections()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::checkPoolSize()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
const bool Database::Pool::checkPoolSize(std::size_t& askingPoolSize) noexcept
{
  static constexpr const char* fName = "Database::Pool::checkPoolSize";

  const std::size_t totalConnections  = this->getTotalConnections();
  const std::size_t poolSizeMax       = this->getPoolSizeMax();

  if (totalConnections >= poolSizeMax)  // Don't saturate Database connections
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
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::checkPoolSize()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::init()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::size_t Database::Pool::init(std::size_t count)
{
  static constexpr const char* fName = "Database::Pool::init";

  std::size_t i = 0;
  std::size_t pool_size = (count>0) ? count : this->getPoolSizeMin();

  spdlog::info("[{}] New pool building", fName);

  while (i < pool_size                                                                              // Create connections until the requested size is reached
         && running_.load(std::memory_order_acquire)                                                // Stop if a signal is detect
         && !this->connectionRefused.load(std::memory_order_acquire))                   // Stop if a previous connection was refused
  {
    bool connectionResult = false;

    try
    {
      connectionResult = this->createConnection(pool_size);
    }
    catch (const repository::broken_connection& e)
    {
      throw;
    }

    i++;

    spdlog::trace("[{}] New Database connection established", fName);
  }

  return i;
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::init()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::getPoolData()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Database::Pool::PoolData& Database::Pool::getPoolData() noexcept
{
  static PoolData instance(this->getPoolSizeMax());                                     // Lazy init, thread-safe in C++11
  return instance;
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::getPoolData()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------












// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::healthCheckLoop()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::healthCheckLoop()
{
  static constexpr const char* fName = "Database::Pool::healthCheckLoop";

  while (this->running_.load(std::memory_order_acquire))
  {
    spdlog::trace("Running {}", fName);

    {
      Database::Pool::PoolData& pool = this->getPoolData();

      std::unique_lock<std::mutex> lock(pool.connections_mutex);

      try
      {
        if (!pool.connections.empty())
        {
          spdlog::debug("Performing health check on existing connections");

          for (auto it = pool.connections.begin(); it != pool.connections.end(); ++it)
          {
            auto& [connection, metrics] = *it;

            if (!metrics.is_acquired)
            {
              metrics.is_acquired = true;                                                           // Don't let threads acquire this connection while validating

              if (this->validateConnection(connection))
              {
                spdlog::trace("[{}] Connection is valid", fName);

                metrics.is_acquired = false;
              }
              else
              {
                spdlog::info("[{}] Invalid connection marked for removal", fName);
                pool.connectionsToRemove.push_back(it);
              }
            }
          }
        }

        this->cleanupMarkedConnections();                                               // Remove invalid connections

        std::size_t totalConnections = pool.connections.size();

        int connectionDiff = static_cast<int>(totalConnections - this->getPoolSizeMin());

        if (connectionDiff < 0)
        {
          this->init(static_cast<std::size_t>(-connectionDiff));                        // Refill pool
        }
      }
      catch (const repository::broken_connection& e)
      {
        spdlog::error("[{}] Database unreachable or port closed", fName);
      }
    }

    condition.notify_all();

    {
      std::unique_lock<std::mutex> waitlock(this->shutdown_mutex_);
      shutdown_cv_.wait_for(
            waitlock,
            this->getHealthCheckInterval(),
            [this]{
                this->connectionRefused = false;
                return !this->running_.load(std::memory_order_acquire);
            }
      );
    }
  }
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::healthCheckLoop()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::calculateAverageDuration()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::chrono::milliseconds Database::Pool::calculateAverageDuration()
{
  std::chrono::milliseconds total_duration{0};
  int total_operations = 0;

  {
    Database::Pool::PoolData& pool = this->getPoolData();

    std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

    if (pool.connections.empty())
    {
      return std::chrono::milliseconds(0);
    }

    for (const auto& [connection, metrics] : pool.connections)
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
// End of Database::Pool::calculateAverageDuration()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::acquireConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define INIT_CONNECTION()                         \
  try                                             \
  {                                               \
    this->init(1);                    \
  }                                               \
  catch (const repository::broken_connection& e)  \
  {                                               \
    throw;                                        \
  }

dboptuniqptr Database::Pool::acquireConnection()
{
  static constexpr const char* fName = "Database::Pool::acquireConnection";

  if (!this->running_.load(std::memory_order_acquire))
  {
    return std::nullopt;
  }

  {
    Database::Pool::PoolData& pool = this->getPoolData();

    std::unique_lock<std::mutex> lock(pool.connections_mutex);

    if (pool.connections.empty())
    {
      // Pool is empty! Do not await healthCheckLoop(). Create emergency connection
      INIT_CONNECTION()
    }
    else
    {
      for (auto it = pool.connections.begin(); it != pool.connections.end(); ++it)
      {
        const auto& connection  = it->first                                                   ;
        auto&       metrics     = it->second                                                  ;

        if (!metrics.is_acquired)
        {
          try
          {
            #if VALIDATE_CONNECTION_BEFORE_ACQUIRE==1
            if (this->validateConnection(connection))
            {
            #else
            if (connection->is_open())
            {
            #endif
              auto now = std::chrono::steady_clock::now()                                     ;
              metrics.start_time    = now                                                     ;
              metrics.last_acquired = now                                                     ;
              metrics.is_acquired   = true                                                    ;
              metrics.usage_count++                                                           ;

              return &connection                                                              ;
            }

            throw repository::broken_connection("Connection lost");

          }
          catch (const repository::broken_connection& e)
          {
            // Remove connection
            pool.connectionsToRemove.push_back(it);
            this->cleanupMarkedConnections();

            throw;
          }
        }
      }

      // Pool count is not enough! Try to create an emergency connection
      INIT_CONNECTION()
    }
  }

  static std::atomic<std::size_t> limiter                                     {0}         ;

  std::size_t current = limiter.fetch_add(1, std::memory_order_acq_rel) + 1               ;

  if (current >= CAOS_LOG_THRESHOLD_CONNECTION_LIMIT_EXCEEDED)
  {
    std::size_t expected = current;

    if (limiter.compare_exchange_strong(expected, 0,
                                        std::memory_order_release,
                                        std::memory_order_relaxed))
    {
      spdlog::warn("Unable to acquire Database connection - {} failed requests (pool busy)", current);
      spdlog::info("Database connection limit exceeded: reduce thread pool size or increase max_connections parameter");
    }
  }

  return std::nullopt;
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::acquireConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::handleInvalidConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::handleInvalidConnection(const dbuniq& connection)
{
  std::size_t newConnection = this->init(1);

  if (newConnection==1)
  {
    this->condition.notify_all();
  }

}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::handleInvalidConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::cleanupMarkedConnections()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::cleanupMarkedConnections()
{
  static constexpr const char* fName = "Database::Pool::cleanupMarkedConnections";

  Database::Pool::PoolData& pool = this->getPoolData();

  std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

  for (auto it : pool.connectionsToRemove)
  {
    try
    {
      if (it != pool.connections.end())
      {
        auto& [connection, metrics] = *it;
        connection->close();
        spdlog::debug("[{}] Removed connection (used {} times)", fName, metrics.usage_count);
        pool.connections.erase(it);
      }
    }
    catch (const std::exception& e)
    {
      spdlog::error("[{}] Error removing connection: {}", fName, e.what());
    }
  }

  pool.connectionsToRemove.clear();
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::cleanupMarkedConnections()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::cleanupIdleConnections()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// void Database::Pool::cleanupIdleConnections()
// {
//   std::lock_guard<std::mutex> lock(Database::Pool::connections_mutex);
//   auto now = std::chrono::steady_clock::now();

//   for (auto it = Database::Pool::connections.begin(); it != Database::Pool::connections.end(); )
//   {
//     if (Database::Pool::connections.size()<=Database::Pool::getPoolSizeMin())
//     {
//       break;
//     }
//     const auto& [connection, metrics] = *it;

//     // Convert weak_ptr into a shared_ptr

//     // auto idle_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - metrics.end_time);

//     // std::cout << (metrics.is_acquired.load(std::memory_order_acquire) ? "ACQUIRED" : "IDLE")
//     //           << " - Idle time: " << idle_time.count() << "ms"
//     //           << " - Timeout: " << Database::Pool::getPoolTimeout().count() << "ms\n";

//     // Se la connessione non è in uso ed è inattiva da troppo tempo
//     if (!metrics.is_acquired && ((now - metrics.end_time) > Database::Pool::getPoolTimeout()))
//     {
//       // Log delle metriche prima di rimuovere
//       auto total_usage_ms = std::chrono::duration_cast<std::chrono::milliseconds>(metrics.total_duration);
//       std::cout << "Closing connection - Total usage: "
//                 << total_usage_ms.count() << "ms, "
//                 << "Used " << metrics.usage_count << " times\n";

//       // USA LO shared_ptr OTTENUTO DAL LOCK
//       Database::Pool::closeConnection(connection);
//     }
//     else
//     {
//       ++it;
//     }
//   }
// }
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::cleanupIdleConnections()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::acquire()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::optional<Database::ConnectionWrapper> Database::Pool::acquire()
{
  try
  {
    auto connection_opt = this->acquireConnection();

    if (connection_opt) {
        return ConnectionWrapper(
            connection_opt, // Estrai il pointer dall'optional
            [this](dboptuniqptr conn_ptr){
              this->releaseConnection(conn_ptr);
            });
    }
  }
  catch (const repository::broken_connection& e)
  {
    throw;
  }

  return std::nullopt;
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::acquire()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::releaseConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::releaseConnection(dboptuniqptr connection_opt)
{
  if (connection_opt.has_value())
  {
    const dbuniq* unique_ptr_ptr = connection_opt.value();

    Database::Pool::PoolData& pool = this->getPoolData();

    std::unique_lock lock(pool.connections_mutex, std::try_to_lock);

    for (auto it = pool.connections.begin(); it != pool.connections.end(); ++it)
    {
      if (&it->first == unique_ptr_ptr)
      {
        auto& metrics = it->second;
        auto now = std::chrono::steady_clock::now();
        metrics.end_time = now;
        metrics.last_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - metrics.start_time);
        metrics.total_duration += metrics.last_duration;
        metrics.is_acquired = false;

        Database::Pool::condition.notify_one();

        return;
      }
    }

    spdlog::warn("Connection not found in pool during release");
  }
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::releaseConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::getTotalDuration()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::chrono::milliseconds Database::Pool::getTotalDuration(const dbuniq& connection)
{
  Database::Pool::PoolData& pool = this->getPoolData();

  std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

  if (auto it = pool.connections.find(connection); it != pool.connections.end())
  {
    return it->second.total_duration;
  }

  return std::chrono::milliseconds(0);
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::getTotalDuration()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::getLastDuration()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
std::chrono::milliseconds Database::Pool::getLastDuration(const dbuniq& connection)
{
  Database::Pool::PoolData& pool = this->getPoolData();

  std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

  if (auto it = pool.connections.find(connection); it != pool.connections.end())
  {
    return it->second.last_duration;
  }

  return std::chrono::milliseconds(0);
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::getLastDuration()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::getUsageCount()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int Database::Pool::getUsageCount(const dbuniq& connection)
{
  Database::Pool::PoolData& pool = this->getPoolData();

  std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

  if (auto it = pool.connections.find(connection); it != pool.connections.end())
  {
    return it->second.usage_count;
  }

  return 0;
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::getUsageCount()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::printConnectionStats()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::printConnectionStats()
{
  std::chrono::milliseconds total_duration{0};
  std::size_t total_uses = 0;
  std::size_t active_connections = 0;
  std::size_t idle_connections = 0;
  std::size_t size = 0;

  {
    Database::Pool::PoolData& pool = this->getPoolData();

    std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

    size = pool.connections.size();

    for (const auto& [conn, metrics] : pool.connections)
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
// End of Database::Pool::printConnectionStats()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------

Database::Database()
{
  this->database = this->chooseDatabase(
  #ifdef CAOS_USE_DB_POSTGRESQL
  DatabaseType::PostgreSQL
  #elif defined(CAOS_USE_DB_MYSQL)
  DatabaseType::MySQL
  #else
  #error "Unknown DatabaseType"
  #endif
  );

  this->pool = std::make_unique<Pool>();

  spdlog::trace("Repository initialized");
}

Database::~Database()
{
  this->pool.reset();
  this->database.reset();
  spdlog::trace("Destroying Repository");
}

std::optional<Database::ConnectionWrapper>  Database::acquire()                         { return this->pool->acquire(); }
void                                        Database::releaseConnection(dboptuniqptr connection) {  this->pool->releaseConnection(connection); }
