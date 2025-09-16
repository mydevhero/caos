/*
 * main.cpp
 *
 *  Created on: 12 lug 2025
 *      Author: mrbi314
 */

#include <libcaos.hpp>
#include <csignal>
#include <atomic>
#include <memory>

std::atomic<bool> running_(true);
std::unique_ptr<caos> mycaos;
const int num_threads = 10;
std::vector<std::thread> threads;

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM)
  {
    std::cout << "Signal " << signal << " intercepted. Shutdown running now\n";
    mycaos.reset();
    running_.store(false,std::memory_order_release);

    // Aspetta che tutti i thread terminino
    for (auto& t : threads) {
        t.join();
    }

  }
}

std::atomic<int> global_i(0);

void worker_thread(int thread_id) {
  while(running_.load(std::memory_order_acquire)) {
    // std::this_thread::sleep_for(std::chrono::milliseconds(2));

    // Ogni thread incrementa il contatore atomicamente
    int current_i = global_i.load(std::memory_order_acquire);

    if(mycaos==nullptr)
    {
      std::cout << "A1\n";
      break;
    }

    try
    {
      auto result = mycaos->repository->echoString(std::to_string(current_i));

      if (result.has_value())
      {
        global_i++;

        if (current_i%10000==0)
        {
          std::cout << current_i << "-" << result.value()<< "\n";
        }
      }
    }
    catch (const repository::broken_connection& e)
    {
      // spdlog::info("Repository unavailable");
    }
    catch (const pqxx::sql_error& e)
    {
      // spdlog::error("SQL error during connection creation: {}", e.what());
    }
    catch (const std::exception& e)
    {
      // spdlog::critical("Exception during connection creation: {}", e.what());
    }
    catch(...)
    {
      // spdlog::error("Can't execute echoString() query");
    }

    // if (current_i>500000)
    // {
    //   std::this_thread::sleep_for(std::chrono::seconds(2));
    // }
    // if (result.has_value())
    // {
    //   std::cout << "Thread " << thread_id << ": " << result.value() << "\n";
    // } else {
    //   std::cout << "Thread " << thread_id << ": Database non disponibile\n";
    // }
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


  // Lancia 10 thread worker
  for (int i = 0; i < num_threads; ++i) {
      threads.emplace_back(worker_thread, i);
  }

  while(running_.load(std::memory_order_acquire))
  {}



  return 0;
}
