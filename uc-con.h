/* SPDX-License-Identifier: MIT */
#ifndef UC_CON_H
#define UC_CON_H

/* Call when a character is received to pass it to the console handler */
void uc_con_handle_ch(char ch);

/* Callback from the console handler when an EOF character is received */
void uc_con_callback_eof(void);

/*
 * Callback from the console handler when the user has entered a line.
 *
 * This will be NULL terminated. The buffer will be of length
 * UC_CON_LINE_LENGTH and can be modified by the callee (for tokenization,
 * etc).
 */
void uc_con_callback_line(char *line);

#endif
