/*
 * main.cpp
 *
 *  Created on: 12 lug 2025
 *      Author: mrbi314
 */

#include <libcaos.hpp>
#include <Exception.hpp>

#include <csignal>
#include <atomic>
#include <memory>
#include "crow.h"

std::atomic<bool> running_(true);
static std::unique_ptr<caos> mycaos;

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM)
  {
    std::cout << "Signal " << signal << " intercepted. Shutdown running now\n";
    mycaos.reset();
    running_.store(false,std::memory_order_release);

  }
}

int main(int argc, char* argv[])
{
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  spdlog::set_level(CAOS_SEVERITY_LEVEL_BEFORE_LOG_START);

  SPDLOG_TRACE("Ready to instance caos");

  // Crea l'istanza
  mycaos = std::make_unique<caos>(argc, argv);

  spdlog::info("Avvio applicazione");

  crow::App<> app;

  CROW_ROUTE(app, "/<string>")([](crow::response& res, std::string str){

    spdlog::trace("URL: /<string>");

    try
    {
      auto ret = mycaos->repository->echoString(str);

      spdlog::trace("AAA");

      if (ret.has_value())
      {
        spdlog::trace("BBB");
        // return  ret.value(); // std::string
        res.set_header("Content-Type", "text/html");
        res.body = ret.value();
        res.code = 200;
        // res.end();
      }
      else
      {
        spdlog::trace("EEE");
        // return  ret.value(); // std::string
        res.set_header("Content-Type", "text/html");
        res.body = "OUUUU";
        res.code = 200;
        // res.end();
      }
    }
    catch (const repository::broken_connection& e)
    {
      // std::cout << "Repository unavailable\n" << e.what() << "\n";
      res.set_header("Content-Type", "text/plain");
      res.body = "Repository unavailable";
      res.code = 503;
      // res.end();
    }
    catch (const pqxx::sql_error& e)
    {
      // std::cout << "Repository error\n";
      res.set_header("Content-Type", "text/plain");
      res.body = "Repository error";
      res.code = 503;
    }
    catch (const std::exception& e)
    {
      // std::cout << "Something went wrong\n";
      res.set_header("Content-Type", "text/plain");
      res.body = "Something went wrong";
      res.code = 503;
    }
    catch(...)
    {
      // spdlog::error("Unknown exception");
      res.set_header("Content-Type", "text/plain");
      res.body = "Unknown exception";
      res.code = 503;
    }

    res.end();
  });

  app.port(18080)./*multithreaded().*/run();

  return 0;
}
