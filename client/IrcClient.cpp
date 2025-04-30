#include "IrcClient.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>

IrcClient::IrcClient() {
	_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket_fd == -1)
		throw std::runtime_error("Failed to create socket");
	fcntl(_socket_fd, F_SETFL, O_NONBLOCK); // non blocking mode
}

IrcClient::~IrcClient() { disconnect(); }

bool IrcClient::connectToServer(const std::string &host, uint16_t port) {
	_server_addr.sin_family = AF_INET;
	_server_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, host.c_str(), &_server_addr.sin_addr) <= 0) {
		std::cerr << "Invalid address" << std::endl;
		return false;
	}

		// non blocking socket returns "EINPROGRESS"
	if (connect(_socket_fd, (struct sockaddr*)&_server_addr, sizeof(_server_addr)) == -1) {
		if (errno != EINPROGRESS) {
			std::cerr << "Connection failed" << std::endl;
			return false;
		}
	}

		// setup poll
	_pollfds.push_back({_socket_fd, POLLIN | POLLOUT, 0});
	return true;
}

void IrcClient::disconnect() {
	if (_socket_fd != -1) {
		close (_socket_fd);
		_socket_fd = -1;
	}
	_pollfds.clear();
}

void IrcClient::sendCommand(const std::string &command) {
	if (!isConnected())
		return ;
	std::string msg = command + "\r\n";
	send(_socket_fd, command.c_str(), command.size(), 0);
}

void IrcClient::listenForResponse() {
	if (_pollfds.empty())
		return ;
	
	int ret = poll(_pollfds.data(), _pollfds.size(), 100);
	if (ret <= 0)
		return ;

	for (const auto &pfd : _pollfds) {
		if (pfd.events & POLLIN) {
			char	buffer[1024];
			ssize_t	bytes = recv(pfd.fd, buffer, sizeof(buffer), 0);
			if (bytes > 0) {
				std::string response(buffer, bytes);
				std::cout << "Server: " << response <<std::endl;
			}
		}
	}
}