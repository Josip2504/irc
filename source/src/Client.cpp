#include "../inc/Client.hpp"
#include <sys/socket.h>
#include <sys/types.h>

Client::Client(int fd, int port, std::string ip) :
	_fd(fd), _port(port), _ip(ip), _authenticated(false), _state(ClientState::Connected), _lounge(nullptr),
	_username(""), _hostname(""), _realname(""), _nickname("")
{
	std::cout << "Client created" << std::endl;
}

Client::~Client()
{
	if(_fd != 0)
		close(_fd);
	std::cout << "Removed Client" << std::endl;
}

void Client::send(const std::string &msg)
{
	int64_t bytes_send = ::send(_fd, msg.c_str(), msg.length(), 0);
	if (bytes_send == -1){
		std::cerr << "send failed !" << std::endl;
	}
}

void Client::on_read(){
	char buffer[1024];								//  | 
	int bytes_read = recv(_fd, buffer, sizeof(buffer) - 1, 0); // leave space for null tearmiantor by adding "- 1"
	if (bytes_read <= 0){
		_state = (ClientState::Disconnected);
		return ;
	}
	buffer[bytes_read] = '\0';

	// std::string tempz(buffer, bytes_read); ----------
	// _input_buffer += tempz;

	_input_buffer += buffer;

	size_t pos;
	//while((pos = _input_buffer.find("\n")) != std::string::npos) // recreate that it takes \r\n
	//{
	//	std::string line = _input_buffer.substr(0, pos);	----------------
	//	_input_buffer.erase(0, pos + 2);
	//	_message_list.push(line);
	//}
	while ((pos = _input_buffer.find("\r\n")) != std::string::npos) {
		std::string line = _input_buffer.substr(0, pos);
		_input_buffer.erase(0, pos + 2); // Remove \r\n
		_message_list.push(line);
	}
}

bool Client::is_disconnected()
{
	if (_state == ClientState::Disconnected)
		return true;
	return false;
}

bool Client::has_message()
{
	if (!_message_list.empty())
		return true;
	return false;
}

std::string Client::next_message()
{
	std::string next_line = _message_list.front();
	_message_list.pop();

	return next_line;
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

		ClientState Client::get_state() const {
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

		std::queue<std::string> Client::get_message_list() const{
			return this->_message_list;
		}
		
//_SET
		//. . .