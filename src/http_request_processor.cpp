#include "http_request_processor.hpp"


HTTPRequestProcessor::HTTPRequestProcessor(
    ServerPtr&& server, boost::asio::ip::tcp::socket&& socket,
    std::string&& uri, std::map<std::string, std::string>&& headers, std::string&& body
)
    : Connection(std::forward<decltype(server)>(server), std::forward<decltype(socket)>(socket))
    , uri_(std::forward<decltype(uri)>(uri))
    , headers_(std::forward<decltype(headers)>(headers))
    , body_(std::forward<decltype(body)>(body))
{
}

