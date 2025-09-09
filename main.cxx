#include <vector>
// TODO #include <vector> has to be included before #include <Corrade/Utility/Arguments.h> or #include <Corrade/Utility/Arguments.h> is not building. This problem is in libc++ version 19.1.7-1
#include <Corrade/Utility/Arguments.h>
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/certify/extensions.hpp>
#include <boost/certify/https_verification.hpp>
#include <boost/json/src.hpp>
#include <boost/json/src.hpp> // Only in one translation unit
#include <cstddef>
#include <deque>
#include <exception>
#include <filesystem>
#include <fmt/color.h>
#include <functional>
#include <iostream>
#include <iterator>
#include <login_matchmaking_game_shared/matchmakingGameSerialization.hxx>
#include <login_matchmaking_game_shared/userMatchmakingSerialization.hxx>
#include <matchmaking_proxy/database/database.hxx>
#include <matchmaking_proxy/logic/matchmakingData.hxx>
#include <matchmaking_proxy/logic/matchmakingGame.hxx>
#include <matchmaking_proxy/server/matchmakingOption.hxx>
#include <matchmaking_proxy/server/server.hxx>
#include <matchmaking_proxy/util.hxx>
#include <modern_durak_game_option/src.hxx>
#include <modern_durak_game_option/userDefinedGameOption.hxx>
#include <modern_durak_game_shared/modern_durak_game_shared.hxx>
#include <my_web_socket/coSpawnPrintException.hxx>
#include <my_web_socket/mockServer.hxx>
#include <my_web_socket/myWebSocket.hxx>
#include <openssl/ssl3.h>
#include <sodium/core.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

BOOST_FUSION_DEFINE_STRUCT ((account_with_combinationsSolved), Account, (std::string, accountName) (std::string, password) (size_t, rating) (size_t, combinationsSolved))

auto const DEFAULT_PORT_USER = std::string{ "55555" };
auto const DEFAULT_PORT_MATCHMAKING_TO_GAME = std::string{ "4242" };
auto const DEFAULT_PORT_USER_TO_GAME_VIA_MATCHMAKING = std::string{ "3232" };
auto const DEFAULT_PORT_GAME_TO_MATCHMAKING = std::string{ "12312" };
auto const DEFAULT_PATH_TO_CHAIN_FILE = PATH_TO_SOURCE + std::string{ "/test/cert/localhost.pem" };
auto const DEFAULT_PATH_TO_PRIVATE_FILE = PATH_TO_SOURCE + std::string{ "/test/cert/localhost-key.pem" };
auto const DEFAULT_PATH_TO_DH_FILE = PATH_TO_SOURCE + std::string{ "/test/cert/dhparam.pem" };
auto const DEFAULT_SECRETS_POLLING_SLEEP_TIMER_SECONDS = std::string{ "2" };
auto const DEFAULT_ADDRESS_OF_GAME = std::string{ "127.0.0.1" };
auto const DEFAULT_DATABASE_PATH = PATH_TO_BINARY + std::string{"/matchmaking.db"};

