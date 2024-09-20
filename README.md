# QuizConnect

QuizConnect is a multiplayer trivia game where clients connect to a server and answer trivia questions. The game features real-time communication between the server and multiple clients, with a variety of trivia categories to test your knowledge.

## Features

- Supports multiple clients connecting to the server.
- Trivia questions on different topics.
- Real-time communication between server and clients.
- Tracks player responses and scores.

## Requirements

- A C compiler (e.g., `gcc`).
- Make (for using the Makefile).
- A terminal for running the server and client.

## Installation

1. Clone this repository or download the source code.
2. Open a terminal and navigate to the project directory.

## Compilation

Use the provided Makefile to compile the `server.c` and `client.c` files. Run the following command:

```bash
make all
```
This will create the server and client executables.

## Running the Game
### Running the Server
To start the server, simply run:
```bash
./server
```
Optionally, you can specify a port or retrieve the IP address using flags:
```bash
./server -p [PORT_NUMBER] -i [IP]
```
- The -p flag allows you to specify a custom port number (optional).
- The -i flag will you to specify a custom IP (optional).

If no IP or port is provided, default values will be used.

### Running the Client
To start a client and connect to the server, use:
```bash
./client
```
Optionally, you can specify the IP address and port number using flags:
```bash
./client -p [PORT_NUMBER] -i [IP]
```
- The -i flag specifies the IP address of the server (optional).
- The -p flag specifies the port number (optional).
  
If no IP or port is provided, default values will be used.

## How to Play
1. Start the server with the command mentioned above.
2. Clients can connect to the server using the appropriate command.
3. Once connected, clients will receive trivia questions to answer. Each correct answer earns points.
4. The game continues as clients answer questions in real-time.


