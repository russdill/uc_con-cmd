/* SPDX-License-Identifier: MIT */
#include <string.h>
#include <stdint.h>

#include "config.h"

#include "uc-con.h"
#include "uc-io.h"

/* Current position in history buffer */
static uint8_t uc_con_idx;
static uint8_t uc_con_last;
/* Current cursor position within line */
static uint8_t uc_con_pos;
/* Current espcape sequence start character (0 is none) */
static uint8_t uc_con_esc;
/* Current escape sequence parameter byte */
static uint8_t uc_con_esc_parm;
/* History buffer */
static char uc_con[UC_CON_CMD_BUFFER][UC_CON_LINE_LENGTH];
static char uc_con_temp[UC_CON_LINE_LENGTH];

/* Delete the character under the cursor */
static void uc_con_del(void)
{
	char *line = uc_con[uc_con_idx];
	uint8_t i;
	if (line[uc_con_pos]) {
		memmove(line + uc_con_pos,
			line + uc_con_pos + 1,
			sizeof(uc_con[0]) - uc_con_pos - 1);
		uc_io_puts(line + uc_con_pos);
		uc_io_putc(' ');
		for (i = 0; i < strlen(line + uc_con_pos) + 1; i++)
			uc_io_putc('\b');
	}
}

/* Delete the character to the left of the cursor */
static void uc_con_backspace(void)
{
	char *line = uc_con[uc_con_idx];
	uint8_t i;
	if (uc_con_pos) {
		uc_io_putc('\b'); /* backspace */
		if (line[uc_con_pos]) {
			memmove(line + uc_con_pos - 1,
				line + uc_con_pos,
				sizeof(uc_con[0]) - uc_con_pos - 1);
			uc_io_puts(line + uc_con_pos - 1);
			uc_io_putc(' ');
			for (i = 0; i < strlen(line + uc_con_pos - 1) + 1; i++)
				uc_io_putc('\b');
		} else {
			line[uc_con_pos - 1] = 0;
			uc_io_puts(" \b");
		}
		uc_con_pos--;
	} else
		uc_io_putc('\a'); /* bell */
}

/* Move to the cursor to the start of the line */
static void uc_con_home(void)
{
	uint8_t i;
	for (i = 0; i < uc_con_pos; i++)
		uc_io_putc('\b');
	uc_con_pos = 0;
}

/* Move to the cursor to the end of the line */
static void uc_con_end(void)
{
	char *line = uc_con[uc_con_idx];
	if (line[uc_con_pos]) {
		uc_io_puts(line + uc_con_pos);
		uc_con_pos = strlen(line);
	}
}

/* Clear/reset the current line */
static void uc_con_new(void)
{
	uc_con_pos = 0;
	uc_con[uc_con_idx][0] = '\0';
	uc_io_putc('\n');
}

/* Print the console prompt */
static void uc_con_prompt(void)
{
	uc_io_puts(UC_CON_PROMPT);
}

/* Cancel editing the current line and reset it to the original text */
static void uc_con_cancel(void)
{
	if (uc_con_idx != uc_con_last) {
		strcpy(uc_con[uc_con_idx], uc_con_temp);
		uc_con_idx = uc_con_last;
	}
	uc_con_new();
	uc_con_prompt();
}

/* Execute the current line and save it to the command buffer */
static void uc_con_exec(void)
{
	uint8_t curr = uc_con_last;
	uint8_t prev = (curr ? curr : UC_CON_CMD_BUFFER) - 1;

	if (!uc_con[uc_con_idx][0]) {
		uc_con_cancel();
		return;
	}

	if (uc_con_idx == prev) {
		/* Just went one up */
		curr = prev;
	} else {
		if (uc_con_last != uc_con_idx) {
			/* Note: strcpy here wrongly trips Werror=restrict
			 * so use memcpy instead */
			memcpy(uc_con[uc_con_last], uc_con[uc_con_idx],
						sizeof(uc_con[0]));
			strcpy(uc_con[uc_con_idx], uc_con_temp);
		}
		uc_con_last = uc_con_last == UC_CON_CMD_BUFFER - 1 ? 0 : uc_con_last + 1;
	}
	uc_con_idx = uc_con_last;

	uc_con_new();
	strcpy(uc_con_temp, uc_con[curr]);
	uc_con_callback_line(uc_con_temp);
	uc_con_prompt();
}

/* Redraw the command buffer */
static void uc_con_redraw(uint8_t idx_new)
{
	uint8_t i, len_orig, len_new;
	uc_con_home();
	uc_io_puts(uc_con[idx_new]);
	len_orig = strlen(uc_con[uc_con_idx]);
	len_new = strlen(uc_con[idx_new]);
	for (i = len_new; i < len_orig; i++)
		uc_io_putc(' ');
	for (i = len_new; i < len_orig; i++)
		uc_io_putc('\b');
	uc_con_pos = len_new;
	uc_con_idx = idx_new;
}

