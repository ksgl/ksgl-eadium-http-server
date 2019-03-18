#pragma once

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include <memory>


class Server;
using ServerPtr = std::shared_ptr<Server>;

class Server final
    : public std::enable_shared_from_this<Server>
{
public:
    explicit Server(boost::asio::io_context& context, const std::uint16_t& port, const boost::filesystem::path& doc_root);
    ~Server();

    void start();
    void stop() noexcept;

    const std::string get_server_string() const;
    const std::string get_index_file_name() const;
    const boost::filesystem::path& get_doc_root() const;
    bool is_running() const;

private:
    void accept_next_connection();

private:
    std::atomic<bool> running_{false};
    const boost::filesystem::path doc_root_;
    const boost::asio::ip::tcp::endpoint endpoint_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

