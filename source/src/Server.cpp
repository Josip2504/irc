#include "../inc/Server.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <sstream>

//TODO: add authentication status and states in header

/* We add fcntl here mentioned in PDF because it shouldnt block
*  The Server will check with poll if any FD is ready to be read 
*	then it will read the fd and process it to not get stuch in execution somewhere
*	with poll the server only works when there is something to do
*/

//_MAIN_
Server::Server(int port, const std::string &pass) : _port(port), _passwd(pass)
{
	(void)_port;
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
	_pfds.push_back(pfd); //maybe add main pfd checks

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
			auto &pfd = _pfds[i];


			if (pfd.revents & POLLIN)
			{
				if (pfd.fd == _listen_fd)
					create_connection();
				else
				{
					Client &cli = _clients.at(pfd.fd);
					cli.on_read();
					if (cli.is_disconnected()){
						remove_client(pfd.fd);
						break;
					}

					while(cli.has_message()){
						std::string line = cli.next_message();
						handle_message(cli, line);
					}
				}
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

	_clients.try_emplace(new_fd, new_fd, new_port, new_ip);
	std::cout << _clients.at(new_fd) << std::endl;

	pollfd new_pfd;
	new_pfd.fd = new_fd;
	new_pfd.events = POLLIN;
	_pfds.push_back(new_pfd);
	
	std::cout << "WE HAVE A NEW CONNECTION" << std::endl;//LOG::
}



			// jsamardz - moved handle_massage down below



/*void Server::handle_message(Client &cli, std::string &line) // PARSING PART
{
	//(void)cli;// Parsing part here, Tokenizing, Function forwarding, Responding . . .
	//(void)line;
	//std::cout << cli.get_username() << " -log > [" << line << "]" << std::endl;
} */

/*void Server::handle_message(int fd) OLD Structure
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
	Client &client = _clients.at(fd);
	Lounge new_lounge();
	//broadcast();

	std::cout << _clients.at(fd).get_username() << " -log > [" << buffer << "]" << std::endl;
}*/

void Server::get_or_make_lounge(std::string &name){
	auto it = _lounges.find(name);
	if (it != _lounges.end()){
		std::cout << "lounge already created" << std::endl;
		return ;
	}

	_lounges.try_emplace(name, name);
	std::cout << "lounge " << name << " created" << std::endl;
}

void Server::remove_client(int fd)
{
	auto it = _clients.find(fd);
	if (it == _clients.end())
		return ;

	if (it->second.get_lounge() != nullptr)
		it->second.get_lounge()->remove_client(&it->second);
	
	_clients.erase(it);

	close(fd);
	for (auto it = _pfds.begin(); it != _pfds.end(); ++it) {
		if (it->fd == fd) {
			_pfds.erase(it);
			break;
		}
	}
	
	std::cout << "Client " << fd << " disconnected.";
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

		// jsamardz new handle msg

void Server::handle_message(Client &cli, std::string &line) // PARSING PART
{
	std::istringstream iss(line);
	std::string cmd;
	iss >> cmd;

	if (cmd == "PASS") {
		handle_pass(cli, iss);
	} 
	else if (cmd == "NICK") {
		handle_nick(cli, iss);
	} 
	else if (cmd == "USER") {
		handle_user(cli, iss);
	}
	else if (cmd == "JOIN") {
		handle_join(cli, iss);
	}
	else if (cmd == "PART") {
		handle_part(cli, iss);
	}
	else if (cmd == "PRIVMSG") {
		handle_privmsg(cli, iss);
	}
	else if (cmd == "NAMES") {
		handle_names(cli, iss);
	}
	else if (cmd == "KICK") {
		handle_kick(cli, iss);
	}
	else if (cmd == "INVITE") {
		handle_invite(cli, iss);
	}
	else if (cmd == "TOPIC") {
		handle_topic(cli, iss);
	}
	else if (cmd == "MODE") {
		handle_mode(cli, iss);
	}
	else {
		if (!cli.is_registered()) {
			cli.send("ERROR :Register first (PASS/NICK/USER)\r\n");
		}
		else {
			cli.send("ERROR :Unknown command\r\n");
		}
	}
}