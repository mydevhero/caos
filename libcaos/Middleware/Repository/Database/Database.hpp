// Database.hpp
#pragma once

#include <libcaos/config.hpp>
#include "../IRepository.hpp"
#include "../Exception.hpp"
#include "DatabaseFwd.hpp"

#ifdef CAOS_USE_DB_POSTGRESQL
#include "PostgreSQL/PostgreSQL.hpp"
#endif

#ifdef CAOS_USE_DB_MYSQL
#include "MySQL/MySQL.hpp"
#endif


