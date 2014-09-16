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
#include <arpa/inet.h>
#include <sstream>
#include <errno.h>
#include "../include/global.h"

//returns my_port
void printPort();

//prints CREATOR statement
void printCreator();

//print the Help command
void printHelp();

//initialize the server
int initListen();

//run that server
int blockAndAccept();

//run that client
int connectToServer(std::string address, std::string port);
/*
 * return the tail of the linkedlist
 */

/*
 * LINKED LIST STUFF
 * the head of the list will always be the listening socket fd
 */

struct node
{
	int fd;		//reference to the fd for the socket
	int id;		//server connection id (index of the list)
	struct sockaddr * addr;
	std::string port;
	std::string address;
	std::string name;
	node *next;
};

int getTail(struct node* head, struct node** ret);
int insertNode(struct node* head,struct node* newNode);
int createRfds(fd_set *rfds);
int appendNodeToString(char* buf, node* value);
int getPortAndIp(node* theNode, int fd);
int buildUpdatedValidList(char* buf);
/*
 * END OF LINKED LIST STUFF
 */

/*
 * print the connections list
 */
void printValidList();

int updateClientsList();
char *port_number;

struct node* ip_list;	
struct node* ip_tail;

/*
 * keep track of the valid connections on the client using these
 */
struct node* valid_connections_head;
struct node* valid_connections_tail;

const char* host_address = NULL;
bool isClient;
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
     /*         VALIDATE ALL OF THE ARGUMENTS		 */
	if (argc != 3)
	{
			cout << argc << " is an illegal # of params" << endl;
			cout << "there must be 3 args" << endl;
			return -1;
	}

	port_number = argv[2];
	if (strcmp(argv[1],"s") == 0)
	{
			isClient = false;
			cout << "running as server ";
	}
	else if (strcmp(argv[1],"c") == 0)
	{
			isClient = true;
			cout << "running as client ";
	}
	else
	{
			cout << "arg1 must be 'c'lient or 's'erver" << endl;
			return 1;
	}

	/*
	 * initiate the listening socket, both server and client
	 */
	if (initListen() == -1)
	{
		cout << "failed to open listening socket!" << endl;
		return -1;
	}

	// add some validation to make sure port is a number
	while (true)
	{
		fd_set rfds;
		if (-1 == createRfds(&rfds))
		{
			cout << "failed to create the rfds" << endl;
			return -1;
		}
		FD_SET(0,&rfds); 	//read on stdin to see when it has input

		cout << "waiting for input" << endl;
		if (select(1024,&rfds,NULL,NULL,NULL) == -1)
		{
			cout << "failed to select: " << strerror(errno) << endl;
			return -1;
		}

		/*
		 * this will always be the listening socket, both client and server
		 */
		if (FD_ISSET(ip_list->fd,&rfds))
		{
			if (-1 == blockAndAccept())
			{
				cout << "failed to accept" << endl;
			}
			else
			{
				cout << "accepted a new connection!" << endl;
			}
			if (-1 == updateClientsList())
			{
				cout << "failed to update all clients valid list" << endl;
				return -1;
			}
		}

		/*
		 * check to see if any open sockets needs to be read
		 */
		if (ip_list->next != NULL)
		{
			struct node* head = ip_list->next;
			while (head != NULL)
			{
				cout << "coming from fd: " << head->fd << endl;

				if (FD_ISSET(head->fd,&rfds))
				{
					char buf[1024] = {0};
					size_t t;
					ssize_t t2;
					t2 = recv(head->fd,&buf,1023,0);
					if (t2 != -1)
					{
						cout << "got a message: " << buf << endl;
					}
					if (-1 == buildUpdatedValidList(buf))
					{
						cout << "failed to buld the valid connections list" << endl;
						return -1;
					}
					printValidList();

				}
				head = head->next;
			}
		}

		/*
		 * check stdin
		 */
		if (FD_ISSET(0,&rfds))
		{
			std::string line;
			std::getline(std::cin,line);
			string arg[3];
			int argc = 0;
			stringstream ssin(line);
			while (ssin.good())
			{
				ssin >> arg[argc];
				cout << arg[argc] << endl;
				argc++;
				if (argc >= 3)
				{
					cout << "cannot have that many arguments" << endl;
					break;
				}
			}

			if (arg[0].compare("myport") == 0)
			{
				printPort();
			}
			else if (arg[0].compare("creator") == 0)
			{
					printCreator();
			}
			else if (arg[0].compare("help") == 0)
			{
					printHelp();
			}
			else if (arg[0].compare("send") == 0)
			{
				size_t len = 7;
				char buf[8] = "message";
				if (-1 == write(ip_list->fd,buf,len))
				{
					cout << "failed to send: " << strerror(errno) << endl;
				}
				else
				{
					cout << "message sent!" << endl;
				}
			}
			else if (arg[0].compare("register") == 0)
			{
				if (argc != 3)
				{
					cout << "invalid number of arguments" << endl;
				}
				else if (!isClient)
				{
					cout << "you are the server, silly. You can't register." << endl;
				}
				else
				{
					connectToServer(arg[1],arg[2]);
				}
			}
			else if ((arg[0].compare("bye") == 0) || (arg[0].compare("quit") == 0))
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
	//	main shell loop
	return 0;
}

int initListen()
{
	cout << "now initializing the listening socket..." << endl;
	struct addrinfo hints, *response;

	int fd = 0;
	memset(&hints,0, sizeof hints);
	
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	/*
		get the address info
	*/
	cout << "listening port number: " <<port_number << endl;
	if (getaddrinfo(NULL,port_number,&hints,&response) != 0)
	{
		cout << "failed to get addr info: " << strerror(errno) << endl;
		return -1;
	}
	
	fd = socket(AF_INET,SOCK_STREAM,0);
	if (fd == -1)
	{
		cout << "failed to open a socket: " << strerror(errno) << endl;
		return -1;
	}

	if (bind(fd,response->ai_addr,response->ai_addrlen) != 0)
	{
		cout << "failed to bind socket to port: " << strerror(errno) << endl;
		return -1;
	}

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
	ip_tail = ip_list;
	cout << "listening socket is fd:" << ip_list->fd << endl;
	cout << "server successfully started" << endl;
	return fd;	
}
int connectToServer(std::string address, std::string port)
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
	if (getaddrinfo(address.c_str(),port.c_str(),&hints,&response) != 0)
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


	struct node* newnode = new struct node;
	if (getPortAndIp(newnode,fd) == -1)
	{
		cout << "failed to get port and ip:" << endl;
		return -1;
	}

	newnode->fd = fd;
	newnode->addr = response->ai_addr;
	newnode->next = NULL;
	newnode->id = 1;
	ip_tail->next = newnode;
	ip_tail = newnode;
	cout << "connected to server!" << endl;

	return 0;
}

