/*
 * tcplistener.c
 *
 *  Created on: Mar 5, 2012
 *      Author: klausk
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <syslog.h>
#include "connection.h"

void *TCPThread (void *ptr)
{
	int sockfd;
	int newsockfd;
	int retval;
	int yes = 1;

	struct addrinfo hints;
	struct addrinfo *p;
	struct addrinfo *servinfo;

	// Prepare to wait for a call
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP; //IP protocol
	hints.ai_flags = 0; //AI_PASSIVE;

	// Get some information about this interface
	retval = getaddrinfo ("127.0.0.1", "1955", &hints, &servinfo);
	if (retval != 0)
	{
		syslog (LOG_NOTICE, gai_strerror (retval));
		pthread_exit (NULL);
	}
	//Loop through result and bind to the first possible
	for (p = servinfo; p != NULL; p = p->ai_next)
	{ // Prepare a socket
		sockfd = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1)
		{
			syslog (LOG_NOTICE, "Error opening socket");
			continue;
		}
		retval = setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (retval == -1)
		{
			syslog (LOG_NOTICE, "setsockopt");
		}
		retval = bind (sockfd, p->ai_addr, p->ai_addrlen);
		if (retval == -1)
		{
			close (sockfd);
			syslog (LOG_NOTICE, "Error binding");
			continue;
		}
		break;
	}
	// Did we find anything?
	if (p == NULL)
	{ // Nope - lets get outta here
		syslog (LOG_NOTICE, "Failed to get any interface to listen at");
		pthread_exit (NULL);
		return NULL;
	}

	retval = listen (sockfd, 5);
	if (retval == -1)
	{
		perror ("listen failed");
		return NULL;
	}
	// Wait for a call to arrive
	while (1)
	{
		struct sockaddr clntsckt;
		socklen_t adrsz;
		// When a call comes in spawn a new thread and hand over the socket
		newsockfd = accept (sockfd, &clntsckt, &adrsz);
		// Spawn a new thread to handle this connection
		pthread_t tid;
		if (pthread_create (&tid, NULL, ConThread, &newsockfd) != 0)
		{
			perror ("An error occurred while starting new connection thread.");
		}
	}
	return NULL;
}

