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
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "../include/global.h"

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
int blockAndAccept();

//run that client
int connectToServer();
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
int createRfds(fd_set *rfds,int *nfds);
/*
 * END OF LINKED LIST STUFF
 */

char *port_number;
struct node* ip_list;	
const char* host_address = NULL;
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

        port_number = argv[2];
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

        // If server, initialize server.

	if (strcmp(argv[1],"s") == 0)
	{
		while (true)
		{	
			if (runServer() == -1)
			{
				cout << "exiting program with error" << endl;
				/*
				 * CLOSE ALL OF THE SOCKET CONNECTIONS HERE EVENTUALLY IAN
				 */
				shutdown(ip_list->fd,0);
				return -1;
			}
		}
	} 
	else if (strcmp(argv[1],"c") == 0)
	{
		if (connectToServer() == -1)
		{
			cout << "connectToServer() failed" << endl;
			return -1;
		}
		while (true)
		{
			fd_set rfds, wfds;
			FD_ZERO(&rfds);
			FD_ZERO(&wfds);
			FD_SET(0,&rfds); 	//read on stdin to see when it has input
			cout << "waiting for input" << endl;
			if (select(2,&rfds,NULL,NULL,NULL) == -1)
			{
				cout << "failed to select: " << strerror(errno) << endl;
				return -1;
			}
			cout <<"found some input" << endl;
			char buf[1024] = {0};
			if (FD_ISSET(0,&rfds))
			{
				std::string line;
				std::getline(std::cin,line);

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
				else if (line.compare("send") == 0)
				{
					size_t len = 7;
					char buf[8] = "message";
					if (-1 == send(ip_list->fd,buf,len,0))
					{
						cout << "failed to send: " << strerror(errno) << endl;
					}
					else
					{
						cout << "message sent!" << endl;
					}
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
		}
	}
	//	main shell loop
	return 0;
}

void printPort()
{
        cout << "listening on port " << port_number << endl;
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
	cout << "now initializing server..." << endl;
	struct addrinfo hints, *response;

	int fd = 0;
	memset(&hints,0, sizeof hints);
	
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	/*
		get the address info
	*/
	if (getaddrinfo(NULL,port_number,&hints,&response) != 0)
	{
		cout << "failed to get addr info: " << strerror(errno) << endl;
		return -1;
	}
	
	/*
		open a new socket and return the FD
	*/

	/*
	cout << response->ai_family << endl;
	cout << response->ai_socktype << endl;
	cout << response ->ai_protocol << endl;
    fd = socket(response->ai_family,response->ai_socktype,response->ai_protocol);
	*/
	fd = socket(AF_INET,SOCK_STREAM,0);
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
	cout << "server successfully started" << endl;
	return fd;	
}

/*
	safe to assume that the server socket fd is listen()ing at this point
*/
int runServer()
{
	/*
	 * INITIALIZE THE SERVER IF IT HAS NOT BEEN YET =)
	 */
	if (ip_list == NULL)
	{
		if (initServer() == -1)
		{
			if (ip_list != NULL)
			{
				shutdown(ip_list->fd,0);
			}
			return -1;
		}
	}
	int status = 0;
	//add sin to set
	/*
	 * need to insert every fd into the read_set and write_set if necessary
	 */

	/*
	 * RETURN THE FD
	 */

	int fd1 = blockAndAccept();
/*
	int flags = 0;
	flags = fcntl(fd1,F_GETFL,0);
	fcntl(fd1,F_SETFL, flags | O_NONBLOCK);
*/
/*
	int fd2= blockAndAccept();
	flags = 0;
	flags = fcntl(fd2,F_GETFL,0);
	fcntl(fd2,F_SETFL, flags | O_NONBLOCK);
*/

	while (true)
	{
		cout << "one connection was made, looping for eternity" << endl;

		int nfds = 0;
		fd_set rfds;
		fd_set wfds;
		/*
		if (createRfds(&rfds,&nfds))
		{
			cout << "failed to create rdfs" << endl;
			return -1;
		}
		*/
		FD_ZERO(&rfds);
		FD_SET(fd1,&rfds);
		//FD_SET(ip_list->fd,&rfds);

		FD_ZERO(&wfds);
		FD_SET(fd1,&wfds);
		//FD_SET(fd2,&rfds);
		cout << "selecting.." << endl;
		status = select(2,&rfds,&wfds,NULL,NULL);
		if (status <= 0)
		{
			cout << "error while calling select: " << strerror(errno) << endl;
		}
		cout << "made it through select()" << endl;
		/*
		 * SEND A MESSAGE BACK SAYING THAT I RECIEVED YOUR MESSAGE
		 */
		struct node* head = ip_list->next;
		/*
		while (head != NULL)
		{
			if (FD_ISSET(head->fd,&rfds))
			{
				send(head->fd,"got it",6,0);
			}
		}
		*/
	}
	/*
	char buf[1024] = {0};
	size_t t;
	ssize_t t2;
	while((t2 = recv(newfd,buf,1024,0)) != -1)
	{
		cout << "this was the number of bytes returned: " << t2 << endl;
		cout << "this is my buffer: " << buf << endl;
		if (strncmp(buf,"exit",4) == 0)
		{
			if (shutdown(newfd,0) < 0)
			{
				cout << "Failed to close socket" << endl;
			}
			else
			{
				cout << "closed the socket" << endl;
			}
			break;
		}
		send(newfd,"got",3,0);
	}

*/

	if (false)
	{
		cout << "somebody connected" << endl;

		int newfd = 0;
		unsigned int length;
		struct sockaddr *newaddr = new struct sockaddr;

		/*
		 * 		accept a new connect on the socket fd
		 */
		cout << "waiting to accept" << endl;
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
	else
	{
		//cout << "connection timed out, nobody connected" << endl;
	}
	return 0;
}	
int connectToServer()
{
	cout << "now initializing client..." << endl;
	struct addrinfo hints, *response;

	int fd = 0;
	memset(&hints,0, sizeof hints);

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	/*
		get the address info
	*/
	if (getaddrinfo(host_address,port_number,&hints,&response) != 0)
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

	if (connect(fd,response->ai_addr,response->ai_addrlen) == -1)
	{
		cout << "failed to connect to server socket: " << strerror(errno) << endl;
		return -1;
	}
	//free(hints); this line isn't working for me, but i'm leaking now
	ip_list = new struct node;

	//init the first node of the ip list to be the server
	ip_list->fd = fd;
	ip_list->addr = response->ai_addr;
	ip_list->next = NULL;
	ip_list->id = 1;
	cout << "connected to server!" << endl;
	return 0;
}

int insertNode(struct node* head,struct node* newNode)
{
	struct node **tail;
	struct node *t = NULL;
	int status = 0;
	cout << "beginning to insert" << endl;
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
	cout << "insert complete" << endl;
	return 0;
}
int getTail(struct node* head,struct node **tail)
{

	if (head == NULL)
	{
		cout << "cannot pass NULL head" << endl;
		return -1;
	}

	cout << "starting to find the tail" << endl;
	/*
	 * traverse until tail is found
	 */
	while (head->next != NULL)
	{
		head = head->next;
	}
	cout << "i found the tail" << endl;

	/*
	 * set return value
	 */
	tail = &head;
	cout << "assigned the tail" << endl;

	return 0;
}

/*
 * LOAD all of the fd's of connections into the set
 */
int createRfds(fd_set *rfds,int *nfds)
{	*nfds = 0;
	struct node *head = ip_list->next;
	FD_ZERO(rfds);
	/*
	 *
	 */
	while (head != NULL)
	{
		FD_SET(head->fd,rfds);
		*nfds += 1;
		head = head->next;
	}

	return 0;
}

int blockAndAccept()
{
	int newfd;
	unsigned int length;
	struct sockaddr *newaddr = new struct sockaddr;
	newfd = accept(ip_list->fd,newaddr,&length);
	if (newfd == -1)
	{
		cout << "accepting the new connection failed: " << strerror(errno) << endl;
		return -1;
	}
	else
	{
		cout << "accepting a new connection" << endl;

		/*
		 * CREATE NEW CONNECTION NODE WITH THE FD
		 */
		struct node* newnode = new struct node;
		newnode->fd = newfd;
		newnode->addr = newaddr;
		newnode->next = NULL;
		if (insertNode(ip_list,newnode))
		{
			cout << "Failed to insert new node" << endl;
			return -1;
		}
	}

	return newfd;
}
