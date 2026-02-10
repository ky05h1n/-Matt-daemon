#pragma once

#include "Md_header.hpp"
#include <vector>

class ServerCore {
public:
    ServerCore(int port = 4242, int maxClients = 3);
    virtual ~ServerCore();

    bool run();

protected:
    struct ClientSlot {
        int fd;
        int id;
    };

    virtual bool onAccept(ClientSlot &client);
    virtual void onClientReadable(ClientSlot &client) = 0;
    virtual void onClientDisconnect(ClientSlot &client, int lastFd);
    virtual void onReject(int clientFd);
    virtual bool shouldStop();

    void closeClient(ClientSlot &client);
    void requestStop();
    void sendText(int fd, const std::string &text);

private:
    int serverFd;
    int port;
    int maxClients;
    bool stopRequested;
    std::vector<ClientSlot> clients;

    bool setupServer();
    void closeAllClients();
};
