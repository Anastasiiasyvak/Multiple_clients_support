#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <fstream>

class LinuxNetworkSystem {
public:
    static int createSocket(int domain, int type, int protocol) {
        return socket(domain, type, protocol);
    }

    static int bindSocket(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
        return bind(sockfd, addr, addrlen);
    }

    static int listenSocket(int sockfd, int backlog) {
        return listen(sockfd, backlog);
    }

    static int acceptConnection(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
        return accept(sockfd, addr, addrlen);
    }

    static ssize_t receiveData(int sockfd, void *buf, size_t len, int flags) {
        return recv(sockfd, buf, len, flags);
    }

    static ssize_t sendData(int sockfd, const void *buf, size_t len, int flags) {
        return send(sockfd, buf, len, flags);
    }

    static int closeSocket(int sockfd) {
        return close(sockfd);
    }
};

class TCPServer {
public:
    TCPServer(int port) {
        serverSocket = LinuxNetworkSystem::createSocket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            perror("Error creating socket");
            exit(1);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (LinuxNetworkSystem::bindSocket(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
            perror("Bind failed");
            LinuxNetworkSystem::closeSocket(serverSocket);
            exit(1);
        }

        if (LinuxNetworkSystem::listenSocket(serverSocket, SOMAXCONN) == -1) {
            perror("Listen failed");
            NetworkSystem::closeSocket(serverSocket);
            exit(1);
        }

        std::cout << "Server listening on port " << port << std::endl;
    }

    ~TCPServer() {
        LinuxNetworkSystem::closeSocket(serverSocket);
    }

    void startListening() {
        while (true) {
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);

            int clientSocket = LinuxNetworkSystem::acceptConnection(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
            if (clientSocket == -1) {
                perror("Accept failed");
                LinuxNetworkSystem::closeSocket(serverSocket);
                exit(1);
            }

            std::cout << "Accepted connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) <<
                      std::endl;

            std::thread clientThread(&TCPServer::handleClientRequest, this, clientSocket);
            clientThread.detach();
        }
    }

private:
    int serverSocket;
    sockaddr_in serverAddr;

    std::string serverDirectory = "/home/nastia/CLionProjects/second_csc/serverStorage";
    std::string clientDirectory = "/home/nastia/CLionProjects/second_csc/clientStorage";

    void handleClientRequest(int clientSocket) {
        char buffer[1024];
        memset(buffer, 0, 1024);
        ssize_t bytesReceived = LinuxNetworkSystem::receiveData(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0) {
            std::cout << "Received command: " << buffer << std::endl;
            std::string command(buffer);

            if (command.find("NAME ") == 0) {
                std::string clientName = command.substr(5);
                processClientCommands(clientSocket, clientName);
            }
            else {
                const char* response = "Unknown command";
                LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
            }
        }
        LinuxNetworkSystem::closeSocket(clientSocket);
    }

    void processClientCommands(int clientSocket, const std::string& clientName) {

        std::string serverClientDirectory = serverDirectory + "/" + clientName;
        struct stat directoryInfo;
        if (stat(serverClientDirectory.c_str(), &directoryInfo) != 0) {
            if (mkdir(serverClientDirectory.c_str(), 0777) != 0) {
                const char* response = "Error creating client folder on server";
                LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
                return;
            }
        }

        while (true) {
            char buffer[1024];
            memset(buffer, 0, 1024);
            ssize_t bytesReceived = LinuxNetworkSystem::receiveData(clientSocket, buffer, sizeof(buffer), 0);

            if (bytesReceived > 0) {
                std::string command(buffer);
                std::cout << "Received command from client " << clientName << ": " << command << std::endl;

                if (command.find("GET ") == 0) {
                    std::string filename = command.substr(4);
                    getFile(clientSocket, clientName, filename);
                }
                else if (command.find("LIST") == 0) {
                    sendFileList(clientSocket, clientName);
                }
                else if (command.find("PUT") == 0){
                    std::string filename = command.substr(4);
                    putFile(clientSocket, clientName, filename);
                }
                else if (command.find("DELETE") == 0){
                    std::string filename = command.substr(7);
                    deleteFile(clientSocket, clientName, filename);
                }
                else if (command.find("INFO") == 0){
                    std::string filename = command.substr(5);
                    retrieveFileInfo(clientSocket, clientName, filename);
                }
                else {
                    const char* response = "Unknown command";
                    LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
                }
            } else {
                break;
            }
        }
    }

    void retrieveFileInfo(int clientSocket, const std::string& clientName, const std::string& filename) {
        std::string fullFilePath = serverDirectory + "/" + clientName + "/" + filename;

        struct stat fileStat;
        if (stat(fullFilePath.c_str(), &fileStat) == 0) {
            std::string info = "File Information:\n";
            info += "Size: " + std::to_string(fileStat.st_size) + " bytes\n";
            info += "Last modified: " + std::string(ctime(&fileStat.st_mtime));
            LinuxNetworkSystem::sendData(clientSocket, info.c_str(), info.size(), 0);
        } else {
            const char* response = "File not found.";
            LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
        }
    }

    void deleteFile(int clientSocket, const std::string& clientName, const std::string& filename) {
        std::string fullFilePath = serverDirectory + "/" + clientName + "/" + filename;

        if (remove(fullFilePath.c_str()) == 0) {
            const char* response = "File deleted successfully";
            LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
        } else {
            const char* response = "Error deleting file";
            LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
        }
    }

    void putFile(int clientSocket, const std::string& clientName, const std::string& filename) {
        std::string clientFilePath = clientDirectory + "/" + clientName + "/" + filename;
        std::string serverFilePath = serverDirectory + "/" + clientName + "/" + filename;

        std::ifstream inputFile(clientFilePath);
        if (!inputFile) {
            const char* response = "Error opening file on client";
            LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
            return;
        }

        std::ofstream outputFile(serverFilePath);
        if (!outputFile) {
            const char* response = "Error creating file on server";
            LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
            return;
        }

        outputFile << inputFile.rdbuf();

        const char* response = "File received successfully";
        LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);

        if (remove(clientFilePath.c_str()) != 0) {
            std::cerr << "Error deleting file from clientDirectory" << std::endl;
        }
    }

