#include "../includes/webserv.hpp"

int main(int argc, char  **argv) 
{
    (void)argc;
    try {
        Server server(argv[1]);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}