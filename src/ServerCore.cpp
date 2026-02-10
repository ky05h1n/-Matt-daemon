#include "../header/ServerCore.hpp"

ServerCore::ServerCore(int portValue, int maxClientsValue)
    : serverFd(-1), port(portValue), maxClients(maxClientsValue), stopRequested(false) {
    clients.resize(static_cast<size_t>(maxClients));
    for (int i = 0; i < maxClients; ++i) {
        clients[i].fd = -1;
        clients[i].id = i + 1;
    }
}

ServerCore::~ServerCore() {
    closeAllClients();
    if (serverFd >= 0) {
        close(serverFd);
        serverFd = -1;
    }
}

bool ServerCore::setupServer() {
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        return false;
    }

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverFd);
        serverFd = -1;
        return false;
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        close(serverFd);
        serverFd = -1;
        return false;
    }

    if (listen(serverFd, maxClients) < 0) {
        close(serverFd);
        serverFd = -1;
        return false;
    }

    return true;
}

void ServerCore::closeAllClients() {
    for (int i = 0; i < maxClients; ++i) {
        if (clients[i].fd >= 0) {
            close(clients[i].fd);
            clients[i].fd = -1;
        }
    }
}

bool ServerCore::run() {
    if (!setupServer()) {
        return false;
    }

    while (!stopRequested) {
        if (shouldStop()) {
            stopRequested = true;
            break;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverFd, &readfds);
        int maxFd = serverFd;

        for (int i = 0; i < maxClients; ++i) {
            if (clients[i].fd >= 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > maxFd) {
                    maxFd = clients[i].fd;
                }
            }
        }

        int activity = select(maxFd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            continue;
        }

        if (FD_ISSET(serverFd, &readfds)) {
            sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int clientSocket = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
            if (clientSocket >= 0) {
                bool added = false;
                for (int i = 0; i < maxClients; ++i) {
                    if (clients[i].fd < 0) {
                        clients[i].fd = clientSocket;
                        if (onAccept(clients[i])) {
                            added = true;
                        } else {
                            close(clientSocket);
                            clients[i].fd = -1;
                        }
                        break;
                    }
                }
                if (!added && clientSocket >= 0) {
                    onReject(clientSocket);
                    close(clientSocket);
                }
            }
        }

        for (int i = 0; i < maxClients; ++i) {
            if (clients[i].fd < 0) {
                continue;
            }
            if (!FD_ISSET(clients[i].fd, &readfds)) {
                continue;
            }

            int prevFd = clients[i].fd;
            onClientReadable(clients[i]);
            if (prevFd >= 0 && clients[i].fd < 0) {
                onClientDisconnect(clients[i], prevFd);
            }
        }
    }

    closeAllClients();
    if (serverFd >= 0) {
        close(serverFd);
        serverFd = -1;
    }
    return true;
}

bool ServerCore::onAccept(ClientSlot &client) {
    (void)client;
    return true;
}

void ServerCore::onClientDisconnect(ClientSlot &client, int lastFd) {
    (void)client;
    (void)lastFd;
}

void ServerCore::onReject(int clientFd) {
    (void)clientFd;
}

bool ServerCore::shouldStop() {
    return false;
}

void ServerCore::closeClient(ClientSlot &client) {
    if (client.fd >= 0) {
        close(client.fd);
        client.fd = -1;
    }
}

void ServerCore::requestStop() {
    stopRequested = true;
}

void ServerCore::sendText(int fd, const std::string &text) {
    send(fd, text.c_str(), text.size(), 0);
}