/* Move to one history entry older */
static void uc_con_up(void)
{
	uint8_t i = uc_con_idx;
	do {
		i = i ? i - 1 : UC_CON_CMD_BUFFER - 1;
		if (i == uc_con_last)
			return;
	} while (!uc_con[i][0]);
	uc_con_redraw(i);
	strcpy(uc_con_temp, uc_con[i]);
}

/* Move to one history entry newer */
static void uc_con_down(void)
{
	uint8_t i = uc_con_idx;
	do {
		if (i == uc_con_last)
			break;
		i = i == UC_CON_CMD_BUFFER - 1 ? 0 : i + 1;
	} while (!uc_con[i][0]);
	uc_con_redraw(i);
	strcpy(uc_con_temp, uc_con[i]);
}

/* Handle an input character */
void uc_con_handle_ch(char _ch)
{
	uint8_t ch = _ch;
	uint8_t i;
	char *line = uc_con[uc_con_idx];

	switch (uc_con_esc) {
	case 0x1b:
		/* escape */
		switch (ch) {
		case '[':
			uc_con_esc = ch;
			uc_con_esc_parm = 0;
			break;
		default:
			uc_io_putc('^');
			uc_io_putc(ch);
			ch = 0;
			uc_con_esc = 0;
		}
		break;
	case '[':
		switch (ch) {
		case 0x30 ... 0x3f:
			/* Parameter bytes */
			uc_con_esc_parm = ch;
			break;
		case 0x20 ... 0x2f:
			/* Intermediate bytes */
			break;
		case 'A':
			/* Cursor up */
			uc_con_up();
			break;
		case 'B':
			/* Cursor down */
			uc_con_down();
			break;
		case 'C':
			/* Cursor forward */
			if (line[uc_con_pos] && uc_con_pos != sizeof(uc_con[0]) - 2)
				uc_io_putc(line[uc_con_pos++]);
			else
				uc_io_putc('\a'); /* Bell */
			break;
		case 'D':
			/* Cursor back */
			if (uc_con_pos) {
				uc_con_pos--;
				uc_io_putc('\b'); /* Backspace */
			} else
				uc_io_putc('\a'); /* Bell */
			break;
		case '~':
			/* del */
			switch (uc_con_esc_parm) {
			case '1':
			case '7':
			case '\0':
				uc_con_home();
				break;
			case '3':
				uc_con_del();
				break;
			case '4':
			case '8':
				uc_con_end();
				break;
			}
			break;
		case 'H':
			uc_con_home();
			break;
		case 'F':
			uc_con_end();
			break;
		case 'E':
		case 'G':
		case 'I' ... '}':
			/* Final bytes */
			break;
		default:
			/* Treat as normal character */
			uc_con_esc = 0;
		}
		if (ch >= 'A' && ch <= '~') {
			uc_con_esc = 0;
			ch = 0;
		}
		break;
	default:
		uc_con_esc = 0;
	}
	if (uc_con_esc)
		return;

	switch (ch) {
	case '\t':
		/* FIXME: Command completion */
		break;
	case 0x7f:
		/* Backspace */
		uc_con_backspace();
		break;
	case 0x1b:
		/* Start of escape sequence */
		uc_con_esc = ch;
		break;
	case '\r':
	case '\n':
		/* New line */
		uc_con_exec();
		break;
	case 0x03:
		/* Ctrl-C */
		uc_io_puts("^C");
		uc_con_cancel();
		break;
	case 0x04:
		/* Ctrl-D */
		uc_con_callback_eof();
		uc_con_cancel();
		break;
	case 0x80 ... 0xff:
		/* ignore */
		break;
	default:
		/* Normal character */
		if (ch < ' ')
			/* ignore */
			break;
		if (uc_con_pos == sizeof(uc_con[0]) - 2) {
			/* Line full */
			uc_io_putc('\a');
		} else {
			if (line[uc_con_pos]) {
				/* Insert */
				memmove(line + uc_con_pos + 1,
					line + uc_con_pos,
					sizeof(uc_con[0]) - uc_con_pos - 1);
				line[uc_con_pos] = ch;
				uc_io_puts(line + uc_con_pos);
				for (i = 0; i < strlen(line + uc_con_pos + 1); i++)
					uc_io_putc('\b');
			} else {
				/* Just append */
				line[uc_con_pos] = ch;
				line[uc_con_pos + 1] = '\0';
				uc_io_putc(ch);
			}
			uc_con_pos++;
		}
	}
}
