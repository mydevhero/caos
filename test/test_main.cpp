/*
 * test_main.cpp
 *
 *  Created on: 26 lug 2025
 *      Author: mrbi314
 */

#define CATCH_CONFIG_ENABLE_SOURCELINE_INFO
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_FAST_COMPILE

#include <iostream>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <libxapp.hpp>





// class xapp_tester // Friend class of xapp
// {
//   public:
//     using ServerConfig = xapp::ServerConfig;
//     using TerminalOptions = xapp::TerminalOptions;

//     xapp xapp;

//     // xapp_tester(){}
//     ~xapp_tester() = default;
// };





// -------------------------------------------------------------------------------------------------
// Fixture (Setup/TearDown comune)
// -------------------------------------------------------------------------------------------------
struct TestFixture
{
  // private:

  public:
    TestFixture()
    {
        std::cout << "=== Setup fixture ===\n";
    }

    ~TestFixture()
    {
        std::cout << "=== Teardown fixture ===\n";
    }

    // xapp_tester xapp_tester;

    std::vector<std::string> valid_ipv4s =
    {
      "127.0.0.1",
      "192.168.1.1",
      "10.0.0.1"
    };

    std::vector<std::string> valid_ipv6s =
    {
      "::ffff:7f00:1",    // 127.0.0.1
      "::ffff:c0a8:101",  // 192.168.1.1
      "::ffff:a00:1",     // 10.0.0.1
      "::1"               // IPv6
    };

    std::vector<std::string> invalid_ips =
    {
      "256.0.0.1",
      "not.an.ip",
      "192.168.1.",
      "1234:5678:::9"     // IPv6 malformed
    };
};





// -------------------------------------------------------------------------------------------------
// Test Cases
// -------------------------------------------------------------------------------------------------

TEST_CASE("Server Configuration Validation", "[config][network]")
{
  // TestFixture fix;
  // auto& server = fix.xapp_tester.xapp.server;

  // SECTION("Valid IPv4 addresses")
  // {
  //   for (const auto& ipaddr : fix.valid_ipv4s)
  //   {
  //     INFO("Testing IP: " << ipaddr);
  //     REQUIRE(server.testValidateIPAddress (ipaddr));

  //     INFO("Testing protocol");
  //     REQUIRE(server.getIPVersion () == IPVersion::IPv4);
  //   }
  // }

  // SECTION("Valid IPv6 addresses")
  // {
  //   for (const auto& ipaddr : fix.valid_ipv6s)
  //   {
  //     INFO("Testing IP: " << ipaddr);
  //     REQUIRE(server.testValidateIPAddress(ipaddr));

  //     INFO("Testing protocol");
  //     REQUIRE(server.getIPVersion () == IPVersion::IPv6);
  //   }
  // }

  // SECTION("Invalid IP addresses")
  // {
  //   for (const auto& ipaddr : fix.invalid_ips)
  //   {
  //     INFO("Testing IP: " << ipaddr);
  //     REQUIRE_FALSE(server.testValidateIPAddress(ipaddr));
  //   }
  // }

  // SECTION("Port validation")
  // {
  //   std::uint16_t port = 0;
  //   REQUIRE_NOTHROW(server.testValidateTCPPort("8080",port));
  //   REQUIRE(port == 8080);

  //   REQUIRE_NOTHROW(server.testValidateTCPPort("18080",port));
  //   REQUIRE(port == 18080);

  //   REQUIRE_THROWS(server.testValidateTCPPort("1023",port));    // Port out-of-range
  //   REQUIRE_THROWS(server.testValidateTCPPort("99999",port));   // Port out-of-range
  // }
}




