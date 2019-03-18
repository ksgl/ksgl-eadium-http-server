#include "io_context_runner.hpp"


IOContextThreadPoolRunner::IOContextThreadPoolRunner(std::size_t thread_count, boost::asio::io_context& context) {
    for (std::size_t i = 0; i < thread_count; i++) {
        pool_.add_thread(new boost::thread(
            [&context] () {
                while (!context.stopped()) {
                    try {
                        // This will block until io_context runs out of work or stopped.
                        context.run();
                    }
                    catch (const std::exception& ex) {
                        std::fprintf(stderr, "\nError: %s\n", ex.what());
                    }
                    catch (...) {
                        std::fprintf(stderr, "\nError: unknown\n");
                    }
                }
            })
        );
    }
}

IOContextThreadPoolRunner::~IOContextThreadPoolRunner() {
    pool_.join_all();
}

