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
#include <ifaddrs.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <errno.h>
#include <netdb.h>
#include "../include/global.h"

//returns my_port
void printPort();

//prints CREATOR statement
void printCreator();

void printExternalIp();

//print the Help command
void printHelp();

//initialize the server
int initListen();

//print open connections
void printOpenList();

//run that server
int blockAndAccept();

int getExternalIp();

/*
 * the flag is to determine whether registering or connecting
 */
int connectTo(std::string address, std::string port,int flag);
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
	std::string hostname;
	node *next;
};
int validateAddressAndPort(std::string address, std::string port);
int getTail(struct node* head, struct node** ret);
int insertNode(struct node* head,struct node* newNode);
int createRfds(fd_set *rfds);
int appendNodeToString(char* buf, node* value);
int getAddressInfo(std::string port, std::string address);
int getPortAndIp(node* theNode, int fd);
int buildUpdatedValidList(char** buf);
int tokenizeBufferedMessage(char *buf, char **tokens, int maxTokens,int *tokenCount);
int getNodeById(struct node **ret, int id);
int isContained(std::string address, std::string port, struct node *head);
int closeSocketAndDeleteNode(struct node* deleteeNode);
/*
 * END OF LINKED LIST STUFF
 */

/*
 * print the connections list
 */
void printValidList();

int updateAndSendValidList();
char *port_number;

struct node* listening_socket;

