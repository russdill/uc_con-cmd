/* SPDX-License-Identifier: MIT */
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "uc-con.h"
#include "uc-cmd.h"

static struct termios termios_orig;

static void cleanup_termios(void)
{
	tcsetattr(0, TCSANOW, &termios_orig);
}

/* Quit upon receiving EOF */
void uc_con_callback_eof(void)
{
	printf("\n");
	exit(0);
}

/* Tie the output of the console to the command runner */
void uc_con_callback_line(char *line)
{
	uc_cmd_tokenize(line);
}

static void math_add(int argc, char *argv[])
{
	printf("%d\n", atoi(argv[0]) + atoi(argv[1]));
}

static void math_subtract(int argc, char *argv[])
{
	printf("%d\n", atoi(argv[0]) - atoi(argv[1]));
}

static void math_negate(int argc, char *argv[])
{
	printf("%d\n", -atoi(argv[0]));
}

static int compare(const void *a_p, const void *b_p)
{
	int a = *(const int *) a_p;
	int b = *(const int *) b_p;
	return a == b ? 0 : (a < b ? -1 : 1);
}

/* example of variable number of arguments */
static void math_sort(int argc, char *argv[])
{
	int n[argc];
	int i;

	for (i = 0; i < argc; i++)
		n[i] = atoi(argv[i]);
	qsort(n, argc, sizeof(n[0]), compare);
	for (i = 0; i < argc; i++)
		printf("%d ", n[i]);
	printf("\n");
}

/* example of sub-command list */
static const struct uc_cmd math_commands[] = {
	{"add", .func = math_add, .n = 2, .help = "<a> <b>"},
	{"subtract", .func = math_subtract, .n = 2, .help = "<a> <b>"},
	{"negate", .func = math_negate, .n = 1, .help = "<a>"},
	{"sort", .func = math_sort, .n = -1},
	{},
};
UC_CMD_SUB(math, math_commands, .help = "additional commands");

/* example of top-level command */
static void cmd_quit(int argc, char *argv[])
{
	exit(0);
}
UC_CMD_FUNC(quit, cmd_quit);

int main(int argc, char *argv[])
{
	struct termios t;

	if (argc > 1) {
		/*
		 * Callback can be used directly for purposes such as
		 * scripting
		 */
		uc_cmd_run(argc - 1, argv + 1);
		return 0;
	}

	/* Make a linux terminal work like a serial console */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	tcgetattr(0, &t);
	tcgetattr(0, &termios_orig);
	atexit(cleanup_termios);
	cfmakeraw(&t);
	t.c_oflag |= OPOST;
	tcsetattr(0, TCSANOW, &t);

	uc_con_handle_ch('\n'); /* Print initial prompt */
	for (;;) {
		int c = getchar();
		if (c == EOF)
			break;
		uc_con_handle_ch(c);
	}

	return 0;
}
