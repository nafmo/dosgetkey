/* getkey.c
 *
 * Waits for a keypress and returns an appropriate exit code.
 * Written in 2000 by peter karlsson <peter@softwolves.pp.se> for Don Alt.
 * This code is released to the Public Domain.
 *
 * $Id$
 *
 */

#include <conio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <dos.h>
#include <ctype.h>

/* Labels to print for verbose mode */
const char *key[] = {
    "Escape", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
    "F11", "F12", "Enter", "Timeout"
};

/* Struct with country info */
typedef struct
{
    unsigned short  dateformat;
    char            currency[5], thousand[2], decimal[2], datesep[2],
                    timesep[2], currformat, currdigits, timeformat;
    unsigned short  upcaseofs, upcaseseg;
    char            listsep[2], reserved[10];
} nls_t;

/* Date formats */
const char *dateformat[] = {
    "%m%%c%d%%c%Y %H%%c%M%%c%S ",
    "%d%%c%m%%c%Y %H%%c%M%%c%S ",
    "%Y%%c%m%%c%d %H%%c%M%%c%S "
};

/* Beep at 1KHz for 0.2 seconds */
void emitbeep(void)
{
    sound(1000);
    delay(200);
    nosound();
}

/* Print date and time in local format */
void printdate(time_t now)
{
    static nls_t    nls;
    static int      firsttime = 1;
    char            format[25];
    struct tm       *tm_p;

    /* On first call, retrieve date format (etc) from DOS */
    if (firsttime)
    {
        union REGS      r;
        struct SREGS    s;

        r.x.ax = 0x3800;
        s.ds   = FP_SEG(&nls);
        r.x.dx = FP_OFF(&nls);
        intdosx(&r, &r, &s);
        firsttime = 0;
    }

    /* Print a date-time string */
    tm_p = localtime(&now);
    strftime(format, 25, dateformat[nls.dateformat], tm_p);
    cprintf(format, nls.datesep[0], nls.datesep[0],
                    nls.timesep[0], nls.timesep[0]);
}

int main(int argc, char *argv[])
{
    int         verbose, beep, showdate, c, rc, i, j;
    char        *prompt;
    time_t      timeout, now, justthen;

    /* Help wanted? */
    if (2 == argc && strcmp(argv[1], "/?") == 0)
    {
        cprintf("Usage: %s [/V] [/B] [/D] [/Pprompt] [t]\r\n\r\n", argv[0]);
        cprintf("  /V        Verbose; Print the name of the key pressed\r\n");
        cprintf("  /B        Beep; Produce a sound if an illegal key was pressed\r\n");
        cprintf("  /D        Date; Display date and time\r\n");
        cprintf("  /Pprompt  Prompt; Message to display\r\n");
        cprintf("  t         Timeout (numeric); Number of seconds to wait (default: 30)\r\n");
        cprintf("\r\nExit codes: 0 = Escape, 1-12 = F-key, 13 = Enter, 14 = Timeout\r\n");
        return 15;
    }

    /* Check parameters */
    verbose = 0;
    beep = 0;
    showdate = 0;
    timeout = time(NULL) + 30;
    prompt = NULL;

    for (i = 1; i < argc; i ++)
    {
        if ('/' == argv[i][0])
        {
            int stay = 1;

            for (j = 1; argv[i][j] && stay; j ++)
            {
                switch (toupper(argv[i][j]))
                {
                     case 'V':
                         verbose = 1;
                         break;

                     case 'B':
                         beep = 1;
                         break;

                     case 'D':
                         showdate = 1;
                         break;

                     case 'P':
                         prompt = &argv[i][j + 1];
                         stay = 0;
                         break;

                     default:
                         cprintf("Illegal option: %s\r\n", argv[i]);
                         return 15;
                }
            }
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
    now = time(NULL);
    justthen = now;
    if (showdate) printdate(now);
    if (prompt) cputs(prompt);

    do
    {
        while (!kbhit() && now <= timeout)
        {
            now = time(NULL);

            /* Update date/time display if time in seconds have changed */
            if (showdate && now != justthen)
            {
                /* Repaint it on the same line */
                cputs("\r");
                printdate(now);
                if (prompt) cputs(prompt);
                justthen = now;
            }
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
                    else if (beep)
                        emitbeep();
                    break;
                }

                case 13: /* Enter */
                    rc = 13;
                    break;

                case 27: /* Escape */
                    rc = 0;
                    break;

                default:
                    if (beep) emitbeep();
                    break;
            }
        }
        now = time(NULL);
    } while (-1 == rc);

    if (verbose) cprintf("%s\n\r", key[rc]);

    return rc;
}
