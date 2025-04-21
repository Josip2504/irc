#pragma once

#include "./Utility.hpp"
#include "./Client.hpp"

class Client;

//NEED TO CREATE :
//	LOUNGE

class Server
{
	private:
		int _port;
		int _listen_fd;
		std::string _passwd;
		std::map<int, Client> _clients;
		bool _run;
		std::vector<pollfd> _pfds;

		void poll_loop();
		void create_connection();
		void handle_message();

		public: //MAIN
		Server(int port, const std::string &pass);
		~Server();
		void run();

	public: //UTILITY
		std::map<int, Client> get_clients( void ) const;
		int	get_listen_fd( void ) const;
};