struct node* open_connections_head;
struct node* open_connections_tail;
int open_connections_size = 0;
int max_connections_allowed = 3;

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

	/*
	 * populate listening_socket->address field with the external IP
	 */
	if (-1 == getExternalIp())
	{
		cout << "failed to get external ip" << endl;
		return -1;
	}
	cout << ".-*^*-._.-*^*-._.-*^*-._.-*^*-._WELCOME!.-*^*-._.-*^*-._.-*^*-._.-*^*-._" << endl;
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

		/********************************************************************************
		 * BEGIN THE SELECT() LOOP!
		 ********************************************************************************/
		if (select(1024,&rfds,NULL,NULL,NULL) == -1)
		{
			cout << "failed to select: " << strerror(errno) << endl;
			return -1;
		}
		//cout << "found input" << endl;
		/*
		 * this will always be the listening socket, both client and server
		 */
		if (FD_ISSET(listening_socket->fd,&rfds))
		{
			if (-1 == blockAndAccept())
			{
				cout << "failed to accept" << endl;
			}
			else
			{
				cout << "accepted a new connection!" << endl;
			}
		}

		struct node* head = open_connections_head;
		/**************************************************
		 * CHECK INPUT ON EACH SOCKET CONNECTION
		 **************************************************/
		while (head != NULL)
		{
			if (FD_ISSET(head->fd,&rfds))
			{
				//cout << "found input on socket: " << head->fd << endl;
				int bufLength = 256;
				char buf[256] = {0};
				ssize_t bytesReceived;
				bytesReceived = recv(head->fd,&buf,256,0);
				if (bytesReceived == -1)
				{
					cout << "failed to receive the message " << buf << endl;
				}
				/***************************************************
				 * CURRENT HEAD CLOSED CONNECTION
				 ***************************************************/
				else if (bytesReceived == 0)
				{
					cout << "the process at " << head->hostname << " has closed their socket" << endl;
					cout << "Closer's Address: " << head->address << endl;
					cout << "Closer's Port: " << head->port << endl;

					closeSocketAndDeleteNode(head);

					if (!isClient)
					{
						updateAndSendValidList();
					}
				}
				/*************************************************
				 * CHECK FOR MESSAGES
				 * each "message" has a "prefix" token which determines how to process the message
				 * if update -> we parse the message into a new valid list of connections from the server
				 * if message-> we assume that the message was sent to us
				 * if port -> we assume that the peer is sending its listening port number
				 *
				 *************************************************/
				else
				{
					cout << buf << endl;
					int maxTokens = 32;
					int tokenCount = 0;
					char *tokens[maxTokens];
					//cout << "initialized buffer" << endl;
					if (-1 == tokenizeBufferedMessage(buf,(char**)&tokens,maxTokens,&tokenCount))
					{
						cout << "failed to tokenize the buffer" << endl;
						return -1;
					}
					else if (tokenCount < 1)
					{
						cout << "too little tokens from the message..." << endl;
						//return -1;
					}
					//cout << "determining what function to run" << endl;
					std::string command = tokens[0];
					/**************************************************************
					 * CHECK TO SEE IF WE NEED TO UPDATE THE VALID LIST OF CONNECTIONS
					 **************************************************************/
					if (command.compare("update") == 0)
					{
						if (tokenCount < 3)
						{
							cout << "not enough tokens to run this command" << endl;
						}
						else if (!isClient)
						{
							cout << "server should not run this command" << endl;
						}
						//cout << "now building list" << endl;
						else if (1 == buildUpdatedValidList((char**)&tokens))
						{
							cout << "failed to build the valid connections list from server" << endl;
						}
						else
						{
							cout << "Valid connections have been updated by the server..." << endl;
							printValidList();
						}
					}
					/******************************************************************************
					 * CHECK TO SEE IF SOMEONE SENT THER LISTENING PORT NUMBER TO THIS PROCESS
					 ******************************************************************************/
					else if (command.compare("port") == 0)
					{
						cout << "starting to set port" << endl;
						if (tokenCount < 2)
						{
							cout << "failed to set port, not enough tokens" << endl;
						}
						else if (!isClient)
						{
							if (isContained(head->address,tokens[1],open_connections_head) == -1)
							{
								cout << "DANGER!! the peer trying to connect has already connected to you, closing the connection" << endl;
								/*
								 * write an errorr message to the client
								 */
								std::string errorMessage = "message Your connection was refused, you are already connected to me on another port/socket!";
								size_t bufLength = strlen(errorMessage.c_str());
								if (-1 == write(head->fd,errorMessage.c_str(),bufLength))
								{
									cout << "failed to send: " << strerror(errno) << endl;
								}
								closeSocketAndDeleteNode(head);
								if (-1 == close(head->fd))
								{
									cout << "failed to close the socket" << strerror(errno) << endl;
								}
							}
							else
							{
								/*
								 * move the value of tokens[1] into the head->port. we only do this now because we had to validate that it did not exist
								 */
								head->port = tokens[1];
								if (updateAndSendValidList() == -1)
								{
									/*
									 * we only updateClientList IF we are the server
									 * otherwise assume that its relevant to the open connections list only
									 */
									cout << "failed to update all clients valid list" << endl;
								}
								else
								{
									printOpenList();
								}
							}
						}
						else if (isClient && isContained(head->address,tokens[1],valid_connections_head) == 0)
						{
							cout << "DANGER!! The peer on this socket is not a validated host. closing the connection immedietely." << endl;
									/*
							 * write an errorr message to the client
							 */
							std::string errorMessage = "message Your connection was refused, you are not registered to the server";
							size_t bufLength = strlen(errorMessage.c_str());
							if (-1 == write(head->fd,errorMessage.c_str(),bufLength))
							{
								cout << "failed to send: " << strerror(errno) << endl;
							}
							closeSocketAndDeleteNode(head);
							if (-1 == close(head->fd))
							{
								cout << "failed to close the socket" << strerror(errno) << endl;
							}
						}
						else
						{
							/*
							 * this will happen if everythign else was successfull and we are the isClient == true
							 */
							head->port = tokens[1];
							printOpenList();
						}
					}
					/**************************************************************************
					 * CHECK TO SEE IF SOMEONE SENT A MESSAGE TO THIS PROCESS
					 **************************************************************************/
					else if (command.compare("message") == 0)
					{
						cout << "Message received from " << head->hostname << endl;
						cout <<	"Sender's IP: " 	<< head->address	<< endl;
						cout << "Sender's Port: " 	<< head->port		<< endl;
						cout << "Message: ";
						for (int i = 1; i < tokenCount; i ++)
						{
							cout << tokens[i] << " ";
						}
						cout << endl;
					}
				}
			}
			head = head->next;
		}
		/****************************************************************************
		 * BEGIN CHECKING FOR SHELL COMMANDS
		 ****************************************************************************/
		if (FD_ISSET(0,&rfds))
		{
			//cout << "reading from stdin" << endl;
			std::string line;
			std::getline(std::cin,line);
			int maxTokensRead = 32;
			string arg[maxTokensRead];
			int argc = 0;
			stringstream ssin(line);
			while (ssin.good())
			{
				ssin >> arg[argc];
				//cout << arg[argc] << endl;
				argc++;
				if (argc >= maxTokensRead)
				{
					cout << "cannot have that many arguments" << endl;
					break;
				}
			}

			if (arg[0].compare("myport") == 0)
			{
				printPort();
			}
			/*************************************************************************
			 * CREATOR COMMAND
			 *************************************************************************/
			else if (arg[0].compare("creator") == 0)
			{
					printCreator();
			}
			/*************************************************************************
			 * MYIP COMMAND
			 *************************************************************************/
			else if (arg[0].compare("myip") == 0)
			{
					printExternalIp();
			}
			/*************************************************************************
			 * HELP COMMAND
			 *************************************************************************/
			else if (arg[0].compare("help") == 0)
			{
					printHelp();
			}
			/*************************************************************************
			 * SEND COMMAND
			 *************************************************************************/
			else if (arg[0].compare("send") == 0)
			{
				struct node* retNode = NULL;
				if (argc < 3)
				{
					cout << "not enough arguments to run this command" << endl;
				}
				else if (!isClient)
				{
					cout << "server is not permitted to send messages" << endl;
				}
				else if (arg[1].compare("1") == 0)
				{
					cout << "you are not allowed to send a message to the server." << endl;
				}
				else if (-1 == getNodeById(&retNode,atoi(arg[1].c_str())))
				{
					cout << "invalid connection ID" << endl;
				}
				else
				{
					std::string prefix = "message ";
					size_t buflen = strlen(prefix.c_str());
					size_t max_buflen = 100 + buflen;

					/*
					 * start i at 2 because it ignores the first 2 arguments (send, connectionID)
					 */
					for (int i = 2; i < argc; i ++)
					{
						buflen = buflen + strlen(arg[i].c_str()) + 1;
					}

					//create buffer
					char buf[buflen];
					strcpy(buf,prefix.c_str());
					for (int i = 2; i < argc; i ++)
					{
						strcat(buf,arg[i].c_str());
						strcat(buf," ");
					}

					if (buflen >= max_buflen)
					{
						cout << "message cannot be greater than 100 characters, please shorten the message!" << endl;
					}
					else if (-1 == write(retNode->fd,&buf,buflen))
					{
						cout << "failed to send: " << strerror(errno) << endl;
					}
				}
			}
			/************************************************************************
			 * REGISTER COMMAND
			 ************************************************************************/
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
				else if (-1 == connectTo(arg[1],arg[2],0))
				{
					cout << "failed to register" << endl;
				}
			}
			/************************************************************************
			 * CONNECT COMMAND
			 ************************************************************************/
			else if (arg[0].compare("connect") == 0)
			{
				if (argc != 3)
				{
					cout << "invalid number of arguments" << endl;
				}
				else if (!isClient)
				{
					cout << "you are the server, silly. You can't register." << endl;
				}

				/*
				 * check to see if the connection is already in open connections list
				 */
				else if (isContained(arg[1],arg[2],open_connections_head) == -1)
				{
					cout << "connection with this address and port already exists!" << endl;
				}
				else if (-1 ==connectTo(arg[1],arg[2],1))
				{
					cout << "failed to connect" << endl;
				}
				else
				{
					printOpenList();
				}
			}
			/******************************************************************
			 * TERMINATE COMMAND
			 *****************************************************************/
			else if (arg[0].compare("terminate") == 0)
			{
				struct node* retNode = NULL;
				int id = atoi(arg[1].c_str());
				if (argc != 2)
				{
					cout << "when using terminate you need 2 arguments, second is the connectID. use list command to find connectionID's" << endl;
				}
				else if (arg[1].compare("1") == 0)
				{
					cout << "now terminating connection with server and all clients (cannot connect to peers while not connnected to server" << endl;
					struct node *head = open_connections_head;
					valid_connections_head = NULL;
					while (head != NULL)
					{
						struct node *deletedNode = head;
						head = head->next;
						if (-1 == closeSocketAndDeleteNode(deletedNode))
						{
							cout << "Failed to delete and close socket connection" << endl;
						}
					}

				}
				else if (getNodeById(&retNode,id) == -1)
				{
					cout << "failed to get the connection from the list by that ID, wrong ID perhaps?" << endl;
				}
				else if (closeSocketAndDeleteNode(retNode) == -1)
				{
					cout << "Failed to closeSocketAndDeleteNode() ..." << endl;
				}
				else
				{
					cout << "successfully terminated the socket connection!" << endl;
				}
			}
			/*****************************************************************************
			 * LIST COMMAND (open connections)
			 *****************************************************************************/
			else if (arg[0].compare("list") == 0)
			{
				printOpenList();
				cout << endl;
			}
			/*****************************************************************************
			 * PRINTS THE VALID LIST (added this for my own use)
			 *****************************************************************************/
			else if (arg[0].compare("valid") == 0)
			{
				printValidList();
				cout << endl;
			}
			/*****************************************************************************
			 * EXIT BYE QUIT COMMAND
			 *****************************************************************************/
			else if ((arg[0].compare("exit") == 0) ||(arg[0].compare("bye") == 0) || (arg[0].compare("quit") == 0))
			{
				struct node *head = open_connections_head;
				while (head != NULL)
				{
					struct node *deletedNode = head;
					head = head->next;
					if (-1 == closeSocketAndDeleteNode(deletedNode))
					{
						cout << "Failed to delete and close socket connection" << endl;
					}
				}

				if (-1 == closeSocketAndDeleteNode(listening_socket))
				{
					cout << "Failed to close listening socket on port " << port_number << endl;
					return -1;
				}
				else
				{
					cout << "Successfully closed the listening socket on port " << port_number	<< endl;
				}
				cout << ".-*^*-._.-*^*-._.-*^*-._.-*^*-._GOODBYE!.-*^*-._.-*^*-._.-*^*-._.-*^*-._" << endl;
				return -1;
			}
			/*******************************************************************
			 * IF INPUT IS UNRECOGNIZED...
			 *******************************************************************/
			else
			{
					cout << "invalid command, type 'help' if you need help." << endl;
			}
		}
		/***********************************************************************************************
		 * END OF STDIN CHECKING
		 ***********************************************************************************************/
	}
	return 0;
}

