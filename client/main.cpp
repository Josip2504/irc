#include "IrcClient.hpp"
#include <iostream>
#include <thread>

int main() {
    IrcClient client;

    // Connect to server (replace with your server's port/password)
    if (!client.connectToServer("127.0.0.1", 6667)) {
        std::cerr << "Connection failed!\n";
        return 1;
    }

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Authenticate
    client.sendCommand("PASS pass\r\n");
    client.sendCommand("NICK mynick\r\n");
    client.sendCommand("USER myuser 0 * :Real Name\r\n");

    // Main loop: listen for server responses
    while (true) {
        client.listenForResponse(); // Prints server messages
    }

    return 0;
}