#include "../inc/Server.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <sstream>


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
