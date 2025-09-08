#include <terminal_options.hpp>

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Init of implementation of struct TerminalOptions
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
TerminalOptions::TerminalOptions(int argc, char** argv) : argc(argc), argv(argv)
{
  options = std::make_unique<cxxopts::Options>(argv[0], CAOS_OFFICIALNAME);

  options->add_options()
    ("help"                                     , "Show help"                                               )
    ("version"                                  , "Get build version&timestamp"                             )

    // Environment
    (CAOS_OPT_APP_ENV_NAME                      , "Environment"                   , cxxopts::value<std::string>()   )

    // Log
    (CAOS_OPT_LOG_SEVERITY_NAME                 , "Log Terminal Severity"         , cxxopts::value<std::string>()   )

    // Server
    (CAOS_OPT_BINDTOADDRESS_NAME                , "Bind to address"               , cxxopts::value<std::string>()   )
    (CAOS_OPT_BINDTOPORT_NAME                   , "Bind to port"                  , cxxopts::value<std::uint16_t>() )
    (CAOS_OPT_THREADS_NAME                      , "Number of threads"             , cxxopts::value<std::uint16_t>() )

    // Database
    (CAOS_OPT_DBUSER_NAME                       , "Database User"                 , cxxopts::value<std::string>()   )
    (CAOS_OPT_DBPASS_NAME                       , "Database Password"             , cxxopts::value<std::string>()   )
    (CAOS_OPT_DBHOST_NAME                       , "Database Host"                 , cxxopts::value<std::string>()   )
    (CAOS_OPT_DBPORT_NAME                       , "Database Port"                 , cxxopts::value<std::uint16_t>() )
    (CAOS_OPT_DBNAME_NAME                       , "Database Name"                 , cxxopts::value<std::string>()   )
    (CAOS_OPT_DBPOOLSIZEMIN_NAME                , "Database Pool Size Min Idle"   , cxxopts::value<std::size_t>()   )
    (CAOS_OPT_DBPOOLSIZEMAX_NAME                , "Database Pool Size Max Idle"   , cxxopts::value<std::size_t>()   )
    (CAOS_OPT_DBPOOLWAIT_NAME                   , "Database Pool Wait"            , cxxopts::value<std::uint32_t>() )
    (CAOS_OPT_DBKEEPALIVES_NAME                 , "Database Keepalives"           , cxxopts::value<std::size_t>()   )
    (CAOS_OPT_DBKEEPALIVES_IDLE_NAME            , "Database Keepalives Idle"      , cxxopts::value<std::size_t>()   )
    (CAOS_OPT_DBKEEPALIVES_INTERVAL_NAME        , "Database Keepalives Interval"  , cxxopts::value<std::size_t>()   )
    (CAOS_OPT_DBKEEPALIVES_COUNT_NAME           , "Database Keepalives Count"     , cxxopts::value<std::size_t>()   )
    (CAOS_OPT_DBMAXWAIT_NAME                    , "Database Max Wait"             , cxxopts::value<std::uint32_t>()   )
    (CAOS_OPT_DBHEALTHCHECKINTERVAL_NAME        , "Database Healtch Check interval", cxxopts::value<std::uint32_t>()   )
  ;

  this->parse();

  this->read();

  SPDLOG_DEBUG("Terminal options parsed");
}



std::string TerminalOptions::help() const noexcept
{
  return options->help();
}


void TerminalOptions::parse()
{
  static constexpr const char* fName = "[caos::TerminalOptions::parse]";

  try
  {
    result = std::make_unique<cxxopts::ParseResult>(options->parse(this->argc, this->argv));
  }
  catch (const cxxopts::exceptions::exception& e)
  {
    throw std::runtime_error("Parsing error: " + std::string(e.what()));
  }
  catch (const std::exception& e)
  {
    std::cerr << fName    << "\n"
              << e.what() << "\n";

    std::exit(1);
  }
}



bool TerminalOptions::has(const std::string& option) const noexcept
{
  return (result && result->count(option) > 0);
}



void TerminalOptions::read()                          const noexcept
{
  if (this->has("help"))
  {
    std::cout << this->help() << "\n";

    #ifndef caos_ENABLE_TESTS
    std::exit(0);
    #endif
  }
  else if (this->has("version"))
  {
    std::cout << CAOS_BUILD_VERSION_WITH_TIMESTAMP << "\n";

    #ifndef caos_ENABLE_TESTS
    std::exit(0);
    #endif
  }
}
/* -------------------------------------------------------------------------------------------------
 * End of implementation of struct TerminalOptions
 * -----------------------------------------------------------------------------------------------*/
