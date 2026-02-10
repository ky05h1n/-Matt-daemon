#include "Bonus_header.hpp"
#include "bonus/Mail.hpp"
#include "ServerCore.hpp"

static bool smtpLoaded = false;
static SmtpConfig smtpConfig;

static void maybe_send_mail(LogLevel level, const std::string &message) {
    if (!smtpLoaded) {
        load_smtp_config(smtpConfig);
        smtpLoaded = true;
    }
    if (should_send_mail(smtpConfig, level, message)) {
        send_smtp_mail(smtpConfig, message);
    }
}

static void print_usage(const char *prog) {
    std::cout << "Usage: " << prog << " [--encrypted <key>]" << std::endl;
    std::cout << "Env: MD_AUTH_PASSWORD_CLIENT_1..3 (default: mattdaemon)" << std::endl;
}

int main(int argc, char **argv) {
    bool encrypted_mode = false;
    std::string expected_key;
    if (argc == 3 && std::string(argv[1]) == "--encrypted") {
        encrypted_mode = true;
        expected_key = argv[2];
    } else if (argc == 1) {
        encrypted_mode = false;
    } else {
        print_usage(argv[0]);
        return 1;
    }

    if (geteuid() != 0) {
        std::cout << "U should be root !" << std::endl;
        return 1;
    }

    if (encrypted_mode && expected_key.empty()) {
        std::cout << "Invalid key." << std::endl;
        return 1;
    }

    const char *env_pass1 = std::getenv("MD_AUTH_PASSWORD_CLIENT_1");
    const char *env_pass2 = std::getenv("MD_AUTH_PASSWORD_CLIENT_2");
    const char *env_pass3 = std::getenv("MD_AUTH_PASSWORD_CLIENT_3");
    std::string passwords[3];
    passwords[0] = env_pass1 ? std::string(env_pass1) : "mattdaemon";
    passwords[1] = env_pass2 ? std::string(env_pass2) : "mattdaemon";
    passwords[2] = env_pass3 ? std::string(env_pass3) : "mattdaemon";
    signal(SIGPIPE, SIG_IGN);


    std::cout << "Bonus daemon listening on port 4242." << std::endl;

    class BonusServer : public ServerCore {
    public:
                BonusServer(const std::string &expected, bool encrypted, const std::string pass_list[3])
            : ServerCore(4242, 3), expectedKey(expected), encryptedMode(encrypted),
              adminSocket(-1) {
            for (int i = 0; i < 3; ++i) {
                useCipher[i] = false;
                stage[i] = 0;
                                passwords[i] = pass_list[i];
                activeFds[i] = -1;
            }
        }

    protected:
        bool onAccept(ClientSlot &client) {
            useCipher[client.id - 1] = false;
            stage[client.id - 1] = 0;
            activeFds[client.id - 1] = client.fd;
            return true;
        }

        void onReject(int clientFd) {
            send_message(clientFd, false, "SERVER_BUSY: max clients reached");
        }

        void onClientReadable(ClientSlot &client) {
            int idx = client.id - 1;
            if (stage[idx] == 0) {
                std::string hello;
                if (!recv_message(client.fd, false, hello)) {
                    closeClient(client);
                    return;
                }
                if (encryptedMode) {
                    if (hello != "ENC") {
                        send_message(client.fd, false, "ERROR: encrypted mode required");
                        closeClient(client);
                        return;
                    }
                    std::string client_key;
                    if (!recv_message(client.fd, false, client_key)) {
                        closeClient(client);
                        return;
                    }
                    if (client_key != expectedKey) {
                        send_message(client.fd, false, "ERROR: bad key");
                        closeClient(client);
                        return;
                    }
                    useCipher[idx] = true;
                    if (!send_message(client.fd, false, "MODE:ROT13")) {
                        closeClient(client);
                        return;
                    }
                    if (!send_message(client.fd, true, "Enter Your Password")) {
                        closeClient(client);
                        return;
                    }
                    stage[idx] = 1;
                    return;
                } else {
                    if (hello != "PLAIN") {
                        send_message(client.fd, false, "ERROR: plain mode required");
                        closeClient(client);
                        return;
                    }
                    useCipher[idx] = false;
                    if (!send_message(client.fd, false, "MODE:PLAIN")) {
                        closeClient(client);
                        return;
                    }
                    if (!send_message(client.fd, false, "Enter Your Password")) {
                        closeClient(client);
                        return;
                    }
                    stage[idx] = 1;
                    return;
                }
            }

            if (stage[idx] == 1) {
                std::string provided;
                if (!recv_message(client.fd, useCipher[idx], provided)) {
                    closeClient(client);
                    return;
                }
                if (provided != passwords[idx]) {
                    std::string msg = "[AUTH] Failure from client " + std::to_string(client.id);
                    append_log_line(msg);
                    maybe_send_mail(LEVEL_ERROR, "[BONUS][AUTH] failed from client " + std::to_string(client.id));
                    send_message(client.fd, useCipher[idx], "AUTH_FAIL");
                    closeClient(client);
                    return;
                }
                if (!send_message(client.fd, useCipher[idx], "AUTH_OK")) {
                    closeClient(client);
                    return;
                }
                append_log_line("[ INFO ] - Client " + std::to_string(client.id) + " authenticated successfully");
                send_message(client.fd, useCipher[idx], "You are Client " + std::to_string(client.id));
                std::cout << "Client " << client.id << " authenticated." << std::endl;
                stage[idx] = 2;
                return;
            }

            if (stage[idx] == 2) {
                std::string input;
                if (!recv_message(client.fd, useCipher[idx], input)) {
                    closeClient(client);
                    return;
                }
                std::cout << "Client " << client.id << " received: " << input << std::endl;

                if (input == "quit") {
                    std::cout << "Client " << client.id << " requested quit." << std::endl;
                    send_message(client.fd, useCipher[idx], "BYE");
                    requestStop();
                    return;
                }

                std::string message = input;
                if (message.rfind("BROADCAST ", 0) == 0) {
                    message = message.substr(10);
                }

                append_log_line("Broadcast from Client " + std::to_string(client.id) + ": " + message);
                maybe_send_mail(LEVEL_LOG, "[BONUS][Client " + std::to_string(client.id) + "] " + message);

                for (int i = 0; i < 3; ++i) {
                    if (i == idx) {
                        continue;
                    }
                    if (activeFds[i] < 0) {
                        continue;
                    }
                    std::string out = "[Client " + std::to_string(client.id) + "] " + message;
                    if (!send_message(activeFds[i], useCipher[i], out)) {
                        continue;
                    }
                }
            }
        }

        void onClientDisconnect(ClientSlot &client, int lastFd) {
            int idx = client.id - 1;
            useCipher[idx] = false;
            stage[idx] = 0;
            append_log_line("[ INFO ] - Client " + std::to_string(client.id) + " disconnected");
            activeFds[idx] = -1;
            if (lastFd == adminSocket) {
                adminSocket = -1;
            }
        }

    private:
        std::string expectedKey;
        bool encryptedMode;
        std::string passwords[3];
        int adminSocket;
        bool useCipher[3];
        int stage[3];
        int activeFds[3];
    };

    BonusServer server(expected_key, encrypted_mode, passwords);
    if (!server.run()) {
        std::cout << "Server setup failed." << std::endl;
        return 1;
    }
    return 0;
}
