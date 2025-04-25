#pragma once

#include "./Utility.hpp"
#include "./Client.hpp"

class Client;

class Lounge
{
	private:
		std::string _name;
		std::string _topic;
		std::string _key;
	
		std::set<Client *> _clients;
		std::set<Client *> _operator;
		std::set<Client *> _invited;

		bool	_inv_only = false;
		bool	_topic_restriction = false;
		int		_limit = 0;

	public: //MEMBERS
		bool	add_client(Client* client, const std::string& key);
		void	remove_client(Client* client);
		bool	is_member(Client* client) const;

		void	broadcast(const std::string& msg, Client* exclude);

		void	add_operator(Client* client);
		void	remove_operator(Client* client);
		bool	is_operator(Client* client) const;

		void	set_invite_only(bool mode);
		bool	is_invite_only() const;

		void	set_topic_restriction(bool mode);
		bool	is_topic_restricted() const;

		void	invite(Client* client);
		bool	is_invited(Client* client) const;

	public: //MAIN
		Lounge(std::string &name);
		~Lounge();

	public: //UTILITY
		const std::string& get_name() const;
		const std::set<Client*>& get_clients() const;
		const std::set<Client*>& get_operators() const;
		void	set_topic(const std::string& topic, Client* setter);
		const std::string& get_topic() const;
		void	set_limit(int limit);
		int		get_limit() const;
		void	set_key(const std::string& key);
		const std::string& get_key() const;
	};