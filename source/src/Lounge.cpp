#include "../inc/Lounge.hpp"

//_MAIN_
Lounge::Lounge(std::string &name) : _name(name)
{
	std::cout << "new Lounge " << _name << " was Created." << std::endl;
}

Lounge::~Lounge()
{
	std::cout << "Lounge " << _name << " got Removed." << std::endl;
}

//_MEMBER_
bool	Lounge::add_client(Client* client, const std::string& key)
{
	if (_inv_only && _invited.find(client) == _invited.end())
		return false;

	if (_limit > 0 && _clients.size() >= (size_t)_limit)
		return false;

	if (!_key.empty() && _key != key)
		return false;

	//set client lounge to this->* if needed
	_clients.insert(client);
	if (_operator.empty())
		_operator.insert(client);

	std::cout << "added client to " << _name << std::endl;

	return true;
}

void	Lounge::remove_client(Client* client)
{
	_clients.erase(client);
	_operator.erase(client);
	_invited.erase(client);
}

void	Lounge::broadcast(const std::string& msg, Client* exclude)
{
	for (Client *cl : _clients)
		if (cl != exclude)	
			cl->send(msg);
}

void	Lounge::add_operator(Client* client){
	_operator.insert(client);
}

void	Lounge::remove_operator(Client* client){
	_operator.erase(client);
}

bool	Lounge::is_operator(Client* client) const{
	if (_operator.find(client) != _operator.end())
		return true;
	return false;
}

void	Lounge::invite(Client* client){
	_invited.insert(client);
}

bool	Lounge::is_invited(Client* client) const{
	if (_invited.find(client) != _invited.end())
		return true;
	return false;}

//_UTILITY_
const std::string& Lounge::get_name() const{
	return _name;
}

const std::set<Client*>& Lounge::get_clients() const{
	return _clients;
}

const std::set<Client*>& Lounge::get_operators() const{
	return _operator;
}

bool	Lounge::is_member(Client* client) const {
	if (_clients.find(client) != _clients.end())
		return true;	
	return false;
}

void	Lounge::set_topic(const std::string& topic, Client* setter){
	if (_topic_restriction && !is_operator(setter))
		return;
	_topic = topic;
}

void	Lounge::set_limit(int limit){
	_limit = limit;
}

int		Lounge::get_limit() const{
	return _limit;
}

void	Lounge::set_key(const std::string& key){
	_key = key;
}

const std::string& Lounge::get_key() const{
	return _key;
}

const std::string& Lounge::get_topic() const{
	return _topic;
}

void	Lounge::set_invite_only(bool mode){
	_inv_only = mode;
}

bool	Lounge::is_invite_only() const{
	return _inv_only;
}

void	Lounge::set_topic_restriction(bool mode){
	_topic_restriction = mode;
}

bool	Lounge::is_topic_restricted() const{
	return _topic_restriction;
}