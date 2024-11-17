#include "../includes/webserv.hpp"

int main() {
    try {
        Server server(8080, "./srcs/public");
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}