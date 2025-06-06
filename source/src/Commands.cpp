#include "../inc/Server.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <sstream>


void Server::handle_pass(Client &cli, std::istringstream &iss) {
	std::string password;
	iss >> password;

	if (password.empty()) {
        cli.send(":server 461 * PASS :Not enough parameters\r\n");
	} 
	else if (password != _passwd) {
        cli.send(":server 464 * :Password incorrect\r\n");
		cli.set_state(ClientState::Disconnected);
	} 
	else {
		cli.set_state(ClientState::Authenticated);
		//cli.send(":server 001 :Password accepted\r\n");
		attempt_register(cli);
	}
}

void Server::handle_nick(Client &cli, std::istringstream &iss) {
	std::string nick;
	iss >> nick;

	if (nick.empty()) {
		cli.send(":server 431 * :No nickname given\r\n");
		return;
	}

	// Check if nickname is already in use
	for (auto &[fd, client] : _clients) {
		if (client.get_nickname() == nick) {
			std::string reply = ":server 433 * "+ nick + " :Nickname is already in use\r\n";
			cli.send(reply);
			return;
		}
	}

	cli.set_nick(nick);
	cli.send(":server 001 :Nickname set to " + nick + "\r\n");

	// Autoregister if USER was already set
//	if (!cli.get_username().empty() && cli.is_authenticated()) {
//		cli.set_state(ClientState::Registered);
//		cli.send(":server 001 :Registration complete\r\n");
//	}
	attempt_register(cli);
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
//	if (!cli.get_nickname().empty() && cli.is_authenticated()) {
//		cli.set_state(ClientState::Registered);
//		cli.send(":server 001 :Registration complete. Welcome!\r\n");
//	}
	attempt_register(cli);
}

