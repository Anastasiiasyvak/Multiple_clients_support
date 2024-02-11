#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>


class TCPClient {
public:

    TCPClient(const char* serverIp, int port) {
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            perror("Error creating socket");
            exit(1);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, serverIp, &(serverAddr.sin_addr));
    }


    ~TCPClient() {
        close(clientSocket);
    }

    bool connectToServer() {
        return connect(clientSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) != -1;
    }

    void receiveServerMessage() const{
        char fileMessage[1024];
        memset(fileMessage, 0, sizeof(fileMessage));
        ssize_t bytes = recv(clientSocket, fileMessage, sizeof(fileMessage), 0);
        if (bytes > 0) {
            std::cout << fileMessage << std::endl;
        }
        else{
            std::cerr << "Failed to receive message from server" << std::endl;
        }

    }


    void command() const{
        std::cout << "1)GET <filename>\n2)LIST\n3)PUT <filename>\n4)DELETE <filename>\n5)INFO <filename>\n";

        while(true) {
            std::string command;

            std::cout << "Enter a command: ";
            getline(std::cin, command);

            if (command == "STOP") {
                break;
            }

            send(clientSocket, command.c_str(), command.size(), 0);
            if (command.find("GET ") == 0 || command.find("LIST") == 0 || command.find("PUT") == 0 || command.find("DELETE") == 0 || command.find("INFO") == 0){
                receiveServerMessage();
            }else{
                std::cout << "unknown command" << std::endl;
            }
        }

    }

    void getClientName() const{
        std::string name;
        while (true) {
            std::cout << "Enter client name: (to select folder on the server) ";
            std::getline(std::cin, name);
            if (!name.empty()) {
                break;
            }
            std::cout << "Name cannot be empty. Please enter your name again." << std::endl;
        }
        send(clientSocket, name.c_str(), name.size(), 0);
    }


private:
    int clientSocket;
    sockaddr_in serverAddr;
};


int main() {
    const char* serverIp = "127.0.0.1";
    int port = 12348;

    TCPClient client(serverIp, port);

    if (!client.connectToServer()) {
        perror("Connect failed");
        return 1;
    }

    client.getClientName();

    client.command();

    return 0;
}
