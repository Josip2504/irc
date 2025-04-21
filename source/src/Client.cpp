#include "../inc/Client.hpp"

Client::Client(int fd, int port, std::string ip) :
	_fd(fd), _port(port), _ip(ip), _authenticated(false), _state(0), _lounge(nullptr),
	_username(""), _hostname(""), _realname(""), _nickname("")
{
	std::cout << "Client created" << std::endl;

}

Client::~Client()
{

}

std::ostream &operator<<(std::ostream &os, const Client &client)
{
	os	<< "[> nickname = " << client.get_nickname() 
		<< " | ip = " << client.get_ip()
		<< " | port = " << client.get_port()
		<< " | fd = " << client.get_fd()
		<< " | lounge = " << client.get_lounge() 
		<< " ]";
	return os;
}

//_GET
		int Client::get_fd() const {
			return _fd;
		}

		int Client::get_port() const {
			return _port;
		}

		std::string Client::get_ip() const {
			return _ip;
		}
		
		bool Client::get_authenticated() const {
			return _authenticated;
		}

		int Client::get_state() const {
			return _state;
		}

		Lounge *Client::get_lounge() const {
			return _lounge;
		}

		std::string Client::get_username() const{
			return _username;
		}

		std::string Client::get_hostname() const{
			return _hostname;
		}

		std::string Client::get_realname() const{
			return _realname;
		}

		std::string Client::get_nickname() const{
			return _nickname;
		}

		std::string Client::get_input_buffer() const {
			return _input_buffer;
		}

//_SET
		//. . .