/*
 * start the listening socket of the process. should be one of the first functions ran in main()
 */
int initListen()
{
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
	}m
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

	//init the first node of the ip list to be the server
	listening_socket = new struct node;
	listening_socket->fd = fd;
	listening_socket->addr = response->ai_addr;
	listening_socket->next = NULL;
	listening_socket->port = port_number;
	cout << "listening port number is " << listening_socket->port << endl;
	return fd;	
}
int validateAddressAndPort(std::string address, std::string port)
{
	struct node* head = valid_connections_head;
	while (head != NULL)
	{
		if (address.compare(head->address) == 0 && port.compare(head->port) == 0)
		{
			/* address is considered valid now */
			return 0;
		}
		head = head->next;
	}
	return -1;
}

/*
 * determines if a node is contained, if it is, then returns a pointer to that node.. else returns null and -1
 */
int getNodeById(struct node **ret, int id)
{
	struct node* head = open_connections_head;
	while (head != NULL)
	{
		if (head->id == id)
		{
			*ret = head;
			return 0;
		}
		head = head->next;
	}

	if (*ret == NULL)
	{
		return -1;
	}
	return 0;
}
int connectTo(std::string address, std::string port,int flag)
{
	/*
	 * also have to check to see if this is a register versus a connect to
	 *
	 * 		flag values
	 * 		0 == register
	 *  	1 == connect
	 *
	 * otherwise we prevent the client from connecting to the server
	 * we check to see if == 0 because that implies that something was not contained in the list
	 *
	 * this works the opposite way for the open_connections because we want to check for duplicates
	 */
	if (isContained(address,port,listening_socket) == -1)
	{
		cout << "you cannot connect to yourself" << endl;
		return -1;
	}
	if (flag == 0 && isContained(address,port,open_connections_head) == -1)
	{
		cout << "you cannot register with the server again!" << endl;
	}

	if (flag == 1 && isContained(address,port,valid_connections_head) == 0)
	{
		cout << "The address you are trying to connect to is not registered and validated." << endl;
		return -1;
	}
	if (flag == 1 && isContained(address,port,open_connections_head) == -1)
	{
		cout << "You are already connected to that peer!" << endl;
		return -1;
	}

	if (max_connections_allowed + 1 <= open_connections_size)
	{
		cout << "you cannot connect, you aready have 3 open peer connections" << endl;
		return -1;
	}

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

	struct node* currentNode = new struct node;
	if (-1 == getPortAndIp(currentNode,fd))
	{
		cout << "failed to get port and ip!" << endl;
		return -1;
	}
	if (open_connections_head == NULL)
	{
		open_connections_head = currentNode;
		open_connections_tail = currentNode;
		currentNode->id = 1;
	}
	else
	{
	 currentNode->id = open_connections_tail->id + 1;
	 open_connections_tail->next = currentNode;
	 open_connections_tail = open_connections_tail->next;
	}

	currentNode->fd = fd;
	currentNode->addr = response->ai_addr;
	currentNode->next = NULL;

	size_t len = 20;
	char buf[20] = {0};
	strcpy((char*)&buf,"port ");
	strcat((char*)&buf,port_number);
	if (-1 == write(fd,buf,len))
	{
		cout << "failed to send my port to connectee: " << strerror(errno) << endl;
		return -1;
	}
	else {
		cout << "successfully sent my port to the server" << endl;
	}

	if (flag == 0)
	{
		cout << "successfully registered to the server!" << endl;
	}
	else
	{
		cout << "a new connection has succesfully been opened!" << endl;
	}
	open_connections_size++;
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
	struct node *head = open_connections_head;
	FD_ZERO(rfds);

	/*
	 * add the listening socket to the fd_set
	 */
	FD_SET(listening_socket->fd,rfds);

	/*
	 * add all of the open connections to the fd_set
	 */
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

	newfd = accept(listening_socket->fd,newaddr,&addr_size);
	if (newfd == -1)
	{
		cout << "accepting the new connection failed: " << strerror(errno) << endl;
		return -1;
	}

	/*
	 * must accept the connection then close it right away (could not find a refuse/reject function)
	 * we are at capacity when there are 4 connections since one must always be with the server
	 */

	if (open_connections_size >= max_connections_allowed + 1)
	{
		cout << "error Cannot accept the new connection, more than 3 open connections is not allowed!" << endl;
		/*
		 * write an errorr message to the client
		 */
		std::string errorMessage = "message Your connection was refused, the host had 3 open connections already!";
		size_t bufLength = strlen(errorMessage.c_str());
		if (-1 == write(newfd,errorMessage.c_str(),bufLength))
		{
			cout << "failed to send: " << strerror(errno) << endl;
		}
		if (-1 == close(newfd))
		{
			cout << "failed to close the socket" << strerror(errno) << endl;
		}
		return -1;
	}

	struct sockaddr_in* address = (struct sockaddr_in*)&newaddr;
	struct node* newNode = new struct node;

	/*
	 * sysout some info about the new accept
	 */
	if (-1 == getPortAndIp(newNode,newfd))
	{
		cout << "failed to get port and ip:" << endl;
		return -1;
	}

	if (open_connections_head == NULL)
	{
		open_connections_head = newNode;
		newNode->id = 1;
		open_connections_tail = open_connections_head;
	}
	else
	{
		open_connections_tail->next = newNode;
		newNode->id = open_connections_tail->id + 1;
		open_connections_tail = open_connections_tail->next;
	}
	newNode->fd = newfd;
	newNode->addr = newaddr;
	newNode->next = NULL;
	open_connections_size++;
	//cout << "finished accepting and inserting" << endl;
	return newfd;
}
int getPortAndIp(node* theNode, int fd)
{
   struct sockaddr_in peer;
   socklen_t peer_len;
   peer_len = sizeof(peer);
   if (getpeername(fd,(struct sockaddr*)&peer, &peer_len) == -1)
   {
	  cout << "failed to getpeername: " << strerror(errno) << endl;
	  return -1;
   }

   int hostBufLength = 255;
   char *hostBuffer = new char[hostBufLength];
   if (-1 == getnameinfo((sockaddr*)&peer, peer_len, hostBuffer, hostBufLength, NULL, 0, 0))
   {
	   cout << "failed to get name info: " << strerror(errno) << endl;
	   return -1;
   }


   /* all of this code to get the port*/
   u_short shorty = ntohs(peer.sin_port);
   std::stringstream ss;
   ss << shorty ;
   std::string s = ss.str();
   theNode->port = s.c_str();
   /* end of code to get the port into a char[]*/
   theNode->hostname = hostBuffer;
   theNode->address = inet_ntoa(peer.sin_addr);
   delete hostBuffer;
   hostBuffer = NULL;
   //cout << "Peer address on fd " << fd << " is: " << theNode->address << ":" << theNode->port << " with name: "<< theNode->hostname << endl;
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
		cout <<"creator" << endl;
		cout << "--->displays info about the author of this program" << endl << endl;
		cout << "myip" << endl;
		cout << "--->prints out the ip address of the process" << endl << endl;
		cout << "myport" << endl;
		cout << "--->prints out the listening port number of the process" << endl << endl;
		cout << "register <hostname or address> <port>" << endl;
		cout << "--->client only. registers client to server. first command that must be ran by all clients" << endl << endl;
		cout << "connect <hostname or address> <port>" << endl;
		cout << "--->client only. connect to a peer process with a new TCP socket connection" << endl << endl;
		cout << "terminate <connectionID>" << endl;
		cout << "--->close the TCP socket connection with the given connectionID" << endl << endl;
		cout << "send <connectionID> <message>" << endl;
		cout << "--->write a message to the given connectionID" << endl << endl;
		cout << "exit" << endl;
		cout << "--->ends the program after closing all existing connections" << endl << endl;
		cout << "list" << endl;
		cout << "--->lists all of the information about each open connection" << endl << endl;
}

