#ifndef IRCCLIENT_HPP
#define IRCCLIENT_HPP

#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

class IrcClient {
	private:
		std::string			_nickname;
		std::string			_username;
		struct sockaddr_in	_server_addr{};
		int					_socket_fd{-1};
			// -1 for not connected

		std::vector<pollfd>	_pollfds;

	public:
		IrcClient();
		~IrcClient();

		// getters
		bool				isConnected() const { return _socket_fd != -1; }
		std::string_view	getNickname() const { return _nickname; }

		// functions
		bool				connectToServer(const std::string &host, uint16_t port);
		void				disconnect();
		void				sendCommand(const std::string &command);
		void				listenForResponse();
};

#endif