int insertNode(struct node* head,struct node* newNode)
{
	struct node *tail;
	int status = 0;
	cout << "beginning to insert" << endl;
	if (getTail(head,&tail) == -1  || tail == NULL)
	{
		cout << "failed to get tail" << endl;
		return -1;
	}
	/*
	 * get the value of tail in t then add the new node
	 */
	cout << "blah tail:" << tail << endl;

//	tail->id++;
	tail->next = newNode;
	cout << "lol"<< tail->id << endl;
	newNode->id = tail->id;
	//increment the ID of the new connection node

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
	cout << "assigned the tail:" << *tail  << endl;

	return 0;
}

/*
 * LOAD all of the fd's of connections into the set
 */
int createRfds(fd_set *rfds)
{
	struct node *head = ip_list;
	FD_ZERO(rfds);

	while (head != NULL)
	{
		FD_SET(head->fd,rfds);
		//cout << "setting: " << head->fd << endl;
		head = head->next;
	}

	return 0;
}
int blockAndAccept()
{
	int newfd;
	struct sockaddr *newaddr = new struct sockaddr;
	socklen_t addr_size = sizeof(struct sockaddr);
	newfd = accept(ip_list->fd,newaddr,&addr_size);

	if (newfd == -1)
	{
		cout << "accepting the new connection failed: " << strerror(errno) << endl;
		return -1;
	}

	struct sockaddr_in* address = (struct sockaddr_in*)&newaddr;
	struct node* newnode = new struct node;
	/*
	 * sysout some info about the new accept
	 */
	if (-1 == getPortAndIp(newnode,newfd))
	{
		cout << "failed to get port and ip:" << endl;
		return -1;
	}
	//cout << "accepting a new connection" << endl;

	/*
	 * CREATE NEW CONNECTION NODE WITH THE FD
	 */
	newnode->fd = newfd;
	newnode->addr = newaddr;
	newnode->next = NULL;
	newnode->id = ip_tail->id + 1;
	ip_tail->next = newnode;
	ip_tail = newnode;
	return newfd;
}
int getPortAndIp(node* theNode, int fd)
{
   struct sockaddr_in peer;
   socklen_t peer_len;
   peer_len = sizeof(peer);
   if (getpeername(fd,(struct sockaddr*)&peer, &peer_len) == -1)
   {
	  cout << "failed to getpeername" << endl;
	  return -1;
   }

   /* all of this code to get the port*/
   u_short shorty = ntohs(peer.sin_port);
   std::stringstream ss;
   ss << shorty ;
   std::string s = ss.str();
   theNode->port = s.c_str();
   /* end of code to get the port into a char[]*/

   theNode->address = inet_ntoa(peer.sin_addr);

   cout << "Peer address is " << theNode->address << ":" << theNode->port<< endl;

   /*
    * cout my own listening socket
    */
   /*
   sockaddr_in sock;
   socklen_t socklen = sizeof(sock);

   if (-1 == getsockname(fd,(struct sockaddr*) &sock,&socklen))
   {
	   cout << "failed to get sockname" << endl;
	   return -1;
   }
   cout << "My address is " << inet_ntoa(sock.sin_addr) << ":" << ntohs(sock.sin_port) << endl;
   */
   return 0;
}
int registerWithServer()
{

	return 0;
}
/*
 * easy functions
 */
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