int
main (int argc, char **argv)
{
  Corrade::Utility::Arguments args{};
  using namespace boost::asio;
  using namespace matchmaking_proxy;
  // clang-format off
  args
    .addOption("port-user", DEFAULT_PORT_USER).setHelp("port-user", "port user to game via matchmaking")
    .addOption("port-matchmaking-to-game", DEFAULT_PORT_MATCHMAKING_TO_GAME).setHelp("port-matchmaking-to-game", "port matchmaking to game")
    .addOption("port-user-to-game-via-matchmaking", DEFAULT_PORT_USER_TO_GAME_VIA_MATCHMAKING).setHelp("port-user-to-game-via-matchmaking", "port user to game via matchmaking")
    .addOption("port-game-to-matchmaking", DEFAULT_PORT_GAME_TO_MATCHMAKING).setHelp("port-game-to-matchmaking", "port game to matchmaking")
    .addOption("path-to-chain-file", DEFAULT_PATH_TO_CHAIN_FILE).setHelp("path-to-chain-file", "path to chain file")
    .addOption("path-to-private-file", DEFAULT_PATH_TO_PRIVATE_FILE).setHelp("path-to-private-file", "path to private file")
    .addOption("path-to-dh-file", DEFAULT_PATH_TO_DH_FILE).setHelp("path-to-dh-file", "path to dh file")
    .addOption("secrets-polling-sleep-time-seconds", DEFAULT_SECRETS_POLLING_SLEEP_TIMER_SECONDS).setHelp("secrets-polling-sleep-time-seconds", "secrets polling sleep time seconds")
    .addOption("address-of-game", DEFAULT_ADDRESS_OF_GAME).setHelp("address-of-game", "address of game")
    .addOption("path-to-database", DEFAULT_DATABASE_PATH).setHelp("path-to-database", "full path to database with database file name")
    .addBooleanOption("ssl-context-verify-none").setHelp("ssl-context-verify-none", "disable ssl verification for user to matchmaking usefull for debuging with google chrome")
    .setGlobalHelp("durak matchmaking")
    .parse(argc, argv);
  if (sodium_init () < 0)
    {
      std::cout << "sodium_init <= 0" << std::endl;
      std::terminate ();
      /* panic! the library couldn't be initialized, it is not safe to use */
    }
  auto const databasePath=args.value ("path-to-database");
  database::createDatabaseIfNotExist (databasePath);
  database::createTables (databasePath);
  io_context ioContext{};
  signal_set signals (ioContext, SIGINT, SIGTERM);
  signals.async_wait ([&] (auto, auto) { ioContext.stop (); });
      try
    {
  thread_pool pool{ 2 };
  auto const PORT_USER =  boost::numeric_cast<uint16_t>(std::stoul(args.value ("port-user")));
  auto const PORT_MATCHMAKING_TO_GAME =  args.value ("port-matchmaking-to-game");
  auto const PORT_USER_TO_GAME_VIA_MATCHMAKING =  args.value ("port-user-to-game-via-matchmaking");
  auto const PORT_GAME_TO_MATCHMAKING =  boost::numeric_cast<uint16_t>(std::stoul(args.value ("port-game-to-matchmaking")));
  auto const PATH_TO_CHAIN_FILE =  args.value ("path-to-chain-file");
  auto const PATH_TO_PRIVATE_FILE =  args.value ("path-to-private-file");
  auto const PATH_TO_DH_FILE =  args.value ("path-to-dh-file");
  auto const SECRETS_POLLING_SLEEP_TIMER_SECONDS =  std::stoul(args.value ("secrets-polling-sleep-time-seconds"));
  std::string ADDRESS_GAME = args.value ("address-of-game");
  auto const SSL_CONTEXT_VERIFY_NONE= args.isSet("ssl-context-verify-none");
  using namespace boost::asio::experimental::awaitable_operators;
  auto userMatchmakingEndpoint = boost::asio::ip::tcp::endpoint{ ip::tcp::v4 (), PORT_USER };
  auto gameMatchmakingEndpoint = boost::asio::ip::tcp::endpoint{ ip::tcp::v4 (), PORT_GAME_TO_MATCHMAKING };
  auto server = Server{ ioContext, pool, userMatchmakingEndpoint, gameMatchmakingEndpoint };
  auto matchmakingOption = MatchmakingOption{};
  matchmakingOption.handleCustomMessageFromUser = [] (std::string const &messageType, std::string const &message, MatchmakingData &matchmakingData) {
  boost::system::error_code ec{};
  auto messageAsObject = confu_json::read_json (message, ec);
  if (ec) std::cout << "no handle for custom message: '" << message << "'" << std::endl;
  else if (messageType == "GetCombinationSolved")
    {
      auto combinationSolved = confu_json::to_object<shared_class::GetCombinationSolved> (messageAsObject);
      soci::session sql (soci::sqlite3, matchmakingData.fullPathIncludingDatabaseName.string ().c_str ());
      bool columnExists = false;
      soci::rowset<soci::row> rs = (sql.prepare << "PRAGMA table_info(Account)");
      for (auto const &row : rs)
        {
          std::string name = row.get<std::string> (1);
          if (name == "combinationsSolved")
            {
              columnExists = true;
              break;
            }
        }
      if (!columnExists) sql << "ALTER TABLE Account ADD COLUMN combinationsSolved INTEGER NOT NULL DEFAULT 0";
      if (auto result = confu_soci::findStruct<account_with_combinationsSolved::Account> (sql, "accountName", combinationSolved.accountName))
        {
          matchmakingData.sendMsgToUser (objectToStringWithObjectName (shared_class::CombinationsSolved{ result->accountName, result->combinationsSolved }));
        }
    }
};
  co_spawn (ioContext,
            server.userMatchmaking (PATH_TO_CHAIN_FILE, PATH_TO_PRIVATE_FILE, PATH_TO_DH_FILE, databasePath,std::chrono::seconds{ SECRETS_POLLING_SLEEP_TIMER_SECONDS}, matchmakingOption,ADDRESS_GAME, PORT_MATCHMAKING_TO_GAME, PORT_USER_TO_GAME_VIA_MATCHMAKING)
                || server.gameMatchmaking (databasePath,
                                           [] (std::string const &messageType, std::string const &message, MatchmakingGameData &matchmakingGameData) {
                                             boost::system::error_code ec{};
                                             auto messageAsObject = confu_json::read_json (message, ec);
                                             if (ec) std::cout << "no handle for custom message: '" << message << "'" << std::endl;
                                             else if (messageType == "CombinationSolved")
                                               {
                                                 auto combinationSolved = confu_json::to_object<shared_class::CombinationSolved> (messageAsObject);
                                                 soci::session sql (soci::sqlite3, matchmakingGameData.fullPathIncludingDatabaseName.string ().c_str ());
                                                 bool columnExists = false;
                                                 soci::rowset<soci::row> rs = (sql.prepare << "PRAGMA table_info(Account)");
                                                 for (auto const &row : rs)
                                                   {
                                                     std::string name = row.get<std::string> (1);
                                                     if (name == "combinationsSolved")
                                                       {
                                                         columnExists = true;
                                                         break;
                                                       }
                                                   }
                                                 if (!columnExists) sql << "ALTER TABLE Account ADD COLUMN combinationsSolved INTEGER NOT NULL DEFAULT 0";
                                                 if (auto accountResult = confu_soci::findStruct<account_with_combinationsSolved::Account> (sql, "accountName", combinationSolved.accountName)) confu_soci::updateStruct (sql, account_with_combinationsSolved::Account{ accountResult.value ().accountName, accountResult.value ().password, accountResult.value ().rating, accountResult.value ().combinationsSolved + 1 });
                                               }
                                             else
                                               std::cout << "no handle for custom message: '" << message << "'" << std::endl;
                                           }),
            my_web_socket::printException);
      ioContext.run ();
    }
  catch (std::exception &e)
    {
      std::printf ("Exception: %s\n", e.what ());
    }
}
