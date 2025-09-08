#pragma once

#include <libcaos/config.hpp>
#include <cxxopts.hpp>

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Terminal option management, add and parse options following caos command on terminal
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
class TerminalOptions {
  public:
    // Elimina costruttori di copia e assegnazione
    TerminalOptions(const TerminalOptions&) = delete;
    TerminalOptions& operator=(const TerminalOptions&) = delete;

    // Metodo per ottenere l'istanza singleton
    static TerminalOptions& get_instance(int argc = 0, char** argv = nullptr)
    {
      static TerminalOptions instance(argc, argv);
      return instance;
    }

    // Metodi originali identici ai tuoi
    ~TerminalOptions() = default;
    void parse();
    [[nodiscard]] bool has(const std::string& option) const noexcept;


    template <typename T>
    T get(const std::string& option) const
    {
      if (!result)
      {
        throw std::runtime_error("Options not parsed yet");
      }
      return (*result)[option].as<T>();
    }

  private:
    TerminalOptions(int argc, char** argv);
    [[nodiscard]] std::string help() const noexcept;

    TerminalOptions& add_option(const std::string& name, const std::string& description);

    template <typename ValueType>
    TerminalOptions& add_option(const std::string& name, const std::string& description, ValueType&& value)
    {
      options->add_options()(name, description, std::forward<ValueType>(value));
      return *this;
    }

    void read()                          const noexcept;

    int argc;
    char** argv;
    std::unique_ptr<cxxopts::Options> options;
    std::unique_ptr<cxxopts::ParseResult> result;
};
/* -------------------------------------------------------------------------------------------------
 *
 * -----------------------------------------------------------------------------------------------*/
