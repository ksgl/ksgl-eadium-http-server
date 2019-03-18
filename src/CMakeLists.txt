add_executable(highload
    get_request_processor.hpp
    connection.hpp
    head_request_processor.hpp
    http_request_processor.hpp
    http_request_reader.hpp
    http_response_writer.hpp
    io_context_runner.hpp
    parser.hpp
    server.hpp
    unknown_request_processor.hpp
    util.hpp

    connection.cpp
    get_request_processor.cpp
    head_request_processor.cpp
    http_request_processor.cpp
    http_request_reader.cpp
    http_response_writer.cpp
    io_context_runner.cpp
    main.cpp
    server.cpp
    unknown_request_processor.cpp
    util.cpp
)

find_package(Boost REQUIRED COMPONENTS filesystem thread stacktrace_addr2line system)

target_link_libraries(highload PUBLIC Boost::boost)
target_link_libraries(highload PUBLIC Boost::disable_autolinking)
target_link_libraries(highload PUBLIC Boost::filesystem)
target_link_libraries(highload PUBLIC Boost::thread)
target_link_libraries(highload PUBLIC Boost::stacktrace_addr2line)
target_link_libraries(highload PUBLIC Boost::system)

target_link_libraries(highload PUBLIC dl)

target_compile_definitions(highload
    PUBLIC BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
)
