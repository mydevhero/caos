#include "Repository/IRepository.hpp"
#include "Repository/Cache/Cache.hpp"
#include "Repository/Database/Database.hpp"

// namespace middleware
// {
//   struct Repository
//   {
//     struct context
//     {
//       bool skip = false;
//       std::optional<ConnectionWrapper> conn;
//     };

//     PostgreSQLPool* pool;

//     PostgreSQL(PostgreSQLPool* p) : pool(p) {}

//     void before_handle(crow::request& req, crow::response& res, context& ctx)
//     {
//       if (skip_PostgreSQLMiddleware_exact_routes.find(req.url) != skip_PostgreSQLMiddleware_exact_routes.end())
//       {
//         ctx.skip = true;
//         return;
//       }

//       for (const auto& prefix : skip_PostgreSQLMiddleware_prefix_routes)
//         if (req.url.rfind(prefix, 0) == 0)
//         {
//           ctx.skip = true;
//           return;
//         }

//       ctx.conn = pool->acquire(); // Acquires a connection from the pool

//       if (!ctx.conn)
//       {
//         CROW_LOG_ERROR << "Failed to acquire database connection for " << req.url;
//         res.code = 503; // Service Unavailable
//         res.body = R"({"error": "Database connection unavailable"})";
//         res.add_header("Content-Type", "application/json");
//         res.end();
//         return;
//       }

//       CROW_LOG_DEBUG << "Acquired DB connection for " << req.url;
//     }

//     void after_handle(crow::request& req, crow::response& res, context& ctx)
//     {
//       if (!ctx.skip && ctx.conn) {
//         CROW_LOG_DEBUG << "Released DB connection for " << req.url;
//       }
//     }
//   };
// }