/*
 * send an updated list of valid connectons to each valid client
 */
int updateAndSendValidList()
{
	cout << "updating valid connections for all clients..." << endl;
	/*
	 * construct the string to write to each socket connection
	 */
	node* head2 = open_connections_head;

	size_t maxBufLength = 256;
	char *buf = new char[maxBufLength];
	bzero(buf,256);

	/*
	 * add "update" header to beginning of the string
	 */
	strcat(buf,"update ");

	while (head2 != NULL)
	{
		appendNodeToString(buf,head2);
		head2 = head2->next;
	}

	/*
	 * write to each socket pipe with the constructed string
	 */
	node* head = open_connections_head;
	while (head != NULL)
	{
		if (-1 == write(head->fd,buf,maxBufLength - 1))
		{
			cout << "failed to write list out to: " << head->fd << endl;
			delete buf;
			return -1;
		}
		head = head->next;
	}
	cout << "successfully updated all of the clients' valid address lists" << endl;
	delete buf;
	buf = NULL;
	return 0;
}

int appendNodeToString(char* buf, node* value)
{
	strcat(buf,value->address.c_str());
	strcat(buf," ");
	strcat(buf,value->port.c_str());
	strcat(buf," ");
	strcat(buf,value->hostname.c_str());
	strcat(buf," ");
	//cout << buf<< endl;
	return 0;
}
int tokenizeBufferedMessage(char *buf, char **tokens,int maxTokens,int *tokenCount)
{
	char *token = strtok(buf," ");
	char **localTokens = tokens;
	int i = 0;

	/*
	 * get all of the words in the message into an array
	 */
	//cout << "beginning to tokenize" << endl;
	while (token != NULL)
	{
		if (i >= maxTokens)
		{
			cout << "too many tokens in the buffer" << endl;
			return -1;
		}

		//cout << "token: " << token << endl;
		localTokens[i] = token;
		//strcpy(localTokens[i],token);
		token = strtok(NULL," ");
		i++;
	}
	//set the number of tokens counted
	*tokenCount = i;
	localTokens[*tokenCount] = NULL;

	//cout << "completed tokenizing" << endl;
	return 0;
}
int buildUpdatedValidList(char** buf)
{
	/*
	 * this will leak a lot of memory for now. @TODO fix this
	 */
	valid_connections_head = NULL;
	valid_connections_tail = NULL;

	/*
	 * do this to skip passed the "update" in index 0
	 */
	int i = 1;
	struct node* currentNode = NULL;
	char ** tokens = buf;
	while (tokens[i] != NULL && tokens[i+1] && tokens[i+2] != NULL)
	{
		//cout << "iterating.. index is "	<< i << endl;
		if (valid_connections_head == NULL)
		{
			currentNode = new struct node;
			valid_connections_head = currentNode;
		}
		else
		{
			currentNode->next = new struct node;
			currentNode = currentNode->next;
		}
		currentNode->address = tokens[i];
		currentNode->port = tokens[i+1];
		currentNode->hostname = tokens[i+2];
		currentNode->next = NULL;
		i = i + 3;
	}
	currentNode = NULL;
	return 0;
}

