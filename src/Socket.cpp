#include "../header/Md_header.hpp"



extern volatile sig_atomic_t g_sig;

void Atr::Run() {

    int serverSocket;
    int clientSockets[3] = {0, 0, 0}; 
    struct sockaddr_in serverAddr;
    fd_set readFds;
    char buffer[1024];
    
    this->Obj.Log("Matt_daemon: Creating server.");
    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        this->Obj.Log("Matt_daemon: ERROR - Can't create socket");
        exit(1);
    }
    
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        this->Obj.Log("Matt_daemon: ERROR - setsockopt failed");
        close(serverSocket);
        exit(1);
    }
    
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(4242);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        this->Obj.Log("Matt_daemon: ERROR - Can't bind to port 4242");
        close(serverSocket);
        exit(1);
    }
    
    if (listen(serverSocket, 3) < 0) {
        this->Obj.Log("Matt_daemon: ERROR - Can't listen on socket");
        close(serverSocket);
        exit(1);
    }
    
    this->Obj.Log("Matt_daemon: Server created.");
    this->Obj.Log("Matt_daemon: Entering Daemon mode.");
    
    std::stringstream ss;
    ss << "Matt_daemon: started. PID: " << getpid();
    this->Obj.Log(ss.str());
    
    while (true) {
        FD_ZERO(&readFds);
        
        FD_SET(serverSocket, &readFds);
        int maxFd = serverSocket;
        
        for (int i = 0; i < 3; i++) {
            if (clientSockets[i] > 0) {
                FD_SET(clientSockets[i], &readFds);
                if (clientSockets[i] > maxFd)
                    maxFd = clientSockets[i];
            }
        }
        
        int activity = select(maxFd + 1, &readFds, NULL, NULL, NULL);
        if (g_sig != 0) {
            this->Obj.Log("Matt_daemon: Signal handler.");
            for (int j = 0; j < 3; j++) {
                if (clientSockets[j] > 0)
                    close(clientSockets[j]);
            }
            close(serverSocket);
            this->RemoveLockfile();
            this->Obj.Log("Matt_daemon: Quitting.");
            return;
        }
        if (activity < 0) {
            continue;
        }
        
        
        if (FD_ISSET(serverSocket, &readFds)) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int newSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            
            if (newSocket < 0) {
                this->Obj.Log("Matt_daemon: ERROR - Accept failed");
                continue;
            }
            
            bool added = false;
            for (int i = 0; i < 3; i++) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = newSocket;
                    this->Obj.Log("Matt_daemon: New client connected");
                    added = true;
                    break;
                }
            }
            
            if (!added) {
                this->Obj.Log("Matt_daemon: Max clients reached, rejecting connection");
                close(newSocket);
            }
        }
        
        for (int i = 0; i < 3; i++) {
            int clientSocket = clientSockets[i];
            
            if (clientSocket > 0 && FD_ISSET(clientSocket, &readFds)) {
                
                memset(buffer, 0, sizeof(buffer));
                ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                
                if (bytesRead <= 0) {
                   
                    this->Obj.Log("Matt_daemon: Client disconnected");
                    close(clientSocket);
                    clientSockets[i] = 0;
                } else {
                    
                    buffer[bytesRead] = '\0';
                    
                    size_t len = strlen(buffer);
                    while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
                        buffer[len-1] = '\0';
                        len--;
                    }
                    
                    if (strcmp(buffer, "quit") == 0) {
                        this->Obj.Log("Matt_daemon: Request quit.");
                        
                        for (int j = 0; j < 3; j++) {
                            if (clientSockets[j] > 0)
                                close(clientSockets[j]);
                        }
                        close(serverSocket);
                        this->RemoveLockfile();
                        this->Obj.Log("Matt_daemon: Quitting.");

                        return;  
                    }
                    
                  
                    std::string logMsg = "Matt_daemon: User input: ";
                    logMsg += buffer;
                    this->Obj.Log(logMsg, LOG);
                }
            }
        }
    }
}