uC Con/Cmd
==========

A C based line-editing console and command tokenizer/runner intended for
microcontrollers. The two components can be used separately but are intended
to be used together to form a simple command console.

## Features (Con)

* Command history can be nagivated with up/down arrows.
* Accepts navigation keys via standard escape sequences.
* Line can be navigated with home/end/right/left/ctrl-c.
* Line can be edited with backspace/delete.

## Features (Cmd)

* Tokenizes input string based on whitespace.
* Top-level command table stored in linker section for easy compile time
  configurability.
* Sub-tables available for sorting commands into sections.
* Help provided that lists available commands along with optional help
  strings.
* Command parser interprets partial commands (eg, dis matches disassemble).

## Requirements

Requires memmove/memcpy/strncmp/strcpy/strlen as well as a putchar
implementation. The putchar implementation can be configured via config.h
and uc-io.h.

## Example Code

main.c is provided as example code that can be run on a Linux system. The
code utilizes both cmd and con. The cmd code can be exercised via passing
arguments to the test program:

    ~ # ./example help
    Commands:
      help
      quit
      math add <a> <b>
      math subtract <a> <b>
      math negate <a>
      math sort

    ~ # ./example add 5 3
    8

Invoked with no arguments, the example program will start a command console:

    ~ # ./example

    con> help
    Commands:
      help
      quit
      math add <a> <b>
      math subtract <a> <b>
      math negate <a>
      math sort

    con> math add 5 3
    8

Note that the cmd parser will match on token substrings:
    con> m n 9
    -9

But will treat ambiguous entries as invalid commands:
    con> math s 5 3
    Unknown command: math s 5 3
    Commands:
      help
      quit
      math add <a> <b>
      math subtract <a> <b>
      math negate <a>
      math sort

## Integrating into Microcontroller Build

When integrating with a microcontroller, a periodic function should check to
see if a new character is available from the console and pass it to con. Note
that con will call it's callbacks and putchar implementation so keep this in
mind if calling from an interrupt context. Example code using the tinyusb CDC
device is shown below:

    void console_periodic(void)
    {
        uint8_t ch;
        bool processed = false;

        if (!tud_cdc_connected())
            return;

        while (tud_cdc_read(&ch, 1)) {
            uc_con_handle_ch(ch);
            processed = true;
        }

        if (processed)
            fflush(stdout);

        tud_cdc_write_flush();
    }

An entry can be added to the linker table in order to specify where the command
table shoud be stored:

     __start_text_uc_cmd = .;
     KEEP(*(_text_uc_cmd))
     __stop_text_uc_cmd_end = .;

Implementations need to be provided for the con callbacks:

    /* Do nothing upon receiving EOF. Could instead reset, etc */
    void uc_con_callback_eof(void)
    {
    }

    /* Tie the output of the console to the command runner */
    void uc_con_callback_line(char *line)
    {
        uc_cmd_tokenize(line);
    }
