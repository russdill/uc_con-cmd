/* SPDX-License-Identifier: MIT */
#ifndef UC_CMD_H
#define UC_CMD_H

#include "config.h"

/*
 * Macro for top-level command entry.
 *
 * Allocates a command entry array member in the command table section. eg:
 *
 *     UC_CMD_FUNC(bozzle, do_bozzle);
 *
 * The number of arguments can be specified by adding the appropriate
 * initializers:
 *
 *     UC_CMD_FUNC(wizzit, do_wizzit, .n = 1, .help = "<count>");
 */
#define UC_CMD_FUNC(name, cb, ...) \
	static const struct uc_cmd __cmd_##name \
		__attribute__((used,section(UC_CMD_TABLE_SECTION))) = \
			{ #name, .func = cb, __VA_ARGS__ }

#define UC_CMD_IS_SUB -2

/*
 * Macro for top-level sub-menu entry.
 *
 * Allocates a reference to a sub-command table in the command table section:
 *
 *    UC_CMD_SUB(fizz, fizz_table);
 */
#define UC_CMD_SUB(name, tbl, ...) \
	static const struct uc_cmd __cmd_##name \
		__attribute__((used,section(UC_CMD_TABLE_SECTION))) = \
			{ #name, .sub = tbl, .n = UC_CMD_IS_SUB, __VA_ARGS__ }

struct uc_cmd {
	/*
	 * Used for storing the name of this command, assigned by the above
	 * macros or for sub-command tables with c array initializer:
	 *
	 * static const struct uc_cmd fizz_table[] = {
	 * 	{"zip", .func = do_zip, .n = 2, .help = "<ups> <downs>"},
	 * 	{"leek", .func = do_leek}.
	 * 	{"ifs", .sub = ifs_table, .n = UC_CMD_IS_SUB},
	 * 	{},
	 * };
	 *
	 * Note the terminating NULL entry.
	 */
	const char *token;
	union {
		/*
		 * Callback if user input matches this command.
		 *
		 * Remaining arguments after the command name tokens are
		 * passed in argv with the number of arguments given in argc.
		 *
		 * WARNING! argv is not a NULL terminated array.
		 */
		void (*func)(int argc, char *argv[]);
		/*
		 * Pointer to a sub-menu array.
		 *
		 * The array needs to be NULL terminated.
		 */
		 const struct uc_cmd *sub;
	};
	/*
	 * Help string to print in the help table after the command name.
	 *
	 * This should typically be a description arguments such as:
	 * "<foo> <bar>"
	 */
	const char *help;

	/*
	 * Number of arguments to accept for a function.
	 *
	 * -1 indicates variable number of arguments, -2 indicates this is
	 *  a sub-array, not a function.
	 */
	int n;
} __attribute__((aligned(UC_CMD_TABLE_ALIGN)));

/*
 * Walk the command table, running the given command if valid or printing
 * an error and help message if not. argv is not required to be NULL
 * terminated.
 */
void uc_cmd_run(int argc, char *argv[]);

/*
 * Tokenize a given command string and run uc_cmd_run. The passed
 * string will be modified turing tokenization.
 */
void uc_cmd_tokenize(char *cmd);

#endif
