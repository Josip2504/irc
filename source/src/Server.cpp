#include "../inc/Server.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <csignal>
#include <sstream>

bool Server::_sig = true;

/* We add fcntl here mentioned in PDF because it shouldnt block
*  The Server will check with poll if any FD is ready to be read 
*	then it will read the fd and process it to not get stuch in execution somewhere
*	with poll the server only works when there is something to do
*/

//_MAIN_
Server::Server(int port, const std::string &pass) :  _run(true), _port(port), _passwd(pass)
{
	if ((this->_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == - 1)
		throw std::runtime_error(std::string("Could not create socket: ") + strerror(errno));
	
	int opt = 1;

	if (setsockopt(_listen_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) == -1){
		close(_listen_fd);
		throw std::runtime_error(std::string("Could not set socket options: ") + strerror(errno));
	}

	//check for best checks to do here dont forget to close fd in case of error
	if (fcntl(_listen_fd, F_SETFL, O_NONBLOCK) == -1){
		close(_listen_fd);
		throw std::runtime_error(std::string("Could not set socket to non-blocking: ") + strerror(errno));
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(_listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1){
		close(_listen_fd);
		throw std::runtime_error(std::string("Could not bind socket: ") + strerror(errno));
	}

	if (listen(_listen_fd, SOMAXCONN) == -1){
		close(_listen_fd);
		throw std::runtime_error(std::string("Could not listen on socket: ") + strerror(errno));
	}
	std::cout << "Server listens succesful on fd = " << _listen_fd << std::endl;
}

Server::~Server()
{
	close(_listen_fd);
}

void Server::run()
{	
	pollfd pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = _listen_fd;
	pfd.events = POLLIN;
	_pfds.push_back(pfd);

	poll_loop();
}

//_MEMBER_
void Server::poll_loop()
{
	while(_sig == true) 
	{

		for (auto& pfd : _pfds){
			pfd.revents = 0;
		}

		int pollz;

		if((pollz = poll(_pfds.data(), _pfds.size(), -1)) == -1){
			if (errno == EINTR)
				continue;
			throw std::runtime_error(std::string("poll() failed: ") + strerror(errno));
		}

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
						if (cli.is_disconnected()){
							remove_client(pfd.fd);
							i--;
							break;
						}
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
	if (new_fd == -1){
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return ;
		else {
			std::cerr << "accept() error: " << strerror(errno) << std::endl;
			return ;
		}
	}

	fcntl(new_fd, F_SETFL, O_NONBLOCK);

	std::string new_ip = inet_ntoa(new_client.sin_addr);
	int new_port = ntohs(new_client.sin_port);

	_clients.try_emplace(new_fd, new_fd, new_port, new_ip);
	std::cout << _clients.at(new_fd) << std::endl;

	pollfd new_pfd;
	memset(&new_pfd, 0, sizeof(new_pfd));
	new_pfd.fd = new_fd;
	new_pfd.events = POLLIN;
	_pfds.push_back(new_pfd);
}

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

	Lounge * lounge = it->second.get_lounge();
	if (lounge != nullptr){
		lounge->remove_client(&it->second);
		if (lounge->is_empty())
		_lounges.erase(lounge->get_name());
	}
	
	for (auto it = _pfds.begin(); it != _pfds.end(); ++it) {
		if (it->fd == fd) {
			_pfds.erase(it);
			break;
		}
	}
	
	close(fd);
	_clients.erase(it);

	std::cout << "Client " << fd << " disconnected.";
}

void Server::shutdown()
{
	std::cout << "shutting down. . ." << std::endl;
	
	std::vector<int> open_fds;
	open_fds.reserve(_clients.size());
	for (auto &cl : _clients){
		open_fds.push_back(cl.first);
	}
	for(int fd : open_fds){
		remove_client(fd);
	}
	
	_lounges.clear();
	
	if (_listen_fd != -1){
		close(_listen_fd);
		_listen_fd = -1;
	}

	_pfds.clear();
	_clients.clear();

	std::cout << "Cleanup done." << std::endl;
}

void Server::init_signals()
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = Server::stop_run;
	signal(SIGINT, Server::stop_run);
	signal(SIGQUIT, Server::stop_run);
}

void Server::stop_run(const int signum)
{
	(void)signum;
	Server::_sig = false;
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

	if (cmd.size() > 100) {
		cli.send("ERROR :Command too long\r\n");
		return ;
	}
	
	if (cmd == "CAP") {
		cli.send("CAP * LS :\r\n");
		cli.send("CAP * ACK :\r\n");
	}
	else if (cmd == "PASS") {
		handle_pass(cli, iss);
		return ;
	} 
	else if (cmd == "NICK") {
		handle_nick(cli, iss);
		return ;
	} 
	else if (cmd == "USER") {
		handle_user(cli, iss);
		return ;
	}
	else if (cmd == "PING") {
		std::string token;
		iss >> token;
		if (token.empty())
			cli.send("ERROR :Missing ping token\r\n");
		else
			cli.send("PONG " + token + "\r\n");
		return ;
	}

	if (cli.is_registered()) {
		if (cmd == "JOIN") {
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
			if (cmd == "WHOIS")
				return ;
			cli.send("ERROR :Unknown command\r\n");
		}
	}
}

void Server::attempt_register(Client &cli){
	if (cli.get_state() == ClientState::Authenticated &&
		!cli.get_nickname().empty() &&
		!cli.get_username().empty() &&
		!cli.get_realname().empty())
	{
		cli.set_state(ClientState::Registered);
		cli.send(":server 001 " + cli.get_nickname() + " :Welcome to the IRC server\r\n");
	}
}
