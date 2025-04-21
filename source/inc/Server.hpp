#pragma once

#include "./Utility.hpp"

class Client;

//NEED TO CREATE :
//	CLIENT
//	CHANNEL

class Server
{
	private:
	int _port;
	int _listen_fd;
	std::string _passwd;
	std::map<int, Client> _clients;

	public: //MAIN
		Server(int port, const std::string &pass);
		~Server();
		void run();
	
	public: //UTILITY
		std::map<int, Client> get_clients( void ) const;
		int	get_listen_fd( void ) const;
};
