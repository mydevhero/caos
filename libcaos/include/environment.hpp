#pragma once

#include <libcaos/config.hpp>
#include <terminal_options.hpp>

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Environment (vars) configuration manager
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
class Environment
{
  public:
    enum class ENV:std::uint8_t { dev=0, test, production, unmanaged, EOE };                        // EOE = End Of Enum

  private:
    const TerminalOptions* terminalPtr;

    ENV   env;
    std::string name;

    // void setEnv(const TerminalOptions& terminal);
    void setEnv();

    static constexpr std::array<const char*, static_cast<std::size_t>(ENV::EOE)>
      ENVChar = { "dev", "test", "production", "unmanaged" };

    static const std::unordered_map<std::string, ENV> ENVMapToString;
    static constexpr const char* ENV2String(ENV env);
    static inline ENV String2ENV(const std::string& envStr);

  public:
    // Elimina costruttori di copia e assegnazione
    Environment(const Environment&) = delete;
    Environment& operator=(const Environment&) = delete;

    // Metodo per ottenere l'istanza singleton
    static Environment& get_instance()
    {
      static Environment instance;
      return instance;
    }

    Environment();

    [[nodiscard]] ENV         getEnv()  const noexcept;
    [[nodiscard]] std::string getName() const noexcept;
};
/* ---------------------------------------------------------------------------------------------
 *
 * -------------------------------------------------------------------------------------------*/
