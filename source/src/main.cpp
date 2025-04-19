#include <iostream>
#include <cstring>
#include <cstdlib>
#include <exception>

/*
*	GOAL is to write just the application for the server
*	pakets will be send per other programs of our own choice
*	the Program should behave like a nice program without unexpected quitting
*	different commands for different stuff . . .
*	
*/

using namespace std;

int check_input(int ac, char **av)
{
	try
	{
		if (ac != 3)
			throw runtime_error("The program expects 2 arguments");
		
		string str_port, pass;

		str_port = av[1];
		pass = av[2];
	
		for (char c : str_port ){
			if (!isdigit(c))
				throw runtime_error("Only digits allowed in <PORT> specification");
		}

		int port = stoi(str_port);
			if (port < 0 || port > 65535)
				throw runtime_error("The given port is out of range | 0 - 65535");
	}
	catch ( const exception &exc )
	{
		cout << "Please use the program accordingly" << endl;
		cout << "ircserv <PORT> <PASSWORD>" << endl;
		cerr << "Error: " << exc.what() << endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int main(int ac, char **av)
{
	if (check_input(ac, av))
		return EXIT_FAILURE;

	// set sigs
		//RETURN

	// RUN MAIN LOOP HERE
	

	return EXIT_SUCCESS;
}