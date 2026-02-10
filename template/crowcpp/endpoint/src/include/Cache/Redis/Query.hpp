#pragma once

#include "Middleware/Repository/Cache/Redis/Redis.hpp"

#ifdef QUERY_EXISTS_IQuery_Template_echoString
std::optional<std::string> Redis::IQuery_Template_echoString(std::string str)
{
  static constexpr const char* fName = "Redis::IQuery_Template_echoString";

  std::optional<std::string> db_result;

  try {
    // 1. Generate la cache key
    std::string cache_key = "echo:" + str;

    // 2. Try getting from Redis
    auto cached_value = this->redis->get(cache_key);
    if (cached_value) {
      spdlog::debug("[{}] Cache hit for key: {}", fName, cache_key);
      std::string cache_result = *cached_value;
      return cache_result + " by Redis";
    }

    spdlog::debug("[{}] Cache miss for key: {}", fName, cache_key);

    if (this->database == nullptr)
    {
      throw std::runtime_error("Database has null object");
    }

    // 3. Cache miss - query database
    db_result = this->database->IQuery_Template_echoString(str);

    // 4. Database return a value, store in cache
    if (db_result.has_value())
    {
      try
      {
        // Store in Redis con TTL (es. 5 minutes)
        this->redis->setex(cache_key,
                    std::chrono::seconds(300),
                    db_result.value());

        spdlog::debug("[{}] Stored in cache with key: {}", fName, cache_key);
      }
      catch (const sw::redis::Error& e)
      {
        // Log dell'errore ma non fallire - la query può comunque restituire il risultato
        spdlog::warn("[{}] Failed to store in cache: {}", fName, e.what());
      }
    }

    return db_result;
  }
  catch (const sw::redis::Error& e)
  {
    // Errore Redis - log e fallback al database
    spdlog::error("[{}] Redis error: {}", fName, e.what());

    // Fallback diretto al database
    if (db_result.has_value())
    {
      return db_result;
    }

    return this->database->IQuery_Template_echoString(str);
  }
  catch (const std::exception& e)
  {
    spdlog::error("[{}] Exception: {}", fName, e.what());
    throw;
  }
  catch(...)
  {
    throw;
  }
};
#endif // End of ifdef QUERY_EXISTS_IQuery_Template_echoString

#ifdef QUERY_EXISTS_IQuery_Template_echoString_custom
std::optional<std::string> Redis::IQuery_Template_echoString_custom(std::string str)
{
  static constexpr const char* fName = "Redis::IQuery_Template_echoString_custom";

  std::optional<std::string> db_result;

  try {
    // 1. Generate la cache key
    std::string cache_key = "echo:" + str;

    // 2. Try getting from Redis
    auto cached_value = this->redis->get(cache_key);
    if (cached_value) {
      spdlog::debug("[{}] Cache hit for key: {}", fName, cache_key);
      std::string cache_result = *cached_value;
      return cache_result + " by Redis";
    }

    spdlog::debug("[{}] Cache miss for key: {}", fName, cache_key);

    if (this->database == nullptr)
    {
      throw std::runtime_error("Database has null object");
    }

    // 3. Cache miss - query database
    db_result = this->database->IQuery_Template_echoString_custom(str);

    // 4. Database return a value, store in cache
    if (db_result.has_value())
    {
      try
      {
        // Store in Redis con TTL (es. 5 minutes)
        this->redis->setex(cache_key,
                    std::chrono::seconds(300),
                    db_result.value());

        spdlog::debug("[{}] Stored in cache with key: {}", fName, cache_key);
      }
      catch (const sw::redis::Error& e)
      {
        // Log dell'errore ma non fallire - la query può comunque restituire il risultato
        spdlog::warn("[{}] Failed to store in cache: {}", fName, e.what());
      }
    }

    return db_result;
  }
  catch (const sw::redis::Error& e)
  {
    // Errore Redis - log e fallback al database
    spdlog::error("[{}] Redis error: {}", fName, e.what());

    // Fallback diretto al database
    if (db_result.has_value())
    {
      return db_result;
    }

    return this->database->IQuery_Template_echoString_custom(str);
  }
  catch (const std::exception& e)
  {
    spdlog::error("[{}] Exception: {}", fName, e.what());
    throw;
  }
  catch(...)
  {
    throw;
  }
};
#endif // End of ifdef QUERY_EXISTS_IQuery_Template_echoString_custom
