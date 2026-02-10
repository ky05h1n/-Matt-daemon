#include "../header/Md_header.hpp"
#include "../header/ServerCore.hpp"

extern volatile sig_atomic_t g_sig;

void Atr::Run() {
    class MandatoryServer : public ServerCore {
    public:
        explicit MandatoryServer(Atr &owner)
            : ServerCore(4242, 3), atr(owner), maintenanceMode(false), adminSocket(-1) {}

    protected:
        bool onAccept(ClientSlot &client) {
            if (maintenanceMode) {
                sendText(client.fd, "Server in maintenance mode. Try later.");
                return false;
            }
            atr.Obj.Log("Matt_daemon: New client connected", LEVEL_INFO);
            return true;
        }

        void onReject(int clientFd) {
            atr.Obj.Log("Matt_daemon: Max clients reached, rejecting connection", LEVEL_ERROR);
            sendText(clientFd, "Server busy. Try later.");
        }

        void onClientReadable(ClientSlot &client) {
            char buffer[1024];
            std::memset(buffer, 0, sizeof(buffer));
            ssize_t bytesRead = recv(client.fd, buffer, sizeof(buffer) - 1, 0);

            if (bytesRead <= 0) {
                atr.Obj.Log("Matt_daemon: Client disconnected", LEVEL_INFO);
                closeClient(client);
                return;
            }

            buffer[bytesRead] = '\0';
            size_t len = strlen(buffer);
            while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
                buffer[len - 1] = '\0';
                len--;
            }

            if (strcmp(buffer, "quit") == 0) {
                atr.Obj.Log("Matt_daemon: Request quit.", LEVEL_ERROR);
                atr.RemoveLockfile();
                atr.Obj.Log("Matt_daemon: Quitting.", LEVEL_ERROR);
                requestStop();
                return;
            }

            if (strcmp(buffer, "CMD_LOCK") == 0) {
                maintenanceMode = true;
                adminSocket = client.fd;
                atr.Obj.Log("Matt_daemon: Maintenance mode enabled.", LEVEL_INFO);
                sendText(client.fd, "Daemon locked. New connections rejected.");
                return;
            }
            if (strcmp(buffer, "CMD_UNLOCK") == 0) {
                if (client.fd == adminSocket) {
                    maintenanceMode = false;
                    adminSocket = -1;
                    atr.Obj.Log("Matt_daemon: Maintenance mode disabled.", LEVEL_INFO);
                    sendText(client.fd, "Daemon unlocked. New connections accepted.");
                } else {
                    sendText(client.fd, "Only admin can unlock.");
                }
                return;
            }
            if (strcmp(buffer, "CMD_STATUS") == 0) {
                std::stringstream status;
                status << "Log size: " << atr.Obj.getLogFileSize() << " bytes, maintenance: ";
                status << (maintenanceMode ? "ON" : "OFF");
                sendText(client.fd, status.str());
                return;
            }

            std::string logMsg = "Matt_daemon: User input: ";
            logMsg += buffer;
            atr.Obj.Log(logMsg, LEVEL_LOG);
        }

        void onClientDisconnect(ClientSlot &client, int lastFd) {
            (void)client;
            if (lastFd == adminSocket) {
                maintenanceMode = false;
                adminSocket = -1;
            }
        }

        bool shouldStop() {
            if (g_sig != 0) {
                atr.Obj.Log("Matt_daemon: Signal handler.", LEVEL_ERROR);
                atr.RemoveLockfile();
                atr.Obj.Log("Matt_daemon: Quitting.", LEVEL_ERROR);
                return true;
            }
            return false;
        }

    private:
        Atr &atr;
        bool maintenanceMode;
        int adminSocket;
    };

    this->Obj.Log("Matt_daemon: Creating server.", LEVEL_INFO);
    MandatoryServer server(*this);
    if (!server.run()) {
        this->Obj.Log("Matt_daemon: ERROR - Server setup failed", LEVEL_ERROR);
        exit(1);
    }
}