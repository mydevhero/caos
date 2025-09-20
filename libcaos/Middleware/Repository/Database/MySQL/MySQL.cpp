#include "MySQL.hpp"

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::setConnectStr()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void Database::Pool::setConnectOpt() noexcept
{
  sql::ConnectOptionsMap options;

  options["hostName"] = this->getHost();
  options["port"]     = this->getPort();
  options["userName"] = this->getUser();
  options["password"] = this->getPass();
  options["schema"]   = this->getName();

  if (this->getConnectTimeout() > 0)
  {
    options["connectTimeout"] = static_cast<int>(this->getConnectTimeout());
  }

  // Salva le options invece della stringa
  this->config.connection_options = options;
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::setConnectStr()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::validateConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
bool Database::Pool::validateConnection(const dbuniq& connection)
{
  static constexpr const char* fName = "MySQL::Pool::validateConnection";

  if (!connection)
  {
    return false;
  }

  try
  {

    if (!connection->isClosed())
    {
      spdlog::trace("[{}] Health check query", fName);

      #if VALIDATE_USING_TRANSACTION == 1
      // Disabilita autocommit per simulare una transazione
      connection->setAutoCommit(false);
      #endif

      // Per MySQL usiamo direttamente lo statement
      std::unique_ptr<sql::Statement> stmt(connection->createStatement());
      std::unique_ptr<sql::ResultSet> result(stmt->executeQuery("SELECT 1"));

      if (result->next())
      {
        int value = result->getInt(1);
        return value == 1;
      }

      #if VALIDATE_USING_TRANSACTION == 1
      connection->commit();
      connection->setAutoCommit(true);
      #endif
    }
  }
  catch (const sql::SQLException& e)
  {
    // Mappatura specifica degli errori MySQL
    if (e.getErrorCode() == 2002 || e.getErrorCode() == 2003 || e.getErrorCode() == 2006 || e.getErrorCode() == 2013)
    {
      // 2002: CR_CONNECTION_ERROR, 2003: CR_CONN_HOST_ERROR,
      // 2006: CR_SERVER_GONE_ERROR, 2013: CR_SERVER_LOST
      throw repository::broken_connection("MySQL connection invalid or server gone away");
    }
    else if (e.getErrorCode() == 1045)
    {
      // ER_ACCESS_DENIED_ERROR
      throw repository::broken_connection("MySQL access denied - invalid credentials");
    }
    else if (e.getErrorCode() == 1049)
    {
      // ER_BAD_DB_ERROR
      throw repository::broken_connection("MySQL database not found");
    }
    else if (e.getErrorCode() == 1044)
    {
      // ER_DBACCESS_DENIED_ERROR
      throw repository::broken_connection("MySQL access to database denied");
    }
    else if (e.getErrorCode() == 1040)
    {
      // ER_CON_COUNT_ERROR
      throw repository::broken_connection("MySQL too many connections");
    }
    else
    {
      // Altri errori SQL
      spdlog::error("[{}] MySQL SQLException: [{}] {}", fName, e.getErrorCode(), e.what());
    }
  }
  catch (const std::exception& e)
  {
    #if VALIDATE_USING_TRANSACTION == 1
    try { connection->rollback(); } catch (...) {}
    try { connection->setAutoCommit(true); } catch (...) {}
    #endif
    throw;
  }
  catch (...)
  {
    #if VALIDATE_USING_TRANSACTION == 1
    try { connection->rollback(); } catch (...) {}
    try { connection->setAutoCommit(true); } catch (...) {}
    #endif
    throw;
  }

  return false;
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::validateConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::createConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
bool Database::Pool::createConnection(std::size_t& pool_size)
{
  static constexpr const char* fName = "Database::Pool::createConnection";

  static std::atomic<bool> loggedOnce {false};

  try
  {
    spdlog::debug("[{}] Creating new connection", fName);

    if(!this->checkPoolSize(pool_size))                                                             // Don't saturate MySQL connections
    {
      return false;
    }

    Database::Pool::PoolData& pool = this->getPoolData();

    // auto connection = std::make_unique<dbconn>(this->getConnectStr());

    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
    if (!driver)
    {
      spdlog::error("MySQL driver not available");
      return false;
    }

    std::unique_ptr<sql::Connection> connection(driver->connect(this->getConnectOpt()));


    if (this->validateConnection(connection))
    {
      ConnectionMetrics metrics;

      pool.connections.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(connection)),
        std::forward_as_tuple(std::move(metrics))
      );

      spdlog::info("[{}] New valid connection created (total: {})",
                  fName, this->getTotalConnections());

      this->connectionRefused = false;

      loggedOnce.store(false, std::memory_order_release);

      return true;
    }

    connection->close();
  }
  catch (const sql::SQLException& e)
  {
    if (e.getErrorCode() == 2003)
    { // Can't connect to MySQL server
      throw repository::broken_connection("Connection invalid");
    }

    if (e.getErrorCode() == 1045)
    { // Access denied
      throw repository::broken_connection("Credential invalid");
    }

    if (e.getErrorCode() == 1049)
    { // Unknown database
      throw repository::broken_connection("Database not found");
    }
  }
  catch (const std::exception& e)
  {
    if (!loggedOnce.load(std::memory_order_acquire))
    {
      spdlog::error("[{}] Exception during connection creation: {}", fName, e.what());
      loggedOnce.store(true, std::memory_order_release);
    }
  }
  catch (...)
  {
    if (!loggedOnce.load(std::memory_order_acquire))
    {
      spdlog::error("[{}] Unknown exception during connection creation", fName);
      loggedOnce.store(true, std::memory_order_release);
    }
  }

  return false;
}
// -------------------------------------------------------------------------------------------------
// End of Database::Pool::createConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------










// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Init of Database::Pool::closeConnection()
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Internal use
void Database::Pool::closeConnection(const dbuniq& connection)
{
  static constexpr const char* fName = "Database::Pool::closeConnection(1)";

  try
  {
    if (!connection)
    {
      spdlog::warn("[{}] Attempted to close null connection", fName);
      return;
    }

    {
      Database::Pool::PoolData& pool = this->getPoolData();

      std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

      if (auto it = pool.connections.find(connection); it != pool.connections.end())
      {
        connection->close();
        spdlog::debug("[{}] Removed metrics for connection (used {} times)", fName, it->second.usage_count);
        it = pool.connections.erase(it);

        return;
      }
    }
  }
  catch (const sql::SQLException& e)
  {
    if (e.getErrorCode() == 2003)
    { // Can't connect to MySQL server
      throw repository::broken_connection("Connection invalid");
    }
    else if (e.getErrorCode() == 1045)
    { // Access denied
      throw repository::broken_connection("Credential invalid");
    }
    else if (e.getErrorCode() == 1049)
    { // Unknown database
      throw repository::broken_connection("Database not found");
    }
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


// Crow's endpoint use
void Database::Pool::closeConnection(std::optional<Database::ConnectionWrapper>& connection)
{
  static constexpr const char* fName = "Database::Pool::closeConnection(2)";

  try
  {
    if (!connection)
    {
      spdlog::warn("[{}] Attempted to close null connection", fName);
      return;
    }

    {
      Database::Pool::PoolData& pool = this->getPoolData();

      std::unique_lock<std::mutex> lock(pool.connections_mutex, std::try_to_lock);

      auto* raw = connection.value().get ();
      auto it = std::find_if (pool.connections.begin(), pool.connections.end(), [raw](auto const& pair) {
        return pair.first.get() == raw;
      });

      if (it != pool.connections.end())
      {
        raw->close();
        spdlog::debug("[{}] Removed metrics for connection (used {} times)", fName, it->second.usage_count);
        it = pool.connections.erase(it);

        return;
      }
    }
  }
  catch (const sql::SQLException& e)
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
// End of Database::Pool::closeConnection()
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
















/***************************************************************************************************
 *
 *
 * MySQL() Constructor/Destructor
 *
 *
 **************************************************************************************************/
MySQL::MySQL(Database* database_):database(database_)
{
  spdlog::trace("MySQL init");

  // this->pool = std::make_unique<Database::Pool>();

  spdlog::trace("MySQL init done");
}





MySQL::~MySQL()
{
  running_.store(false, std::memory_order_release);
  // this->pool.reset();
}
/***************************************************************************************************
 *
 *
 *
 *
 *
 **************************************************************************************************/
