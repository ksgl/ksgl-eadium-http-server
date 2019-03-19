#include "io_context_runner.hpp"
#include "server.hpp"
#include "parser.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/stacktrace.hpp>

#include <iostream>
#include <string>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>


extern "C" void handle_critical_signal(int signal) {
    try {
        std::fprintf(stderr, "Stack trace:\n%s", boost::lexical_cast<std::string>(boost::stacktrace::stacktrace()).c_str());
    }
    catch (...) {
        std::fprintf(stderr, "Exception: unable to dump the stacktrace");
    }

    const int exitCode = 128 + signal;
    const char* imm = std::getenv("ALLOW_ATEXIT_ON_SIGNAL");
    if (imm) {
        std::fprintf(stderr, "Shutdown on signal: %d (CRITICAL)", signal);
        std::exit(exitCode);
    }
    else {
        std::fprintf(stderr, "Immediate shutdown on signal: %d (CRITICAL)", signal);
        std::_Exit(exitCode);
    }
}

extern "C" void handle_SIGPIPE_signal(int signal) {
    if (signal == SIGPIPE) {
        std::fprintf(stderr, "Caught SIGPIPE, ignoring...\n");
    }
}

int main(int argc, const char* argv[]) {
    int exit_code = EXIT_SUCCESS;

    try {
        // Installing the critical signal handler.
        struct ::sigaction signal_handler;
        std::memset(&signal_handler, 0, sizeof(signal_handler));
        signal_handler.sa_handler = handle_critical_signal;
//      ::sigaction(SIGBUS,  &signal_handler, nullptr);
        ::sigaction(SIGILL,  &signal_handler, nullptr);
        ::sigaction(SIGABRT, &signal_handler, nullptr);
        ::sigaction(SIGFPE,  &signal_handler, nullptr);
        ::sigaction(SIGSEGV, &signal_handler, nullptr);

        struct ::sigaction signal_handler_SIGPIPE;
        std::memset(&signal_handler_SIGPIPE, 0, sizeof(signal_handler_SIGPIPE));
        signal_handler_SIGPIPE.sa_handler = handle_SIGPIPE_signal;
        ::sigaction(SIGPIPE, &signal_handler_SIGPIPE, nullptr);

        CfgParser parser = CfgParser("/etc/httpd.conf");
        parser.parse_config();
        Settings* cfg = parser.get_setting();

        const auto thread_pool_size = std::min<std::size_t>(std::thread::hardware_concurrency(), cfg->cpu_limit);

        boost::asio::io_context io_context(thread_pool_size); // даем подсказу io_context-у во скольких тредах его будем run()

        auto server = std::make_shared<Server>(io_context, uint16_t(cfg->port), cfg->document_root);

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

        signals.async_wait(
            [server] (const boost::system::error_code& ec, int signal) {
                if (!ec) {
                    std::cout << "Caught signal " << signal << " - stopping the server...." << std::endl;
                    server->stop();
                }
            }
        );

        std::clog << "cpu_limit: " << cfg->cpu_limit << std::endl;
        std::clog << "Start listening on port " << cfg->port << std::endl;

        server->start();

        IOContextThreadPoolRunner(thread_pool_size, io_context);
    }
    // Use fprintf here to avoid riscs of new uncaught exceptions.
    catch (const std::exception& ex) {
        std::fprintf(stderr, "\nError: %s\n", ex.what());
        exit_code = EXIT_FAILURE;
    }
    catch (...) {
        std::fprintf(stderr, "\nError: unknown\n");
        exit_code = EXIT_FAILURE;
    }

    return exit_code;
}