    void getFile(int clientSocket, const std::string& clientName, const std::string& filename) {
        std::string serverFilePath = serverDirectory + "/" + clientName + "/" + filename;
        std::string clientFilePath = clientDirectory + "/" + clientName + "/" + filename;

        std::ifstream file(serverFilePath);
        if (file) {
            std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            LinuxNetworkSystem::sendData(clientSocket, fileContent.c_str(), fileContent.size(), 0);

            std::ofstream clientFile(clientFilePath);
            if (clientFile) {
                clientFile << fileContent;
                clientFile.close();
            } else {
                std::cerr << "Error creating file in clientDirectory" << std::endl;
            }

            if (remove(serverFilePath.c_str()) != 0) {
                std::cerr << "Error deleting file from serverDirectory" << std::endl;
            }
        } else {
            const char* response = "File not found.";
            LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
        }
    }

    void sendFileList(int clientSocket, const std::string& clientName){
        std::string fileList;
        std::string path = serverDirectory + "/" + clientName;

        DIR *dir;
        struct dirent *ent;

        if ((dir = opendir(path.c_str())) != NULL){
            while ((ent = readdir(dir)) != NULL){
                if (std::string(ent->d_name) != "." && std::string(ent->d_name) != "..") {
                    fileList += ent->d_name;
                    fileList += "\n";
                }
            }
            closedir(dir);
        } else {
            const char* response = "Error reading directory";
            LinuxNetworkSystem::sendData(clientSocket, response, strlen(response), 0);
            return;
        }
        LinuxNetworkSystem::sendData(clientSocket, fileList.c_str(), fileList.size(), 0);
    }
};

int main() {
    int port = 12348;
    TCPServer server(port);

    server.startListening();

    return 0;
}
