#pragma once

#include <map>

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
	
	private:
		int listen_fd;
		std::map<int, Client> clients;
};
