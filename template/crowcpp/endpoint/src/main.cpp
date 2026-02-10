#include <Repository.hpp>
#include <memory>

int main(int argc, char* argv[])
{

  auto caos = std::make_unique<Caos>(argc, argv, initFlags::Repository|initFlags::CrowCpp);

  crow::App<> app;

#ifdef QUERY_EXISTS_IQuery_Template_echoString
  CROW_ROUTE(app, "/echoString/<string>")([&caos](crow::response& res, std::string str)
  {
    try
    {
      auto ret = caos->repository->IQuery_Template_echoString(str);

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
  CROW_ROUTE(app, "/echoString_custom/<string>")([&caos](crow::response& res, std::string str)
  {
    try
    {
      auto ret = caos->repository->IQuery_Template_echoString_custom(str);

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

  app
  .bindaddr(caos->crowcpp->getHost())
  .port(caos->crowcpp->getPort())
  .concurrency(caos->crowcpp->getThreadCount())
  /*.multithreaded()*/
  .run();

  return 0;
}
