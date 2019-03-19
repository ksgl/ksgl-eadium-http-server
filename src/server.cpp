#include "http_request_reader.hpp"
#include "server.hpp"

#include <boost/filesystem.hpp>

#include <iostream>


Server::Server(boost::asio::io_context& context, const std::uint16_t& port, const boost::filesystem::path& doc_root)
    : doc_root_(doc_root)
    , endpoint_(boost::asio::ip::tcp::v4(), port)
    , acceptor_(context)
{
}

Server::~Server() {
    stop();
}

void Server::start() {
    if (!running_) {
        running_ = true;
        acceptor_.open(endpoint_.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint_);
        acceptor_.listen();
        accept_next_connection();
    }
}

void Server::stop() noexcept {
    if (running_) {
        running_ = false;
        boost::system::error_code ec;
        acceptor_.cancel(ec);
        acceptor_.close(ec);
    }
}

bool Server::is_running() const {
    return running_;
}

const std::string Server::get_server_string() const {
    return "ksgl-eadium-highload/0.0.1";
}

const std::string Server::get_index_file_name() const {
    return "index.html";
}

const boost::filesystem::path& Server::get_doc_root() const {
    return doc_root_;
}

void Server::accept_next_connection() {
    acceptor_.async_accept(
        [self_wptr = weak_from_this()] (boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (ec == boost::asio::error::operation_aborted) {
                return;
            }
            else if (ec) {
                std::clog << "Failed to accept a connection: " << ec.message() << std::endl;
                auto self_sptr = self_wptr.lock();
                if (self_sptr) {
                    self_sptr->accept_next_connection();
                }
            }
            else {
                auto self_sptr = self_wptr.lock();
                if (self_sptr && self_sptr->running_) {
                    self_sptr->accept_next_connection();
                    std::make_shared<HTTPRequestReader>(std::move(self_sptr), std::move(socket))->start();
                }
                else {
                    std::clog << "Failed to accept a connection: server is not ready" << std::endl;
                }
            }
        }
    );
}

