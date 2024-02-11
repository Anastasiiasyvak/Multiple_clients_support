#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <thread>

class TCPServer {
public:
    TCPServer(int port) {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            perror("Error creating socket");
            exit(1);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
            perror("Bind failed");
            close(serverSocket);
            exit(1);
        }

        if (listen(serverSocket, SOMAXCONN) == -1) {
            perror("Listen failed");
            close(serverSocket);
            exit(1);
        }

        std::cout << "Server listening on port " << port << std::endl;
    }

    ~TCPServer() {
        close(serverSocket);
    }

    void startListening() {
        while (true) {
            sockaddr_in clientAddr{};
            socklen_t clientAddrLen = sizeof(clientAddr);

            int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
            if (clientSocket == -1) {
                perror("Accept failed");
                close(serverSocket);
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
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0) {
            std::cout << "Received command: " << buffer << std::endl;
            std::string command(buffer);

            if (command.find("NAME ") == 0) {
                std::string clientName = command.substr(5);
                processClientCommands(clientSocket, clientName);
            }
            else {
                const char* response = "Unknown command";
                send(clientSocket, response, strlen(response), 0);
            }
        }
        close(clientSocket);
    }

    void processClientCommands(int clientSocket, const std::string& clientName) {
        while (true) {
            char buffer[1024];
            memset(buffer, 0, 1024);
            ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

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
                    send(clientSocket, response, strlen(response), 0);
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

            send(clientSocket, info.c_str(), info.size(), 0);
        } else {
            const char* response = "File not found.";
            send(clientSocket, response, strlen(response), 0);
        }
    }

    void deleteFile(int clientSocket, const std::string& clientName, const std::string& filename) {
        std::string fullFilePath = serverDirectory + "/" + clientName + "/" + filename;

        if (remove(fullFilePath.c_str()) == 0) {
            const char* response = "File deleted successfully";
            send(clientSocket, response, strlen(response), 0);
        } else {
            const char* response = "Error deleting file";
            send(clientSocket, response, strlen(response), 0);
        }
    }

    void putFile(int clientSocket, const std::string& clientName, const std::string& filename) {
        std::string clientFilePath = clientDirectory + "/" + clientName + "/" + filename;
        std::string serverFilePath = serverDirectory + "/" + clientName + "/" + filename;

        std::ifstream inputFile(clientFilePath, std::ios::binary);
        if (!inputFile) {
            const char* response = "Error opening file on client";
            send(clientSocket, response, strlen(response), 0);
            return;
        }

        std::ofstream outputFile(serverFilePath, std::ios::binary);
        if (!outputFile) {
            const char* response = "Error creating file on server";
            send(clientSocket, response, strlen(response), 0);
            return;
        }

        outputFile << inputFile.rdbuf();

        const char* response = "File received successfully";
        send(clientSocket, response, strlen(response), 0);

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
            send(clientSocket, fileContent.c_str(), fileContent.size(), 0);

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
            send(clientSocket, response, strlen(response), 0);
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
            send(clientSocket, response, strlen(response), 0);
            return;
        }
        send(clientSocket, fileList.c_str(), fileList.size(), 0);
    }
};

int main() {
    int port = 12348;
    TCPServer server(port);

    server.startListening();

    return 0;
}
