#include <environment.hpp>

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Inizio implementazione Environment
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
const std::unordered_map<std::string, Environment::ENV> Environment::ENVMapToString =
{
  {"dev", ENV::dev},
  {"test", ENV::test},
  {"production", ENV::production},
  {"unmanaged", ENV::unmanaged}
};



constexpr const char* Environment::ENV2String(ENV env)
{
  if (env < ENV::EOE)
  {
    return ENVChar.at(static_cast<size_t>(env));
  }

  throw std::out_of_range("ENV is unnknown");
}



inline Environment::ENV Environment::String2ENV(const std::string& envStr)
{
  auto it = ENVMapToString.find(envStr);

  if (it != ENVMapToString.end())
  {
    return it->second;
  }

  throw std::out_of_range("Invalid ENV string: " + envStr);
}



Environment::Environment()
  : terminalPtr(&TerminalOptions::get_instance()), env(ENV::unmanaged), name(ENV2String(env))
{
  SPDLOG_DEBUG("Setting APP environment");

  this->setEnv();                                                                                   // Validazione a assegnazione environment
}



Environment::ENV Environment::getEnv()   const noexcept                                             // Getter Environment
{
  return this->env;
}



std::string Environment::getName()  const noexcept                                                  // Getter Environment Name
{
  return this->name;
}



void Environment::setEnv()                                                                    // Setter Environment & Environment Name
{
  static constexpr const char* fName = "[Environment::setEnv]";
  std::string techMsg;

  try
  {
    if (terminalPtr->has(CAOS_OPT_APP_ENV_NAME))
    {
      this->name = terminalPtr->get<std::string>(CAOS_OPT_APP_ENV_NAME);
    }
    else if (const char* env_name = std::getenv(CAOS_ENV_APP_ENV_NAME))
    {
      this->name = env_name;
    }
    else
    {
      this->name = CAOS_DEFAULT_APP_ENV_VALUE;
    }

    techMsg = R"(Allowed environment is "dev", "test", "production", "unmanaged")";

    if (this->name.empty())
    {
      throw std::invalid_argument("APP_ENV_NAME empty!");
    }

    SPDLOG_INFO("APP environment is \"{}\"",this->name);

    this->env = String2ENV(this->name);                                                             // Valida Environment
  }
  catch(const std::invalid_argument& e)
  {
    std::cerr << fName    << "\n"
              << e.what() << "\n"
              << techMsg  << "\n";

    std::exit(1);
  }
  catch(const std::out_of_range& e)
  {
    std::cerr << fName    << "\n"
              << e.what() << "\n"
              << techMsg  << "\n";

    std::exit(1);
  }
  catch (const std::exception& e)
  {
    std::cerr << fName    << "\n"
              << e.what() << "\n";

    std::exit(1);
  }
}
/* -------------------------------------------------------------------------------------------------
 * End of Environment
 * -----------------------------------------------------------------------------------------------*/
