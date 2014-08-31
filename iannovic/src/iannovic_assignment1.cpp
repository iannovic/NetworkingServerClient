/**
 * @iannovic_assignment1
 * @author  Ian Novickis <iannovic@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <errno.h>

//returns my_port
void printPort();

//prints CREATOR statement
void printCreator();

//print the Help command
void printHelp();

//do socket stuff test
int doSocket();

//initialize the server
int initServer();


//run that server
int runServer();

/*
 * return the tail of the linkedlist
 */

/*
 * LINKED LIST STUFF
 */

struct node
{
	int fd;		//reference to the fd for the socket
	int id;		//server connection id (index of the list)
	struct sockaddr * addr;
	node *next;
};
int getTail(struct node* head, struct node** ret);
int insertNode(struct node* head,struct node* newNode);
/*
 * END OF LINKED LIST STUFF
 */

int my_port;
struct node* ip_list;	
#include "../include/global.h"

using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
     /*
                VALIDATE ALL OF THE ARGUMENTS
        */
        if (argc != 3)
        {
                cout << argc << " is an illegal # of params" << endl;
                cout << "there must be 3 args" << endl;
                return -1;
        }

	my_port = atoi(argv[2]);

        if (strcmp(argv[1],"s") == 0)
        {
                cout << "running as server ";
        }
        else if (strcmp(argv[1],"c") == 0)
        {
                cout << "running as client ";
        }
        else
        {
                cout << "arg1 must be 'c'lient or 's'erver" << endl;
                return 1;
        }

        // add some validation to make sure port is a number

        cout << "listening on port " << argv[2] << "..." << endl;

        // If server, initialize server.

	if (strcmp(argv[1],"s") == 0)
	{	int status;
		cout << "now initializing server..." << endl;
		status = initServer();
		if (status < 0)
		{
			cout << "now exiting program" << endl;
		}
		else
		{
			cout << "server successfully started" << endl;
		}
		while (true)
		{	
			status = runServer();	
			if (status == -1)
			{
				cout << "exiting program with error" << endl;
				return -1;
			}
		}
	} 
	
	//	main shell loop
	
        for (std::string line; std::getline(std::cin, line);)
        {
                if (line.compare("myport") == 0)
                {
                        printPort();
                }
                else if (line.compare("creator") == 0)
                {
                        printCreator();
                }
                else if (line.compare("help") == 0)
                {
                        printHelp();
                }
                else if (line.compare("socket") == 0)
                {
			
                }
		else if ((line.compare("bye") == 0) || (line.compare("quit") == 0))
		{
			cout << "gracefully closing the program" << endl;
			return -1;
		}
                else
                {
                        cout << "invalid command, type 'help' if you need, help." << endl;
                }
	}
	return 0;
}

void printPort()
{
        cout << "listening on port " << my_port << endl;
}

void printCreator()
{
        cout << "Name: Ian Novickis" << endl;
        cout << "UBIT: iannovic" << endl;
        cout << "UBemail: iannovic@buffalo.edu" << endl;
        cout << "I have read and understood the course academic integrity policy located at: " << endl;
        cout << "http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#intregrity" << endl;
}

void printHelp()
{
        cout << "print all the commands here, so that dumb people can see" << endl;
}

int initServer()
{
	struct addrinfo hints, *response;
	int fd = 0;

	memset(&hints,0, sizeof hints);
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	/*
		get the address info
	*/
	if (getaddrinfo(NULL,"4545",&hints,&response) != 0)
	{
		cout << "failed to get addr info: " << strerror(errno) << endl;
		return -1;
	}
	
	/*
		open a new socket and return the FD
	*/
      	fd = socket(response->ai_family,response->ai_socktype,response->ai_protocol);
	if (fd == -1)
	{
		cout << "failed to open a socket: " << strerror(errno) << endl;
		return -1;
	}

	/*
		name the socket and bind it
	*/
	if (bind(fd,response->ai_addr,response->ai_addrlen) != 0)
	{
		cout << "failed to bind socket to port: " << strerror(errno) << endl;
		return -1;
	}

	/*
		tell the socket to start listening for connections
	*/
	if (listen(fd,10) != 0)
	{
		cout << "failed to listen on socket" << strerror(errno) << endl;
		return -1;
	}

	//free(hints); this line isn't working for me, but i'm leaking now
	ip_list = new struct node;
	
	//init the first node of the ip list to be the server
	ip_list->fd = fd;
	ip_list->addr = response->ai_addr;
	ip_list->next = NULL;
	ip_list->id = 1;
	return fd;	
}

/*
	safe to assume that the server socket fd is listen()ing at this point
*/
int runServer()
{
	fd_set read_set;
	fd_set write_set;
        int nfds = 0;
	int status = 0;
	//add sin to set
	FD_ZERO(&write_set);	
	FD_ZERO(&read_set);	
	/*
	 * need to insert every fd into the read_set and write_set if necessary
	 */
	FD_SET(ip_list->fd,&read_set);
	nfds++;
	status = select(nfds,&read_set,&write_set,NULL,NULL);
	if (status <= 0)
	{
		cout << "error while calling select: " << strerror(errno) << endl;
	}

	if (FD_ISSET(ip_list->fd,&read_set) != 0)
	{
		cout << "somebody connected" << endl;

		int newfd = 0;
		unsigned int length;
		struct sockaddr *newaddr = new struct sockaddr;

		/*
		 * 		accept a new connect on the socket fd
		 */
		newfd = accept(ip_list->fd,newaddr,&length);
		if (newfd == -1)
		{
			cout <<"accepting the new connection failed: " << strerror(errno) << endl;
			return -1;
		}
		else
		{
			cout << "conection accepted" << endl;
		}

		/*
		 * 	Create a new node and insert at the end of the linked list
		 */
		struct node *newNode = new struct node;
		newNode->fd = newfd;
		newNode->addr = newaddr;
		newNode->next = NULL;
		status = insertNode(ip_list,newNode);
		if (status)
		{
			cout << "failed to insert new node into the linked list" << endl;
			return -1;
		}

	}
	return 0;
}	
int connectToServer()
{
	/*
	 * getaddrinfo()
	 * call socket()
	 * call connect()
	 *
	 */
	return 0;
}
int insertNode(struct node* head,struct node* newNode)
{
	struct node **tail = NULL;
	struct node *t = NULL;
	int status = 0;

	status = getTail(head,tail);
	if (status)
	{
		cout << "failed to get tail" << endl;
		return -1;
	}

	/*
	 * get the value of tail in t then add the new node
	 */
	t = *tail;
	t->next = newNode;

	//increment the ID of the new connection node
	newNode->id = t->id++;

	return 0;
}
int getTail(struct node* head,struct node **tail)
{
	/*
	 * traverse until tail is found
	 */
	while (head->next != NULL)
	{
		head = head->next;
	}

	/*
	 * set return value
	 */
	*tail = head;

	return 0;
}