void Server::handle_join(Client &cli, std::istringstream &iss) {
	if (!cli.is_registered()) {
		cli.send("ERROR :Register first\r\n");
		return;
	}

	std::string channel_name, key;
	iss >> channel_name;
	iss >> key;

	if (channel_name.empty() || channel_name[0] != '#') {
		cli.send("ERROR :Invalid channel name (use #name)\r\n");
		return;
	}

	std::string lounge_name = channel_name.substr(1);

	auto [it, created] = _lounges.try_emplace(lounge_name, lounge_name);
	Lounge &lounge = it->second;

	if (lounge.is_invite_only() && !lounge.is_invited(&cli)) {
		cli.send(":server 473 " + cli.get_nickname() + " " + channel_name + " :Cannot join (+i)\r\n");
		return;
	}
	else if (!lounge.get_key().empty() && lounge.get_key() != key) {
		cli.send(":server 475 " + cli.get_nickname() + " " + channel_name + " :Bad channel key\r\n");
		return;
	}
	else if (lounge.get_limit() > 0 && lounge.get_client_count() >= static_cast<size_t>(lounge.get_limit())) {
		cli.send(":server 471 " + cli.get_nickname() + " " + channel_name + " :Channel is full (+l)\r\n");
		return;
	}

	// join logic
	if (lounge.add_client(&cli, key)) {
		cli.set_lounge(&lounge);
		
		lounge.broadcast(":" + cli.get_nickname() + " JOIN " + channel_name + "\r\n", nullptr);

		if (!lounge.get_topic().empty()) {
			cli.send(":" + cli.get_nickname() + " TOPIC " + channel_name + " :" + lounge.get_topic() + "\r\n");
		}

		std::string names_list;
		for (Client *member : lounge.get_clients()) {
			if (lounge.is_operator(member)) {
				names_list += "@";
			}
			names_list += member->get_nickname() + " ";
		}
		cli.send(":server 353 " + cli.get_nickname() + " = " + channel_name + " :" + names_list + "\r\n");
		cli.send(":server 366 " + cli.get_nickname() + " " + channel_name + " :End of /NAMES list\r\n");
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

void Server::handle_kick(Client &cli, std::istringstream &iss) {
	std::string channel, target, reason;
	iss >> channel >> target;
	std::getline(iss, reason); // Optional reason

	if (channel.empty() || channel[0] != '#') {
		cli.send("ERROR :Invalid channel\r\n");
		return;
	}

	std::string lounge_name = channel.substr(1);
	auto lounge_it = _lounges.find(lounge_name);

	if (lounge_it == _lounges.end()) {
		cli.send("ERROR :Channel doesn't exist\r\n");
		return;
	}

	Lounge &lounge = lounge_it->second;

	if (!lounge.is_operator(&cli)) {
		cli.send("ERROR :You're not a channel operator\r\n");
		return;
	}

	for (auto &[fd, client] : _clients) {
		if (client.get_nickname() == target && lounge.is_member(&client)) {
			lounge.broadcast(":" + cli.get_nickname() + " KICK " + channel + " " + target + reason + "\r\n", nullptr);
			lounge.remove_client(&client);
			client.set_lounge(nullptr);
			return;
		}
	}

	cli.send("ERROR :User not in channel\r\n");
}

void Server::handle_invite(Client &cli, std::istringstream &iss) {
	std::string target, channel;
	iss >> target >> channel;

	if (channel.empty() || channel[0] != '#') {
		cli.send("ERROR :Invalid channel\r\n");
		return;
	}

	std::string lounge_name = channel.substr(1);
	auto lounge_it = _lounges.find(lounge_name);

	if (lounge_it == _lounges.end()) {
		cli.send("ERROR :Channel doesn't exist\r\n");
		return;
	}

	Lounge &lounge = lounge_it->second;

	if (!lounge.is_operator(&cli)) {
		cli.send("ERROR :You're not a channel operator\r\n");
		return;
	}

	for (auto &[fd, client] : _clients) {
		if (client.get_nickname() == target) {
			lounge.invite(&client);
			client.send(":" + cli.get_nickname() + " INVITE " + target + " " + channel + "\r\n");
			cli.send(":server 341 " + cli.get_nickname() + " " + target + " " + channel + "\r\n");
			return;
		}
	}

	cli.send("ERROR :User not found\r\n");
}

void Server::handle_topic(Client &cli, std::istringstream &iss) {
	std::string channel, new_topic;
	iss >> channel;
	std::getline(iss, new_topic);

	if (channel.empty() || channel[0] != '#') {
		cli.send("ERROR :Invalid channel\r\n");
		return;
	}

	std::string lounge_name = channel.substr(1);
	auto lounge_it = _lounges.find(lounge_name);

	if (lounge_it == _lounges.end()) {
		cli.send("ERROR :Channel doesn't exist\r\n");
		return;
	}

	Lounge &lounge = lounge_it->second;

	if (new_topic.empty()) {
		// GET topic
		cli.send(":server 332 " + cli.get_nickname() + " " + channel + " :" + lounge.get_topic() + "\r\n");
	} else {
		// SET topic
		if (lounge.is_topic_restricted() && !lounge.is_operator(&cli)) {
			cli.send("ERROR :You need operator privileges\r\n");
		} else {
			lounge.set_topic(new_topic.substr(1), &cli); // Remove leading space
			lounge.broadcast(":" + cli.get_nickname() + " TOPIC " + channel + " :" + new_topic.substr(1) + "\r\n", nullptr);
		}
	}
}

void Server::handle_mode(Client &cli, std::istringstream &iss) {
	std::string target, mode, arg;
	iss >> target >> mode >> arg;

	if (target.empty()) {
		cli.send("ERROR :No target\r\n");
		return;
	}

	if (target[0] != '#') {
        // Handle user modes
		if (target[0 == "Nickname"])
			return ;
        if (target != cli.get_nickname()) {
            cli.send("ERROR :Can only change your own modes\r\n");
            return;
        }

        if (mode == "+i") {
            cli.set_inv(true);
            cli.send(":" + cli.get_nickname() + " MODE " + target + " +i\r\n");
        }
        else if (mode == "-i") {
            cli.set_inv(false);
            cli.send(":" + cli.get_nickname() + " MODE " + target + " -i\r\n");
        }
        else {
            cli.send("ERROR :Unknown user mode\r\n");
        }
        return;  // Important: Return after handling user mode
	}

	std::string lounge_name = target.substr(1);
	auto lounge_it = _lounges.find(lounge_name);

	if (lounge_it == _lounges.end()) {
		cli.send("ERROR :Channel doesn't exist\r\n");
		return;
	}

	Lounge &lounge = lounge_it->second;

	if (!lounge.is_operator(&cli)) {
		cli.send("ERROR :You're not a channel operator\r\n");
		return;
	}

	if (mode == "+i") {
		lounge.set_invite_only(true);
		lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " +i\r\n", nullptr);
	}
	else if (mode == "-i") {
		lounge.set_invite_only(false);
		lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " -i\r\n", nullptr);
	}
	else if (mode == "+t") {
		lounge.set_topic_restriction(true);
		lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " +t\r\n", nullptr);
	}
	else if (mode == "-t") {
		lounge.set_topic_restriction(false);
		lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " -t\r\n", nullptr);
	}
	else if (mode == "+o") {
		for (auto &[fd, client] : _clients) {
			if (client.get_nickname() == arg && lounge.is_member(&client)) {
				lounge.add_operator(&client);
				lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " +o " + arg + "\r\n", nullptr);
				return;
			}
		}
		cli.send("ERROR :User not found\r\n");
	}
	else if (mode == "-o") {
		for (auto &[fd, client] : _clients) {
			if (client.get_nickname() == arg) {
				lounge.remove_operator(&client);
				lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " -o " + arg + "\r\n", nullptr);
				return;
			}
		}
		cli.send("ERROR :User not found\r\n");
	}
	else if (mode == "+k") {
		if (arg.empty()) {
			cli.send("ERROR :Key required (+k <password>)\r\n");
		} else {
			lounge.set_key(arg);
			lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " +k " + arg + "\r\n", nullptr);
		}
	}
	else if (mode == "-k") {
		lounge.set_key("");
		lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " -k\r\n", nullptr);
	}
	else if (mode == "+l") {
		try {
			int limit = std::stoi(arg);
			lounge.set_limit(limit);
			lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " +l " + arg + "\r\n", nullptr);
		} catch (...) {
			cli.send("ERROR :Invalid limit value\r\n");
		}
	}
	else if (mode == "-l") {
		lounge.set_limit(0);
		lounge.broadcast(":" + cli.get_nickname() + " MODE " + target + " -l\r\n", nullptr);
	}
	else {
		cli.send("ERROR :Unknown mode\r\n");
	}
}
