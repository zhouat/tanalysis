/*	$OpenBSD: main.c,v 1.22 2008/10/09 06:48:11 jmc Exp $	*/
/*	$NetBSD: main.c,v 1.7 1997/05/13 06:15:57 mikel Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static const char copyright[] =
"@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
#if 0
static const char sccsid[] = "@(#)main.c	8.2 (Berkeley) 4/20/95";
#else
static const char rcsid[] = "$OpenBSD: main.c,v 1.22 2008/10/09 06:48:11 jmc Exp $";
#endif
#endif /* not lint */

#include "rcv.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include "extern.h"

static void	usage(void);
	int	main(int, char **);

/*
 * Mail -- a mail program
 *
 * Startup -- interface with user.
 */

int
main(int argc, char **argv)
{
	int i;
	struct name *to, *cc, *bcc, *smopts;
	char *subject;
	char *replyto;
	char *header;
	char *ef;
	char* cmd;
	char nosrc = 0;
	char *rc;
	extern const char version[];

        /*
	 * drop any sgid/suid privileges
         */ 
        if (setgid (getgid()) < 0) {
          err(1, "setgid");
 	}
	
	if (setuid (getuid()) < 0) {
          err(1, "setuid");
	}  
 
	/*
	 * Set up a reasonable environment.
	 * Figure out whether we are being run interactively,
	 * start the SIGCHLD catcher, and so forth.
	 */
	(void)signal(SIGCHLD, sigchild);
	(void)signal(SIGPIPE, SIG_IGN);
	if (isatty(0))
		assign("interactive", "");
	
	/*
	 * Grab some stuff from the environment we might use
	 */

	if ((cmd = getenv("PAGER")))
		assign("PAGER", cmd);
	if ((cmd = getenv("LISTER")))
		assign("LISTER", cmd);
	if ((cmd = getenv("SHELL")))
		assign("SHELL", cmd);
	if ((cmd = getenv("EDITOR")))
		assign("EDITOR", cmd);
	if ((cmd = getenv("VISUAL")))
		assign("VISUAL", cmd);
	if ((cmd = getenv("MBOX")))
		assign("MBOX", cmd);
	if ((cmd = getenv("DEAD")))
		assign("DEAD", cmd);
	if ((cmd = getenv("REPLYTO")))
		assign("REPLYTO", cmd);

	image = -1;
	/*
	 * Now, determine how we are being used.
	 * We successively pick off - flags.
	 * If there is anything left, it is the base of the list
	 * of users to mail to.  Argp will be set to point to the
	 * first of these users.
	 */
	ef = NULL;
	to = NULL;
	cc = NULL;
	bcc = NULL;
	smopts = NULL;
	subject = NULL;
	header = NULL;
	replyto = NULL;
	while ((i = getopt(argc, argv, "EINT:a:b:c:defins:u:v")) != -1) {
		switch (i) {
		case 'T':
			/*
			 * Next argument is temp file to write which
			 * articles have been read/deleted for netnews.
			 */
			Tflag = optarg;
			if ((i = creat(Tflag, 0600)) < 0)
				err(1, "%s", Tflag);
			(void)close(i);
			break;
		case 'u':
			/*
			 * Next argument is person to pretend to be.
			 */
#ifndef DEBIAN
			if (strlen(optarg) >= MAXLOGNAME)
				errx(1, "username `%s' too long", optarg);
#endif			
			unsetenv("MAIL");
			myname = optarg;
			uflag = 1;
			break;
		case 'i':
			/*
			 * User wants to ignore interrupts.
			 * Set the variable "ignore"
			 */
			assign("ignore", "");
			break;
		case 'd':
			debug++;
			break;
#define REMOVE_NEWLINES(arg) { 	char *t; 					\
				for (t = (arg); *t; t++)			\
					if (*t == '\n' || *t == '\r') *t = ' '; \
			     }					
		case 's':
			/*
			 * Give a subject field for sending from
			 * non terminal
			 */
			REMOVE_NEWLINES(optarg);
			subject = optarg;
			break;

		case 'a':
			/*
			 * Give additional header fields for sending from
			 * non terminal
			 */
			REMOVE_NEWLINES(optarg);
			if (header == NULL) {
				if ((header = (char *)malloc(strlen(optarg)+1)) != NULL)
					strcpy(header, optarg);
                        } else {
				if ((header = (char *)realloc(header, strlen(optarg)+strlen(header)+2)) != NULL) {
					strcat(header, "\n");
					strcat(header, optarg);
				}
			}
			break;
		case 'f':
			/*
			 * User is specifying file to "edit" with Mail,
			 * as opposed to reading system mailbox.
			 * If no argument is given after -f, we read his
			 * mbox file.
			 *
			 * getopt() can't handle optional arguments, so here
			 * is an ugly hack to get around it.
			 */
			if ((argv[optind]) && (argv[optind][0] != '-'))
				ef = argv[optind++];
			else
				ef = "&";
			break;
		case 'n':
			/*
			 * User doesn't want to source /usr/lib/Mail.rc
			 */
			nosrc++;
			break;
		case 'N':
			/*
			 * Avoid initial header printing.
			 */
			assign("noheader", "");
			break;
		case 'v':
			/*
			 * Send mailer verbose flag
			 */
			assign("verbose", "");
			break;
		case 'I':
			/*
			 * We're interactive
			 */
			assign("interactive", "");
			break;
		case 'c':
			/*
			 * Get Carbon Copy Recipient list
			 */
			cc = cat(cc, nalloc(optarg, GCC));
			break;
		case 'b':
			/*
			 * Get Blind Carbon Copy Recipient list
			 */
			bcc = cat(bcc, nalloc(optarg, GBCC));
			break;
		case 'e':
		case 'E':
			/*
			 * Don't send messages with an empty body.
			 */
			assign("skipempty", "");
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	for (i = optind; (argv[i]) && (*argv[i] != '-'); i++)
		to = cat(to, nalloc(argv[i], GTO));
	for (; argv[i]; i++)
		smopts = cat(smopts, nalloc(argv[i], 0));
	/*
	 * Check for inconsistent arguments.
	 */
	if (to == NULL && (subject != NULL || cc != NULL || bcc != NULL))
		errx(1, "You must specify direct recipients with -s, -c, or -b");
	if (ef != NULL && to != NULL)
		errx(1, "Cannot give -f and people to send to");
	/*
	 * Block SIGINT except where we install an explicit handler for it.
	 */
	sigemptyset(&intset);
	sigaddset(&intset, SIGINT);
	(void)sigprocmask(SIG_BLOCK, &intset, NULL);
	/*
	 * Initialization.
	 */
	tinit();
	setscreensize();
	input = stdin;
	rcvmode = !to;
	spreserve();
	if (!nosrc)
		load(_PATH_MASTER_RC);
	replyto = value("REPLYTO");
	/*
	 * Expand returns a savestr, but load only uses the file name
	 * for fopen, so it's safe to do this.
	 */
	if ((rc = getenv("MAILRC")) == 0)
		rc = "~/.mailrc";
	load(expand(rc));
	if (!rcvmode) {
		mail(to, cc, bcc, smopts, subject, header, replyto);
		/*
		 * why wait?
		 */
		exit(senderr);
	}
	/*
	 * Ok, we are reading mail.
	 * Decide whether we are editing a mailbox or reading
	 * the system mailbox, and open up the right stuff.
	 */
	if (ef == NULL)
		ef = "%";
	if (setfile(ef) < 0)
		exit(1);		/* error already reported */

	if (value("quiet") == NULL)
		(void)printf("Mail version %s.  Type ? for help.\n",
			version);
	announce();
	//fflush(stdout);
	commands();
	(void)ignoresig(SIGHUP, NULL, NULL);
	(void)ignoresig(SIGINT, NULL, NULL);
	(void)ignoresig(SIGQUIT, NULL, NULL);
	quit();
	exit(0);
}

/*
 * Compute what the screen size for printing headers should be.
 * We use the following algorithm for the height:
 *	If baud rate < 1200, use  9
 *	If baud rate = 1200, use 14
 *	If baud rate > 1200, use 24 or ws_row
 * Width is either 80 or ws_col;
 */
void
setscreensize(void)
{
	struct termios tbuf;
	struct winsize ws;
	speed_t ospeed;

	if (ioctl(1, TIOCGWINSZ, (char *) &ws) < 0)
		ws.ws_col = ws.ws_row = 0;
	if (tcgetattr(1, &tbuf) < 0)
		ospeed = B9600;
	else
		ospeed = cfgetospeed(&tbuf);
	if (ospeed < B1200)
		screenheight = 9;
	else if (ospeed == B1200)
		screenheight = 14;
	else if (ws.ws_row != 0)
		screenheight = ws.ws_row;
	else
		screenheight = 24;
	if ((realscreenheight = ws.ws_row) == 0)
		realscreenheight = 24;
	if ((screenwidth = ws.ws_col) == 0)
		screenwidth = 80;
}

static void
usage(void)
{

	printf("usage: %s [-dEIinv] [-a header] [-b bcc-addr] [-c cc-addr] "
	    "[-s subject] to-addr ...\n", __progname);
	printf("       %*s [-- sendmail-options ...]\n",
	    (int)strlen(__progname), "");
	printf("       %s [-dEIiNnv] -f [name]\n", __progname);
	printf("       %s [-dEIiNnv] [-u user]\n", __progname);
	exit(1);
}
