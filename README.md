# Communication Protocol Description

## Short Description:

The project consists of two components: a server and a client utilizing a TCP connection. The server employs multi-threading to concurrently serve multiple clients, enabling them to interact with files stored on the server. The client application facilitates sending commands to the server to manage files, including retrieval, upload, deletion, and obtaining file information.

## Interaction Process:

The server initializes on port 12348 with the IP Address 127.0.0.1 (IPv4, AF_INET), utilizing TCP (SOCK_STREAM). Each char transmitted corresponds to 1 byte in C++.

### Server-side Interactive Process:

1. **Creating the TCP Server:**
   - Establishing a TCP server that awaits connections on a designated port.
2. **Handling Client Connections:**
   - Accepting connections from clients and spawning individual threads to process their requests.
3. **Processing Client Commands:**
   - Executing commands from clients, such as retrieving file lists, uploading, and deleting files.

### Client Interaction Process:

1. **Connecting to the Server:**
   - Establishing a TCP connection with the server.
2. **Sending Commands:**
   - Transmitting commands to the server to manage files, including retrieval, upload, deletion, and obtaining file information.
3. **Receiving Responses:**
   - Receiving and presenting server responses to the user.

### Commands:

- **GET <'filename'>:**
  - *Description:* Requests a specific file from the server.
  - *Bytes:* 4 bytes for the command, plus the size of the filename and 1 byte for the null terminator ('\0').
  - *Process:* Initiates the file retrieval process from the server to the client.

- **LIST:**
  - *Description:* Requests a list of all files present in the server's directory.
  - *Bytes:* 4 bytes for the command.
  - *Process:* Triggers the server to generate and transmit a list of filenames to the client.

- **PUT <'filename'>:**
  - *Description:* Instructs the server to accept a file upload from the client.
  - *Bytes:* 4 bytes for the command, plus the size of the filename and 1 byte for the null terminator ('\0').
  - *Process:* Facilitates the transmission of a file from the client to the server.

- **DELETE <'filename'>:**
  - *Description:* Prompts the server to delete the specified file.
  - *Bytes:* 7 bytes for the command, plus the size of the filename and 1 byte for the null terminator ('\0').
  - *Process:* Initiates the file deletion process on the server.

- **INFO <'filename'>:**
  - *Description:* Requests information about the specified file from the server.
  - *Bytes:* 7 bytes for the command, plus the size of the filename and 1 byte for the null terminator ('\0').
  - *Process:* Triggers the server to provide metadata about the specified file.


**Compilation and Execution Requirements**

- To compile the server: `g++ server.cpp -o server`
- To run the server: `./server`

- To compile the client: `g++ client.cpp -o client`
- To run the client: `./client`

