#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>


class IOContextThreadPoolRunner {
public:
    IOContextThreadPoolRunner(std::size_t thread_count, boost::asio::io_context& context);
    ~IOContextThreadPoolRunner();

private:
    boost::thread_group pool_;
};

