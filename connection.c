/*
 * connection.c
 *
 *  Created on: Mar 5, 2012
 *      Author: klausk
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <syslog.h>

void *ConThread (void *p)
{
	int sockfd;
	const int buflen = 256;
	char buffer[buflen];
	int n;

	// Grab a copy of the socket fd
	sockfd = *(int *) p;

	syslog(LOG_NOTICE, "ConThread started conversation with client.");

	memset (buffer, 0, 256);
	// Continuously read from the socket and echo it until the socket dies out
	n = recv (sockfd, buffer, buflen, 0);
	if (n < 0)
	{
		syslog (LOG_NOTICE, "Failed to recv");
		return NULL;
	}
	while (n > 0)
	{
		// Echo back
		if (write (sockfd, buffer, strlen (buffer)) < 0)
		{
			syslog (LOG_NOTICE, "Failed to write to port");
			return NULL;
		}
		memset (buffer, 0, buflen);
		// Read again
		n = recv (sockfd, buffer, buflen, 0);
	}

	syslog (LOG_NOTICE,
	        "Exited the Communications thread. \
	        Still listening for new connections.");
	return NULL;
}
