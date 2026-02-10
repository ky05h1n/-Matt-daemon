#include "Bonus_header.hpp"

static void print_usage(const char *prog) {
    std::cout << "Usage: " << prog << " <server_ip> [--encrypted <key>]" << std::endl;
}

int main(int argc, char **argv) {
    if (argc != 2 && argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    std::string server_ip = argv[1];
    bool encrypted_mode = false;
    std::string key;
    if (argc == 4) {
        if (std::string(argv[2]) != "--encrypted") {
            print_usage(argv[0]);
            return 1;
        }
        encrypted_mode = true;
        key = argv[3];
        if (key.empty()) {
            std::cout << "Invalid key." << std::endl;
            return 1;
        }
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cout << "Failed to create socket." << std::endl;
        return 1;
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(4242);
    if (inet_pton(AF_INET, server_ip.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cout << "Invalid server IP." << std::endl;
        close(sock);
        return 1;
    }

    if (connect(sock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cout << "Connect failed." << std::endl;
        close(sock);
        return 1;
    }

    if (encrypted_mode) {
        if (!send_message(sock, false, "ENC")) {
            std::cout << "Send failed." << std::endl;
            close(sock);
            return 1;
        }
        if (!send_message(sock, false, key)) {
            std::cout << "Send failed." << std::endl;
            close(sock);
            return 1;
        }
    } else {
        if (!send_message(sock, false, "PLAIN")) {
            std::cout << "Send failed." << std::endl;
            close(sock);
            return 1;
        }
    }

    std::string mode;
    if (!recv_message(sock, false, mode)) {
        std::cout << "Server disconnected." << std::endl;
        close(sock);
        return 1;
    }

    if (mode.rfind("ERROR:", 0) == 0) {
        std::cout << mode << std::endl;
        close(sock);
        return 1;
    }
    if (mode.rfind("SERVER_BUSY", 0) == 0) {
        std::cout << mode << std::endl;
        close(sock);
        return 1;
    }

    bool use_cipher = (mode == "MODE:ROT13");

    std::string prompt;
    if (!recv_message(sock, use_cipher, prompt)) {
        std::cout << "Server disconnected." << std::endl;
        close(sock);
        return 1;
    }

    std::cout << prompt << std::endl;
    char *pw = getpass("");
    if (!pw) {
        close(sock);
        return 1;
    }
    std::string password(pw);
    if (!send_message(sock, use_cipher, password)) {
        std::cout << "Send failed." << std::endl;
        close(sock);
        return 1;
    }

    std::string auth_reply;
    if (!recv_message(sock, use_cipher, auth_reply)) {
        std::cout << "Server disconnected." << std::endl;
        close(sock);
        return 1;
    }

    if (auth_reply != "AUTH_OK") {
        std::cout << "Authentication failed." << std::endl;
        close(sock);
        return 1;
    }

    std::cout << "Authenticated. Type messages (quit to exit)." << std::endl;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int maxfd = sock > STDIN_FILENO ? sock : STDIN_FILENO;
        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            continue;
        }

        if (FD_ISSET(sock, &readfds)) {
            std::string reply;
            if (!recv_message(sock, use_cipher, reply)) {
                std::cout << "Server disconnected." << std::endl;
                break;
            }
            std::cout << reply << std::endl;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            std::string line;
            if (!std::getline(std::cin, line)) {
                break;
            }
            if (!send_message(sock, use_cipher, line)) {
                std::cout << "Send failed." << std::endl;
                break;
            }
            if (line == "quit") {
                break;
            }
        }
    }

    close(sock);
    return 0;
}
