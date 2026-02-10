#include <Repository.hpp>
#include <memory>

int main(int argc, char* argv[])
{
  middleware::Repository middleware{argc, argv};
  crow::App<middleware::Repository> app{middleware};

#ifdef QUERY_EXISTS_IQuery_Template_echoString
  CROW_ROUTE(app, "/echoString/<string>")([&app](crow::request& req, crow::response& res, std::string str)
  {
    try
    {
      auto& caos = app.get_context<middleware::Repository>(req);

      auto ret = caos.repository->IQuery_Template_echoString(str);

      if (ret.has_value())
      {
        res.set_header("Content-Type", "text/html");
        res.body = ret.value();
        res.code = 200;
      }
      else
      {
        res.set_header("Content-Type", "text/html");
        res.body = "No value";
        res.code = 200;
      }
    }
    catch (const repository::broken_connection& e)
    {
      res.set_header("Content-Type", "text/plain");
      res.body = "Repository unavailable";
      res.code = 503;
    }
    catch (const std::exception& e)
    {
      res.set_header("Content-Type", "text/plain");
      res.body = "Something went wrong";
      res.code = 503;
    }
    catch(...)
    {
      res.set_header("Content-Type", "text/plain");
      res.body = "Unknown exception";
      res.code = 503;
    }

    res.end();
  });
#endif

#ifdef QUERY_EXISTS_IQuery_Template_echoString_custom
  CROW_ROUTE(app, "/echoString_custom/<string>")([&app](crow::request& req, crow::response& res, std::string str)
  {
    try
    {
      auto& caos = app.get_context<middleware::Repository>(req);

      auto ret = caos.repository->IQuery_Template_echoString_custom(str);

      if (ret.has_value())
      {
        res.set_header("Content-Type", "text/html");
        res.body = ret.value();
        res.code = 200;
      }
      else
      {
        res.set_header("Content-Type", "text/html");
        res.body = "No value";
        res.code = 200;
      }
    }
    catch (const repository::broken_connection& e)
    {
      res.set_header("Content-Type", "text/plain");
      res.body = "Repository unavailable";
      res.code = 503;
    }
    catch (const std::exception& e)
    {
      res.set_header("Content-Type", "text/plain");
      res.body = "Something went wrong";
      res.code = 503;
    }
    catch(...)
    {
      res.set_header("Content-Type", "text/plain");
      res.body = "Unknown exception";
      res.code = 503;
    }

    res.end();
  });
#endif

  auto& caos_middleware_instance = app.get_middleware<middleware::Repository>();

  app
  .bindaddr(caos_middleware_instance.caos->crowcpp->getHost())
  .port(caos_middleware_instance.caos->crowcpp->getPort())
  .concurrency(caos_middleware_instance.caos->crowcpp->getThreadCount())
  /*.multithreaded()*/
  .run();

  return 0;
}
