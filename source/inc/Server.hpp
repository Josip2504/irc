#pragma once

#include "./Utility.hpp"

class Client;

//NEED TO CREATE :
//	CLIENT
//	CHANNEL

class Server
{
	public: //MAIN
		Server(int port, const std::string &pass);
		~Server();
		void run();
	
	public: //UTILITY
		std::map<int, Client> get_clients( void ) const;
		int	get_listen_fd( void ) const;
		
	private:
		int port;
		std::string passwd;
		int listen_fd;
		std::map<int, Client> clients;
};
