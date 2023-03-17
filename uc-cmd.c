/* SPDX-License-Identifier: MIT */
/*
 * uc-cmd.c - Command tokenizer/runner for microcontrollers.
 *
 * Copyright (c) 2023, Russ Dill <russ.dill@gmail.com>
 */
#include <string.h>
#include <stdint.h>

#include "uc-cmd.h"
#include "uc-io.h"

extern const struct uc_cmd UC_CMD_TABLE_START[];
extern const struct uc_cmd UC_CMD_TABLE_END;

static const struct uc_cmd *help_recurse[UC_CMD_MAX_DEPTH];

static void help_sub_name(const struct uc_cmd *cmd, int depth)
{
	uint8_t i;
	uc_io_puts("  ");
	for (i = 0; i < depth; i++) {
		uc_io_puts(help_recurse[i]->token);
		uc_io_putc(' ');
	}
	uc_io_puts(cmd->token);
}

static void uc_cmd_help(int argc, char *argv[])
{
	uint8_t depth = 0;
	const struct uc_cmd *cmd = UC_CMD_TABLE_START;

	uc_io_puts(UC_CMD_HELP_HEADER);

	while ((depth || cmd < &UC_CMD_TABLE_END) && cmd->token) {
		if (!cmd->token) {
			cmd = help_recurse[--depth] + 1;
			continue;
		}
		if (cmd->n != UC_CMD_IS_SUB) {
			help_sub_name(cmd, depth);
			if (cmd->help) {
				uc_io_putc(' ');
				uc_io_puts(cmd->help);
			}
			uc_io_putc('\n');
		} else {
			if (depth == UC_CMD_MAX_DEPTH) {
				help_sub_name(cmd, depth);
				uc_io_puts("...\n");
			} else {
				help_recurse[depth++] = cmd;
				cmd = cmd->sub;
				continue; /* Don't increment cmd */
			}
		}
		cmd++;
	}
	uc_io_putc('\n');
}
UC_CMD_FUNC(help, uc_cmd_help);

void uc_cmd_run(int argc, char *argv[])
{
	const struct uc_cmd *cmd = UC_CMD_TABLE_START;
	const struct uc_cmd *best = NULL;
	uint8_t arg;

	for (arg = 0; arg < argc && cmd; arg++) {
		uint8_t arg_len = strlen(argv[arg]);
		uint8_t matches = 0;
		best = NULL;

		for (; (arg || cmd < &UC_CMD_TABLE_END) && cmd->token; cmd++) {
			if (!strncmp(argv[arg], cmd->token, arg_len)) {
				/* token starts with argv */
				uint8_t t_len = strlen(cmd->token);
				if (arg_len <= t_len) {
					matches++;
					best = cmd;
					if (arg_len == t_len) {
						/* Exact, ignore others */
						matches = 1;
						break;
					}
				}
			}
		}

		if (matches > 1)
			/* Ambiguous, ignore match */
			best = NULL;
		cmd = best && best->n == UC_CMD_IS_SUB ? best->sub : NULL;
	}

	if (best && best->func && (best->n == -1 || best->n == argc - arg))
		best->func(argc - arg, argv + arg);
	else {
		/* Failed, print error and help */
		uc_io_puts(UC_CMD_UNKNOWN_CMD);
		for (arg = 0; arg < argc; arg++) {
			uc_io_putc(' ');
			uc_io_puts(argv[arg]);
		}
		uc_io_putc('\n');
		uc_cmd_help(0, NULL);
	}
}

void uc_cmd_tokenize(char *cmd)
{
	uint8_t arg;
	uint8_t argc = 0;
	char *tokens[UC_CMD_MAX_TOKENS];

	for (arg = 0; arg < UC_CMD_MAX_TOKENS; arg++) {
		while (cmd[0] == ' ')
			cmd++;
		if (cmd[0]) {
			tokens[arg] = cmd;
			argc++;
			while (cmd[0] && cmd[0] != ' ')
				cmd++;
			if (cmd[0]) {
				cmd[0] = '\0';
				cmd++;
			}
		} else
			tokens[arg] = NULL;
	}
	uc_cmd_run(argc, tokens);
}
