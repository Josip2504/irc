#include "../inc/Server.hpp"
#include <stdexcept>


/* We add fcntl here mentioned in PDF because it shouldnt block
*  The Server will check with poll if any FD is ready to be read 
*	then it will read the fd and process it to not get stuch in execution somewhere
*	with poll the server only works when there is something to do
*/
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
					handle_message();
			}
		}
	}
}

void Server::create_connection() // TODO:
{
	std::cout << "WE HAVE A NEW CONNECTION" << std::endl;
}

void Server::handle_message() // TODO:
{
	std::cout << "WE HAVE A NEW MESSAGE" << std::endl;
}

//__UTILITY__//
std::map<int, Client> Server::get_clients( void ) const{
	return this->_clients;
}

int	Server::get_listen_fd( void ) const{
	return this->_listen_fd;
}