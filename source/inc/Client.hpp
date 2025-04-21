#pragma once

#include "./Utility.hpp"

class Lounge;

class Client
{
	private:
		int _fd;
		int _port;
		std::string _ip;

		bool _authenticated;
		int _state;
		Lounge *_lounge;

		std::string _username;
		std::string _hostname;
		std::string _realname;
		std::string _nickname;

		std::string _input_buffer;

	public:
		Client(int fd, int port, std::string ip);
		~Client();
	
	
	public:
		int get_fd() const ;
		int get_port() const ;
		std::string get_ip()const ;

		bool get_authenticated() const ;
		int get_state() const ;
		Lounge *get_lounge() const ;

		std::string get_username() const ;
		std::string get_hostname() const ;
		std::string get_realname() const ;
		std::string get_nickname() const ;

		std::string get_input_buffer() const ;

};

std::ostream &operator<<(std::ostream &os, const Client &client);