#pragma once

#include "./Utility.hpp"
#include "./Client.hpp"
#include "./Lounge.hpp"
#include <string>

class Client;

class Server
{
	private:
		int _port;
		int _listen_fd;
		std::string _passwd;
		std::map<int, Client> _clients;
		std::vector<pollfd> _pfds;
		std::map<std::string, Lounge> _lounges;

	private: //MEMBERS
		void poll_loop();
		void create_connection();
		void handle_message(Client &cli, std::string &line);
		void get_or_make_lounge(std::string &name);
		void remove_client(int fd);

		//void tokenize(); calls the according operation if match
		// operations

	public: //MAIN
		Server(int port, const std::string &pass);
		~Server();
		void run();

	public: //UTILITY
		std::map<int, Client> get_clients( void ) const;
		int	get_listen_fd( void ) const;
		std::map<std::string, Lounge> get_lounges() const;

	public: //jsamardz
		void handle_pass(Client &cli, std::istringstream &iss);
		void handle_nick(Client &cli, std::istringstream &iss);
		void handle_user(Client &cli, std::istringstream &iss);
};