void printValidList()
{
	struct node* head = valid_connections_head;
	cout << "Printing all VALID addresses" << endl;
	cout << "=====================================" << endl;
	while (head != NULL)
	{
		cout << "Hostname: "	<< head->hostname;
		cout << ", Address: " 	<< head->address;
		cout << ", Port: " 		<< head->port 	<< endl;
		head = head->next;
	}
	cout << "=====================================" << endl;
}

void printOpenList()
{
	struct node* head = open_connections_head;
	cout << "Printing all OPEN Connections" << endl;
	cout << "=====================================" << endl;
	while (head != NULL)
	{

		cout <<"ConnectID:" << head->id;
		cout << " hostname: " << head->hostname;
		cout <<" address: " << head->address;
		cout <<" port: " << head->port << endl;
		head = head->next;
	}
	cout << "=====================================" << endl;
}

int isContained(std::string address,std::string port,struct node* head)
{
	/*
	 *TODO check to see if connected to self as a base case
	 */

	//cout << "starting to check if it's already connected to this port/address" << endl;
	while (head != NULL)
	{
		if ((address == head->address || address == head->hostname) && port == head->port)
		{
			/*
			 * if we enter this block, then the connection already existed.
			 */
			return -1;
		}
		head = head->next;
	}
	//cout << "success, beginning to connect..." << endl;
	return 0;
}
int closeSocketAndDeleteNode(struct node* deleteeNode)
{
	//cout << "starting to deleteNode" << endl;
	/*
	 * close the socket
	 * write a function that closes the socket, removes the open connection from the list of connections
	 * updates the size of the list
	 */
	struct node* head = open_connections_head;
	struct node* previous = NULL;
	/*
	 * check to see if the head is the deletee, if so, then set him to be the next element in the list b/c no one else is pointing to him
	 */
	//cout << "checking if its the head" << endl;
	if (head == deleteeNode)
	{
		open_connections_head = open_connections_head->next;
		head = NULL;
	}
	//cout << "checking everything else..." << endl;
	while (head != NULL)
	{
		if (head == deleteeNode)
		{
			previous->next = head->next;
			if (open_connections_tail == head)
			{
				open_connections_tail = previous;
			}
			break;
		}
		previous = head;
		head = head->next;
	}


	/*
	 * if the head is NULL at this point, then assume that the structure is empty
	 */
	if (head == NULL)
	{
		open_connections_tail == NULL;
	}

	/*
	 * decrement the size of the structure
	 */
	open_connections_size--;

	if (close(deleteeNode->fd) == -1)
	{
		cout << "failed to close the socket with connectionID: " << deleteeNode->id << endl;
	}
	return 0;
}

int getExternalIp()
{
	/*
	 * this portion of code was taken from the man page of getifaddrs(3)
	 * http://man7.org/linux/man-pages/man3/getifaddrs.3.html
	 *
	 */
		struct ifaddrs *ifaddr, *ifa;
		int family, n;
		char host[NI_MAXHOST];

	   if (getifaddrs(&ifaddr) == -1)
	   {
		   cout << "failed to getifaddrs: " << strerror(errno) << endl;
		   return -1;
	   }
	   void * tmpAddrPtr=NULL;
       for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
       {
           if (ifa->ifa_addr == NULL)
           {
               continue;
           }

           family = ifa->ifa_addr->sa_family;

           /* For an AF_INET* interface address, display the address */
           if (ifa->ifa_addr->sa_family == AF_INET)
           {
        	   // check it is IP4
			  tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
			  char addressBuffer[INET_ADDRSTRLEN];

			  inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

			  if (strcmp(ifa->ifa_name,"eth0") == 0)
			  {
				  listening_socket->address = addressBuffer;
				  //cout << listening_socket->address << endl;
			  }
           }
       }
	return 0;
}

void printExternalIp()
{
	cout << "Your external IP is: " << listening_socket->address << endl;
}