/* prima funzionava
TEST_CASE("TerminalOptions implementation", "[terminal][options]")
{
  std::string program_name      = "test_prog";
  std::string ip                = "127.0.0.1";
  std::string port              = "18080";
  std::string threads           = "3";
  std::string opt_bindtoaddress = OPT_BINDTOADDRESS_NAME;
  std::string opt_bindtoport    = OPT_BINDTOPORT_NAME;
  std::string opt_threads       = OPT_THREADS_NAME;

  xapp::TerminalOptions opts(program_name, "Test program");

  SECTION("Initial options are present")
  {
    std::vector<std::string> args = {
      program_name,
      "--help",
      "--version"
    };

    // Converti in array di char*
    std::vector<char*> argv;
    for (auto& arg : args)
      argv.push_back(const_cast<char*>(arg.data()));

    opts.parse(argv.size(), argv.data());

    REQUIRE(opts.has("help") == true);
    REQUIRE(opts.has("version") == true);
  }

  SECTION("Help message generation")
  {
    REQUIRE_FALSE(opts.help().empty());
    REQUIRE(opts.help().find("Test program") != std::string::npos);
  }

  SECTION("Network options")
  {
    std::vector<std::string> args = {
      program_name,
      "--" + opt_bindtoaddress + "=" + ip,
      "--" + opt_bindtoport + "=" + port,
      "--" + opt_threads + "=" + threads
    };

    // Converti in array di char*
    std::vector<char*> argv;
    for (auto& arg : args)
      argv.push_back(const_cast<char*>(arg.data()));

    opts.parse(argv.size(), argv.data());

    REQUIRE(opts.has(OPT_BINDTOADDRESS_NAME) == true);
    REQUIRE(opts.has(OPT_BINDTOPORT_NAME) == true);
    REQUIRE(opts.has(OPT_THREADS_NAME) == true);

    // Verifica i valori
    REQUIRE(opts.get<std::string>(OPT_BINDTOADDRESS_NAME) == ip);
    REQUIRE(opts.get<std::string>(OPT_BINDTOPORT_NAME) == port);
    REQUIRE(opts.get<std::string>(OPT_THREADS_NAME) == threads);
  }
}

TEST_CASE("Environment variables", "[network][env]") {
  std::string program_name = "test_prog";
  std::string ip = "127.0.0.1";
  std::string port = "18080";
  std::string threads = "3";


  SECTION("Set network options from environment") {
    REQUIRE(setenv(ENV_BINDTOADDRESS_NAME, ip.c_str(), 1) == 0);
    REQUIRE(setenv(ENV_BINDTOPORT_NAME, port.c_str(), 1) == 0);
    REQUIRE(setenv(ENV_THREADS_NAME, threads.c_str(), 1) == 0);

    // Ora verifica che siano state impostate correttamente
    const char* env_addr = getenv(ENV_BINDTOADDRESS_NAME);
    const char* env_port = getenv(ENV_BINDTOPORT_NAME);
    const char* env_threads = getenv(ENV_THREADS_NAME);

    // Verifica che esistano
    REQUIRE(env_addr != nullptr);
    REQUIRE(env_port != nullptr);
    REQUIRE(env_threads != nullptr);

    // Verifica i valori
    REQUIRE(std::string(env_addr) == ip);
    REQUIRE(std::string(env_port) == port);
    REQUIRE(std::string(env_threads) == threads);
  }

  unsetenv(ENV_BINDTOADDRESS_NAME);
  unsetenv(ENV_BINDTOPORT_NAME);
  unsetenv(ENV_THREADS_NAME);

}
*/

// TEST_CASE("ServerConfig validation", "[server][config]") {
//     // Setup per test con ambiente pulito
//     unsetenv(ENV_BINDTOADDRESS);
//     unsetenv(ENV_BINDTOPORT);
//     unsetenv(ENV_THREADS);

//     xapp::TerminalOptions dummyOpts("dummy", "dummy");
//     xapp::ServerConfig cfg("127.0.0.1", "8080", "4", dummyOpts);

//     SECTION("IP address validation") {
//         REQUIRE(cfg.testValidateIPAddress("127.0.0.1") == true);
//         REQUIRE(cfg.testValidateIPAddress("::1") == true); // IPv6
//         REQUIRE(cfg.testValidateIPAddress("invalid.ip") == false);

//         SECTION("Sets correct IP version") {
//             cfg.testValidateIPAddress("192.168.1.1");
//             REQUIRE(cfg.getIPVersion() == IPVersion::IPv4);

//             cfg.testValidateIPAddress("2001:db8::1");
//             REQUIRE(cfg.getIPVersion() == IPVersion::IPv6);
//         }
//     }

//     SECTION("TCP port validation") {
//         std::uint16_t port;
//         REQUIRE(cfg.testValidateTCPPort("8080", port) == true);
//         REQUIRE(port == 8080);

//         REQUIRE_THROWS_AS(cfg.testValidateTCPPort("1023", port),
//                           UserFriendlyException); // Porta troppo bassa
//         REQUIRE_THROWS_AS(cfg.testValidateTCPPort("70000", port),
//                           UserFriendlyException); // Porta troppo alta
//         REQUIRE_THROWS_AS(cfg.testValidateTCPPort("abc", port),
//                           UserFriendlyException); // Porta non numerica
//     }

//     SECTION("Thread count validation") {
//         std::uint16_t workers;
//         REQUIRE(cfg.testValidateThreadCount("4", workers) == true);
//         REQUIRE(workers == 4);

