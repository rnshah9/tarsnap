#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "ttyfd.h"
#include "warnp.h"

#include "sigquit.h"

/* Set to a non-zero value when SIGQUIT is received. */
sig_atomic_t sigquit_received;

/* Saved terminal settings. */
static struct termios tc_saved;
static int fd_terminal;

static void sigquit_handler(int);
static void termios_restore(void);
static int tcsetattr_nostop(int, int, const struct termios *);

/**
 * sigquit_handler(sig):
 * Record that SIGQUIT has been received.
 */
static void
sigquit_handler(int sig)
{

	(void)sig;	/* UNUSED */

	sigquit_received = 1;
}

/**
 * termios_restore(void):
 * Restore the saved tc_saved termios state.
 */
static void
termios_restore(void)
{

	/*
	 * Discard return value; we're exiting anyway and there's nothing
	 * that we can do to remedy the situation if the system cannot
	 * restore the previous terminal settings.
	 */
	(void)tcsetattr_nostop(fd_terminal, TCSANOW, &tc_saved);

	/*
	 * Close the terminal file descriptor we opened a long time ago.  We
	 * had to hold it for the duration in case we find ourselves talking
	 * to a different terminal; we don't want to restore settings to the
	 * wrong terminal.
	 */
	close(fd_terminal);
}

/**
 * Call tcsetattr(3), but block SIGTTOU while doing so in order to avoid
 * being stopped if backgrounded.
 */
static int
tcsetattr_nostop(int fd, int action, const struct termios *t)
{
	void (*oldsig)(int);
	int rc;

	if ((oldsig = signal(SIGTTOU, SIG_IGN)) == SIG_ERR)
		goto err0;
	rc = tcsetattr(fd, action, t);
	if (signal(SIGTTOU, oldsig) == SIG_ERR)
		goto err0;

	/* Return status code from tcsetattr. */
	return (rc);

err0:
	/* Failure! */
	return (-1);
}

/**
 * sigquit_init(void):
 * Prepare to catch SIGQUIT and ^Q, and zero sigquit_received.
 */
int
sigquit_init(void)
{
	struct sigaction sa;
	struct termios tc_new;
	size_t i;

	/* We haven't seen SIGQUIT yet... */
	sigquit_received = 0;

	/* ... but when it happens, we want to catch it. */
	sa.sa_handler = sigquit_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGQUIT, &sa, NULL)) {
		warnp("sigaction(SIGQUIT)");
		goto err0;
	}

	/* Try to get a file descriptor for the terminal. */
	if ((fd_terminal = ttyfd()) == -1) {
		/*
		 * This is normal for processes which don't have a terminal
		 * attached (e.g., if running from cron), so we shouldn't
		 * print a warning, even though ^Q is going to break.
		 */
		goto done;
	}

	/* Get current terminal settings for stdin. */
	if (tcgetattr(fd_terminal, &tc_saved)) {
		/*
		 * Theoretically we should have a terminal now, but we can't
		 * rule out the possibility of some weird "sort of a terminal
		 * but not really" behaviour making tcgetattr fail even though
		 * we were previously told that this was a terminal.  Check for
		 * the common error codes operating systems return from
		 * tcgetattr for non-terminals and ignore them.
		 */
		if ((errno == ENOTTY) || (errno == ENXIO) ||
		    (errno == EBADF) || (errno == EINVAL) ||
		    (errno == ENODEV))
			goto done;

		warnp("tcgetattr(stdin)");
		goto err0;
	}

	/* Restore the terminal settings on exit(3). */
	if (atexit(termios_restore)) {
		warn("atexit");
		goto err0;
	}

	/* Copy terminal settings. */
	memcpy(&tc_new, &tc_saved, sizeof(struct termios));

	/* Remove any meaning which ^Q already has. */
	for (i = 0; i < NCCS; i++) {
		if (tc_new.c_cc[i] == ('q' & 0x1f))
			tc_new.c_cc[i] = _POSIX_VDISABLE;
	}

	/* Set VQUIT to ^Q. */
	tc_new.c_cc[VQUIT] = 'q' & 0x1f;

	/* Set new terminal settings. */
	if (tcsetattr_nostop(fd_terminal, TCSANOW, &tc_new)) {
		warnp("tcsetattr(stdin)");
		goto err0;
	}

done:
	/* Success! */
	return (0);

err0:
	/* Failure! */
	return (-1);
}
