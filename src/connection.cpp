#include "connection.hpp"


Connection::Connection(ServerPtr&& server, boost::asio::ip::tcp::socket&& socket)
    : server_(std::forward<decltype(server)>(server))
    , socket_(std::forward<decltype(socket)>(socket))
{
}