//         REQUIRE_THROWS_AS(cfg.testValidateThreadCount("0", workers),
//                           UserFriendlyException);
//         REQUIRE_THROWS_AS(cfg.testValidateThreadCount("999", workers),
//                           UserFriendlyException); // Dipende da hardware_concurrency
//         REQUIRE_THROWS_AS(cfg.testValidateThreadCount("abc", workers),
//                           UserFriendlyException);
//     }
// }

// TEST_CASE("ServerConfig initialization priority", "[server][config][priority]") {
//     // Test della priorità: command line > env vars > defaults

//     SECTION("Command line priority") {
//         xapp::TerminalOptions opts("test", "test");
//         const char* argv[] = {"test",
//                              "--" OPT_BINDTOADDRESS, "192.168.1.1",
//                              "--" OPT_BINDTOPORT, "9090",
//                              "--" OPT_THREADS, "8"};
//         opts.parse(7, const_cast<char**>(argv));

//         xapp::ServerConfig cfg("127.0.0.1", "8080", "4", opts);
//         REQUIRE(cfg.getAddress() == "192.168.1.1");
//         REQUIRE(cfg.getPort() == 9090);
//         REQUIRE(cfg.getWorkerCount() == 8);
//     }

//     SECTION("Environment variables priority") {
//         setenv(ENV_BINDTOADDRESS, "10.0.0.1", 1);
//         setenv(ENV_BINDTOPORT, "7070", 1);
//         setenv(ENV_THREADS, "6", 1);

//         xapp::TerminalOptions opts("test", "test");
//         xapp::ServerConfig cfg("127.0.0.1", "8080", "4", opts);

//         REQUIRE(cfg.getAddress() == "10.0.0.1");
//         REQUIRE(cfg.getPort() == 7070);
//         REQUIRE(cfg.getWorkerCount() == 6);

//         // Cleanup
//         unsetenv(ENV_BINDTOADDRESS);
//         unsetenv(ENV_BINDTOPORT);
//         unsetenv(ENV_THREADS);
//     }

//     SECTION("Default values fallback") {
//         xapp::TerminalOptions opts("test", "test");
//         xapp::ServerConfig cfg("127.0.0.1", "8080", "4", opts);

//         REQUIRE(cfg.getAddress() == "127.0.0.1");
//         REQUIRE(cfg.getPort() == 8080);
//         REQUIRE(cfg.getWorkerCount() == 4);
//     }
// }

// TEST_CASE("xapp integration tests", "[xapp][integration]") {
//     SECTION("Normal initialization") {
//         const char* argv[] = {"xapp",
//                              "--" OPT_BINDTOADDRESS, "127.0.0.1",
//                              "--" OPT_BINDTOPORT, "8080",
//                              "--" OPT_THREADS, "4"};

//         REQUIRE_NOTHROW(xapp app(7, const_cast<char**>(argv)));

//         xapp app(7, const_cast<char**>(argv));
//         REQUIRE(app.server.getAddress() == "127.0.0.1");
//         REQUIRE(app.server.getPort() == 8080);
//         REQUIRE(app.server.getWorkerCount() == 4);
//     }

//     SECTION("Help and version flags") {
//         const char* help_argv[] = {"xapp", "--help"};
//         REQUIRE_NOTHROW(xapp app(2, const_cast<char**>(help_argv)));

//         const char* version_argv[] = {"xapp", "--version"};
//         REQUIRE_NOTHROW(xapp app(2, const_cast<char**>(version_argv)));
//     }

//     SECTION("Invalid arguments") {
//         const char* invalid_argv[] = {"xapp", "--invalid-option"};
//         REQUIRE_THROWS_AS(xapp app(2, const_cast<char**>(invalid_argv)),
//                           std::runtime_error);
//     }
// }

// TEST_CASE("xapp printing methods", "[xapp][output]") {
//     xapp app; // Usa il costruttore di test

//     SECTION("Logo printing") {
//         REQUIRE_NOTHROW(app.PRINT_LOGO());
//     }

//     SECTION("Build info printing") {
//         REQUIRE_NOTHROW(app.PRINT_BUILD_COUNT());
//         REQUIRE_NOTHROW(app.PRINT_BUILD_TIMESTAMP());
//         REQUIRE_NOTHROW(app.PRINT_BUILD_WITH_TIMESTAMP());
//     }

//     SECTION("Header printing") {
//         REQUIRE_NOTHROW(app.PRINT_HEADER());
//     }
// }
