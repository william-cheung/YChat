#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include "kbhit.h"

// ----------------------------------------------------------------------------

/**  file scope variables  **/
static int _mode = 0;

// ----------------------------------------------------------------------------

void _chmod (int mode) {
	static struct termios oldt, newt;
	if (_mode == 0 && mode == 1) { 
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~( ICANON | ECHO );
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
		_mode = 1;
	} else if (_mode == 1 && mode == 0) {
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
		_mode = 0;
	}
}

int _rdmod (void) {
	return _mode;
}

int _kbhit (void) {
	struct timeval tv;
	fd_set rdfs;
	
	if (_mode == 0) return 1;

	tv.tv_sec = 0, tv.tv_usec = 0;

	FD_ZERO(&rdfs);
	FD_SET (STDIN_FILENO, &rdfs);

	select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
	return FD_ISSET(STDIN_FILENO, &rdfs);
}

/*int _kbhit() {
	int ch = getchar();	 
	if (ch != EOF) {
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}*/

int _getch (void) {
	return getchar();
}

// ----------------------------------------------------------------------------
