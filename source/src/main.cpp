#include "../inc/Utility.hpp"
#include "../inc/Client.hpp"
#include "../inc/Server.hpp"
#include "../inc/Lounge.hpp"

/*
*	GOAL is to write just the application for the server
*	pakets will be send per other programs of our own choice
*	the Program should behave like a nice program without unexpected quitting
*	different commands for different stuff . . .
*	! WE SPECIFY FOR IPv4 adresses !
*/

int check_input(int ac, char **av)
{
	try
	{
		if (ac != 3)
			throw std::runtime_error("The program expects 2 arguments");
		
		std::string str_port, pass;

		str_port = av[1];
		pass = av[2];
	
		for (char c : str_port ){
			if (!isdigit(c))
				throw std::runtime_error("Only digits allowed in <PORT> specification");
		}

		int port = stoi(str_port);
			if (port < 0 || port > 65535)
				throw std::runtime_error("The given port is out of range | 0 - 65535");
	}
	catch ( const std::exception &exc )
	{
		std::cout << "Please use the program accordingly" << std::endl;
		std::cout << "ircserv <PORT> <PASSWORD>" << std::endl;
		std::cerr << "Error: " << exc.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int main(int ac, char **av)
{
	if (check_input(ac, av))
		return EXIT_FAILURE;

	try
	{
		Server serv(atoi(av[1]), std::string(av[2]));
		serv.run();	
	}
	catch(const std::exception& exc)
	{
		std::cerr << "Error: " << exc.what() << std::endl;
	}

	return EXIT_SUCCESS;
}