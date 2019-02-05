/*
 * main.c
 *
 *  Created on: Mar 5, 2012
 *      Author: klausk
 */

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>

#include "tcplistener.h"

// Global handle for the TCP listener thread
pthread_t tid;

// Signal handler
static void Terminate (int sig, siginfo_t *siginfo, void *context)
{
	{ // Send a cancellation signal to the listener
		perror ("Received a terminate signal");
		// Brutally kill the listener
		pthread_cancel (tid);
	}
}

int main ( )
{
	struct sigaction act;

	pid_t pid, sid;

	printf ("Daemonising\n");

	//	Daemonise the program
	// 	Fork a child process
	pid = fork ();
	if (pid < 0)
	{
		perror ("Process did not fork!\n");
		exit (EXIT_FAILURE);
	}
	// If we got a good PID, then we can exit the parent process.
	if (pid > 0)
	{
		exit (EXIT_SUCCESS);
	}
	// Child running from here
	// Change the file mode mask
	umask (0);

	// Open any logs here
	char str[255];
	// Create a new SID for the child process
	sid = setsid ();
	if (sid < 0)
	{
		// Log any failures here
		sprintf (str, "sid = %d", sid);
		perror (str);
		exit (EXIT_FAILURE);
	}

	// Change the current working directory
	if ((chdir ("/")) < 0)
	{
		// Log any failures here
		exit (EXIT_FAILURE);
	}

	// At this point it would be nice to be able to log to a logfile
	// because we closes the stdout and stderr
	setlogmask (LOG_UPTO(LOG_NOTICE));

	openlog ("daemonising", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	// Find the log-entries in /var/log/messages prepended with "daemonising"

	syslog (LOG_NOTICE, "Program started by User %d", getuid ());
	// Close out the standard file descriptors
	close (STDIN_FILENO);
	close (STDOUT_FILENO);
	close (STDERR_FILENO);

	syslog (LOG_NOTICE, "Standard file descriptors closed.");

	memset (&act, '\0', sizeof(act));

	// Install a signal handler
	// Use the sa_sigaction field because the handles has two
	// additional parameters
	act.sa_sigaction = &Terminate;

	// The SA_SIGINFO flag tells sigaction() to use the sa_sigaction
	// field, not sa_handler.
	act.sa_flags = SA_SIGINFO;

	// Install the signal handler and relate it to the SIGTERM flag
	if (sigaction (SIGTERM, &act, NULL) < 0)
	{
		syslog (LOG_NOTICE, "sigaction failed to install");
	}
	syslog (LOG_NOTICE,
	        "sigaction installed successfully. \
	        Kill me using the SIGTERM signal.");
	// Start the TCP thread
	if (pthread_create (&tid, NULL, TCPThread, NULL) != 0)
	{
		syslog (LOG_NOTICE,
		        "An error occurred when starting main listener thread.");
	}
	syslog (LOG_NOTICE,
	        "The TCP listener thread started successfully. Awaiting your call.");
	// Just hang around until all threads has died out
	pthread_join (tid, NULL);
	syslog (LOG_NOTICE, "TCP listener died. Leaving this world. Bye.");
	closelog ();
	return 0;
}