/*
 * send an updated list of valid connectons to each valid client
 */
int updateClientsList()
{
	/*
	 * construct the string to write to each socket connection
	 */
	node* head2 = ip_list;
	char *buf = new char[1024];
	bzero(buf,1024);

	/*
	 * add "update" header to beginning of the string
	 */
	strcat(buf,"update");


	while (head2 != NULL)
	{
		appendNodeToString(buf,head2);
		head2 = head2->next;
	}

	/*
	 * write to each socket pipe with the constructed string
	 */
	cout << "buffer is:" << buf << endl;
	node* head = ip_list->next;
	while (head != NULL)
	{
		cout << "Writing to the fd:" << head->fd << "..." << endl;
		if (-1 == write(head->fd,buf,1023))
		{
			cout << "failed to write list out to: " << head->fd << endl;
		}
		else
		{
			cout << "successfully wrote to " << head->fd << " with message: " << buf << endl;
		}
		head = head->next;
	}
	free(buf);
	return 0;
}

int appendNodeToString(char* buf, node* value)
{
	strcat(buf,value->address.c_str());
	strcat(buf," ");
	strcat(buf,value->port.c_str());
	strcat(buf," ");
	cout << buf<< endl;
	return 0;
}

int buildUpdatedValidList(char* buf)
{
	/*
	 * this will leak a lot of memory for now. @TODO fix this
	 */
	valid_connections_head = NULL;
	valid_connections_tail = NULL;
	/*
	 * FIX THE ABOVE TO FREE EVERYTHING PROPERLY PLEASE IAN
	 */

	cout << "starting to build new valid connections list.." << endl;
	char *token = strtok(buf," ");
	cout << "this is your first token: " << token << endl;
	if (strcmp(token,"update") == -1)
	{
		cout << "cannot call this function without an update header in the stream" << endl;
		return -1;
	}

	cout << "finished validating the operation..." << endl;

	char* addressBuffer = new char[64];
	char* portBuffer = new char[16];

	struct node* currentNode = new node;
	valid_connections_head = currentNode;
	int i = 0;

	/*
	 * iterate once passed the "update"
	 */
	int maxTokensAllowed = 16;
	char **tokens = new char*[maxTokensAllowed];
	while (token != NULL)
	{
		if (i >= maxTokensAllowed)
		{
			break;
		}

		cout << "token: " << token << endl;
		fuckexec[i] = new char[strlen(token)];
		strcpy(tokens[i],token);
		token = strtok(NULL," ");

		i++;
	}

	i = 0;
	while (tokens[i] != NULL)
	{

	}
	/*
	token = strtok(NULL," ");
	while (token != NULL)
	{
		cout << i << endl;
		if (i == 0)
		{
			addressBuffer = token;
			i = 1;
		}
		else
		{
			if (currentNode == NULL)
			{
				currentNode = new node;
			}
			portBuffer = token;
			currentNode->address = addressBuffer;
			currentNode->port = portBuffer;
			cout << addressBuffer << ":" << portBuffer << endl;
			currentNode = currentNode->next;

			bzero(addressBuffer,63);
			bzero(portBuffer,15);
			i = 0;
		}
		token = strtok(NULL," ");
		cout << buf << endl;
		cout << "this is your new token:" << token << endl;
	}

	//free(&addressBuffer);
	//free(&portBuffer);

	cout << "list has been built!" << endl;
	*/
	return 0;
}

void printValidList()
{
	struct node* head = valid_connections_head;
	cout << "printing all valid connections" << endl;
	while (head != NULL)
	{
		cout << head->address << ":" << head->port << endl;
		head = head->next;
	}
}
