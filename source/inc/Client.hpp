#pragma once

#include "./Utility.hpp"
#include "./Lounge.hpp"

class Lounge;

enum class ClientState {
	Connected,
	Authenticated,
	Registered,
	Disconnected
};

class Client
{
	private:
		int _fd;
		int _port;
		std::string _ip;

		bool _authenticated;
		ClientState _state;
		Lounge *_lounge;

		std::string _username;
		std::string _hostname;
		std::string _realname;
		std::string _nickname;

		std::string _input_buffer;
		std::queue<std::string>_message_list;

	public://MAIN
		Client(int fd, int port, std::string ip);
		~Client();
	
	public: //MEMBER
		void send(const std::string &msg);
		
		void on_read();
		bool is_disconnected();

		bool has_message();
		std::string next_message();


	public: //UTILITY
		int get_fd() const ;
		int get_port() const ;
		std::string get_ip()const ;

		bool get_authenticated() const ;
		ClientState get_state() const ;
		Lounge *get_lounge() const ;

		std::string get_username() const ;
		std::string get_hostname() const ;
		std::string get_realname() const ;
		std::string get_nickname() const ;

		std::string get_input_buffer() const ;
		std::queue<std::string> get_message_list() const;

};

std::ostream &operator<<(std::ostream &os, const Client &client);