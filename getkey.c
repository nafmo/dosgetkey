/* getkey.c
 *
 * Waits for a keypress and returns an appropriate exit code.
 * Written in 2000 by peter karlsson <peter@softwolves.pp.se> for Don Alt.
 * This code is released to the Public Domain.
 *
 */
#include <conio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <dos.h>

/* Labels to print for verbose mode */
const char *key[] = {
    "Escape", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "F11", "F12", "Enter", "Timeout"
};

int main(int argc, char *argv[])
{
    int verbose, c, rc, i;
    time_t timeout, now;
    union REGS r;

    /* Help wanted? */
    if (2 == argc && strcmp(argv[1], "/?") == 0)
    {
        cprintf("Usage: %s [/V] [t]\r\n\r\n", argv[0]);
        cprintf("  /V  Verbose; Print the name of the key pressed\r\n");
        cprintf("  t   Timeout (numeric); Number of seconds to wait (default: 30)\r\n");
        cprintf("\r\nExit codes: 0 = Escape, 1-12 = F-key, 13 = Enter, 14 = Timeout\r\n");
        return 15;
    }

    /* Check parameters */
    verbose = 0;
    timeout = time(NULL) + 30;

    for (i = 1; i < argc; i ++)
    {
        if (stricmp(argv[i], "/V") == 0)
        {
            verbose = 1;
        }
        else if (argv[i][0] == '/')
        {
            cprintf("Illegal option: %s\r\n", argv[i]);
            return 15;
        }
        else
        {
            int seconds;

            sscanf(argv[i], "%d", &seconds);
            timeout = time(NULL) + seconds;
        }
    }

    /* Wait for key */
    rc = -1;
    do
    {
        now = time(NULL);
        while (!kbhit() && now <= timeout)
        {
            now = time(NULL);
        }
        /* Leave if we have reached timeout */
        if (now > timeout)
            rc = 14;
        else
        {
            c = getch();

            switch (c)
            {
                case 0: /* Extended key */
                {
                    c = getch();
                    if (c >= 59 && c <= 68) /* F1-F10 */
                       rc = c - 58;
                    else if (133 == c || 134 == c) /* F11-F12 */
                       rc = c - 122;
                    break;
                }

                case 13: /* Enter */
                     rc = 13;
                     break;

                case 27: /* Escape */
                     rc = 0;
                     break;
            }
        }
    } while (-1 == rc);

    if (verbose) cprintf("%s\n\r", key[rc]);

    return rc;
}
