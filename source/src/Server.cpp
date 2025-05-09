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


	//jsamardz commands part below

void Server::handle_pass(Client &cli, std::istringstream &iss) {
	std::string password;
	iss >> password;

	if (password.empty()) {
		cli.send("ERROR :Password required\r\n");
	} 
	else if (password != _passwd) {
		cli.send("ERROR :Invalid password\r\n");
		remove_client(cli.get_fd());
	} 
	else {
		cli.set_state(ClientState::Authenticated);
		cli.send(":server 001 :Password accepted\r\n");
	}
}

void Server::handle_nick(Client &cli, std::istringstream &iss) {
	std::string nick;
	iss >> nick;

	if (nick.empty()) {
		cli.send("ERROR :No nickname given\r\n");
		return;
	}

	// Check if nickname is already in use
	for (auto &[fd, client] : _clients) {
		if (client.get_nickname() == nick) {
			cli.send("ERROR :Nickname already in use\r\n");
			return;
		}
	}

	cli.set_nick(nick);
	cli.send(":server 001 :Nickname set to " + nick + "\r\n");

	// Autoregister if USER was already set
	if (!cli.get_username().empty() && cli.is_authenticated()) {
		cli.set_state(ClientState::Registered);
		cli.send(":server 001 :Registration complete\r\n");
	}
}

void Server::handle_user(Client &cli, std::istringstream &iss) {
	std::string username, hostname, servername, realname;
	iss >> username >> hostname >> servername;
	std::getline(iss, realname); // Realname may contain spaces

	if (username.empty() || hostname.empty() || realname.empty()) {
		cli.send("ERROR :USER syntax: USER <username> <hostname> <servername> :<realname>\r\n");
		return;
	}

	cli.set_username(username);
	cli.set_hostname(hostname);
	cli.set_realname(realname.substr(1)); // Remove leading ':'

	// Auto-register if NICK was already set
	if (!cli.get_nickname().empty() && cli.is_authenticated()) {
		cli.set_state(ClientState::Registered);
		cli.send(":server 001 :Registration complete. Welcome!\r\n");
	}
}

void Server::handle_join(Client &cli, std::istringstream &iss) {
	if (!cli.is_registered()) {
		cli.send("ERROR :Register first\r\n");
		return;
	}

	std::string channel_name;
	iss >> channel_name;

	if (channel_name.empty() || channel_name[0] != '#') {
		cli.send("ERROR :Invalid channel name (use #name)\r\n");
		return;
	}

	// Remove '#' prefix
	std::string lounge_name = channel_name.substr(1);
	// Create channel if it doesn't exist
	auto [it, created] = _lounges.try_emplace(lounge_name, lounge_name);
	Lounge &lounge = it->second;

	if (lounge.add_client(&cli, "")) { // No password for now
		cli.set_lounge(&lounge);
		
		lounge.broadcast(":" + cli.get_nickname() + " JOIN " + channel_name + "\r\n", nullptr);

		if (!lounge.get_topic().empty()) {
			cli.send(":" + cli.get_nickname() + " TOPIC " + channel_name + " :" + lounge.get_topic() + "\r\n");
		}
	} else {
		cli.send("ERROR :Cannot join channel\r\n");
	}
}

void Server::handle_part(Client &cli, std::istringstream &iss) {
	std::string channel_name;
	iss >> channel_name;

	if (channel_name.empty() || channel_name[0] != '#') {
		cli.send("ERROR :Invalid channel name\r\n");
		return;
	}

	std::string lounge_name = channel_name.substr(1);
	auto it = _lounges.find(lounge_name);

	if (it != _lounges.end() && it->second.is_member(&cli)) {
		it->second.broadcast(":" + cli.get_nickname() + " PART " + channel_name + "\r\n", nullptr);
		it->second.remove_client(&cli);
		cli.set_lounge(nullptr);

		// Cleanup
		if (it->second.is_empty()) {
			_lounges.erase(it);
		}
	} else {
		cli.send("ERROR :Not in channel\r\n");
	}
}

void Server::handle_privmsg(Client &cli, std::istringstream &iss) {
	std::string target, message;
	iss >> target;
	std::getline(iss, message);

	if (target.empty() || message.empty()) {
		cli.send("ERROR :Usage: PRIVMSG <#channel|nick> :<message>\r\n");
		return;
	}

	if (target[0] == '#') {
		// Channel message
		std::string lounge_name = target.substr(1);
		auto it = _lounges.find(lounge_name);
		
		if (it != _lounges.end() && it->second.is_member(&cli)) {
			it->second.broadcast(":" + cli.get_nickname() + " PRIVMSG " + target + message + "\r\n", &cli);
		} else {
			cli.send("ERROR :Cannot send to channel\r\n");
		}
	} else {
		// Private message
		for (auto &[fd, client] : _clients) {
			if (client.get_nickname() == target) {
				client.send(":" + cli.get_nickname() + " PRIVMSG " + target + message + "\r\n");
				return;
			}
		}
		cli.send("ERROR :User not found\r\n");
	}
}

void Server::handle_names(Client &cli, std::istringstream &iss) {
	std::string channel_name;
	iss >> channel_name;

	if (channel_name.empty() || channel_name[0] != '#') {
		cli.send("ERROR :Invalid channel\r\n");
		return;
	}

	std::string lounge_name = channel_name.substr(1);
	auto it = _lounges.find(lounge_name);

	if (it != _lounges.end()) {
		std::string names_list;
		for (Client *member : it->second.get_clients()) {
			if (it->second.is_operator(member)) {
				names_list += "@";
			}
			names_list += member->get_nickname() + " ";
		}
		cli.send(":server 353 " + cli.get_nickname() + " = " + channel_name + " :" + names_list + "\r\n");
		cli.send(":server 366 " + cli.get_nickname() + " " + channel_name + " :End of NAMES list\r\n");
	} else {
		cli.send("ERROR :Channel doesn't exist\r\n");
	}
}

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
	else {
		if (!cli.is_registered()) {
			cli.send("ERROR :Register first (PASS/NICK/USER)\r\n");
		}
	// TODO other commands
	}
}