#include "../inc/Server.hpp"
#include <fcntl.h>
#include <stdexcept>

//TODO: add authentication status and states in header

/* We add fcntl here mentioned in PDF because it shouldnt block
*  The Server will check with poll if any FD is ready to be read 
*	then it will read the fd and process it to not get stuch in execution somewhere
*	with poll the server only works when there is something to do
*/

//_MAIN_
Server::Server(int port, const std::string &pass) : _port(port), _passwd(pass)
{
	if ((this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == - 1)
		throw std::runtime_error("Could not create Socket");
	
	int opt = 1; // can be toggled to 0 to turn off this option, if needed can be stored

	if (setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
		close(_listen_fd);
		throw std::runtime_error("Could not set Socket options");
	}

	//check for best checks to do here dont forget to close fd in case of error
	fcntl(_listen_fd, F_SETFL, O_NONBLOCK);

	// needs to be configured one time only the OS remebers the settings
	struct sockaddr_in addr;
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(_listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1){
		close(_listen_fd);
		throw std::runtime_error("Could not set bind socket to FD");
	}

	if (listen(_listen_fd, SOMAXCONN) == -1){
		close(_listen_fd);
		throw std::runtime_error("Couldn't bind the port/FD to listening");
	}
	std::cout << "Server listens succesful on fd = " << _listen_fd << std::endl;
}

Server::~Server()
{
	close(_listen_fd);
}

void Server::run()
{
	//set signals
	
	pollfd pfd;
	pfd.fd = _listen_fd;
	pfd.events = POLLIN;
	_pfds.push_back(pfd);

	poll_loop();
	
	//unset signals
}

//_MEMBER_
void Server::poll_loop()
{
	while(!(1 != 1))// add _running check 
	{
		int pollz; // TODO:

		if((pollz = poll(_pfds.data(), _pfds.size(), -1)) == -1)
			throw std::runtime_error("poll failed");

		for (size_t i = 0; i < _pfds.size(); ++i)
		{
			if (_pfds[i].revents & POLLIN)
			{
				if (_pfds[i].fd == _listen_fd)
					create_connection();
				else
					handle_message(_pfds[i].fd);
			}
		}
	}
}

void Server::create_connection()
{
	sockaddr_in new_client;
	socklen_t client_len = sizeof(new_client);
	
	int new_fd = accept(_listen_fd, (sockaddr *)&new_client, &client_len);
	if (new_fd == -1){//maybe add throw return to keep server running
		std::cerr << "failed to get client new_fd" << std::endl;
		return ;
	}

	fcntl(new_fd, F_SETFL, O_NONBLOCK);

	std::string new_ip = inet_ntoa(new_client.sin_addr);
	int new_port = ntohs(new_client.sin_port);

	_clients.emplace(new_fd, Client(new_fd, new_port, new_ip));
	std::cout << _clients.at(new_fd) << std::endl;

	pollfd new_pfd;
	new_pfd.fd = new_fd;
	new_pfd.events = POLLIN;
	_pfds.push_back(new_pfd);
}

void Server::handle_message(int fd)
{	
	char buffer[512];
	int bytes_read = recv(fd, buffer, 511, 0);
	buffer[bytes_read -1] = '\0'; // we cut out the /n char

	if (bytes_read <= 0){
		std::cout << "there was a problem to receive msg from fd = " << fd <<  std::endl;
		close(fd);
		//need to delete client
		return ;
	}

	// Parsing part here, Tokenizing, Function forwarding, Responding . . .
	// Put all onto Server Class

	//TESTING FOR LOUNGE
	//. . .
	Client &client = _clients.at(fd);
	//broadcast();


	std::cout << _clients.at(fd).get_username() << " > [" << buffer << "]" << std::endl;
}

//_UTILITY_
std::map<int, Client> Server::get_clients( void ) const{
	return this->_clients;
}

int	Server::get_listen_fd( void ) const{
	return this->_listen_fd;
}

std::map<std::string, Lounge> Server::get_lounges() const{
	return this->_lounges;
}
