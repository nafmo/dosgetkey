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

#define KEYS  14

#define TRUE  1
#define FALSE 0
typedef int BOOL;

/* Labels to print for verbose mode */
const char *key[KEYS + 1] = {
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

/* NLS date formats */
const char *dateformat[] = {
    "%m%%c%d%%c%Y %H%%c%M%%c%S ",
    "%d%%c%m%%c%Y %H%%c%M%%c%S ",
    "%Y%%c%m%%c%d %H%%c%M%%c%S "
};

const char *longdateformat = "%a %d %b %Y %H%%c%M%%c%S ";

/* Beep at 1KHz for 0.2 seconds */
int emitbeep(void)
{
    sound(1000);
    delay(200);
    nosound();
    return 1;
}

/* Print date and time in local format */
void printdate(time_t now, BOOL longdate)
{
    static nls_t    nls;
    static BOOL     firsttime = TRUE;
    char            format[32];
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
        firsttime = FALSE;
    }

    /* Print a date-time string */
    tm_p = localtime(&now);
    if (longdate)
    {
        strftime(format, sizeof(format), longdateformat, tm_p);
        cprintf(format, nls.timesep[0], nls.timesep[0]);
    }
    else
    {
        strftime(format, sizeof(format), dateformat[nls.dateformat], tm_p);
        cprintf(format, nls.datesep[0], nls.datesep[0],
                        nls.timesep[0], nls.timesep[0]);
    }
}

int main(int argc, char *argv[])
{
    int         c, rc, i, j, x;
    BOOL        disable[14], verbose, beep, showdate, longdate, center;
    char        *prompt;
    time_t      timeout, now, justthen;

    /* Help wanted? */
    if (2 == argc && strcmp(argv[1], "/?") == 0)
    {
        cprintf("Usage: %s [/B] [/C] [/D] [/L] [/Pprompt] [/V] [t] [key]\r\n\r\n", argv[0]);
        cprintf("  /B        Beep; Produce a sound if an illegal key was pressed\r\n");
        cprintf("  /C        Center; Center prompt on line\r\n");
        cprintf("  /D        Date; Display date and time\r\n");
        cprintf("  /L        Long date; Display verbose date format\r\n");
        cprintf("  /Pprompt  Prompt; Message to display\r\n");
        cprintf("  /V        Verbose; Print the name of the key pressed\r\n");
        cprintf("  t         Timeout (numeric); Number of seconds to wait (default: 30)\r\n");
        cprintf("  key       Name of key; List keys to DISABLE\r\n");
        cprintf("\r\nExit codes: 0 = Escape, 1-12 = F-key, 13 = Enter, 14 = Timeout\r\n");
        return 15;
    }

    /* Defaults */
    verbose  = FALSE;
    beep     = FALSE;
    showdate = FALSE;
    longdate = FALSE;
    center   = FALSE;

    timeout = time(NULL) + 30;
    prompt = NULL;

    for (i = 0; i < KEYS; i ++)
        disable[i] = FALSE;

    /* Check parameters */
    for (i = 1; i < argc; i ++)
    {
        if ('/' == argv[i][0])
        {
            BOOL stay = TRUE;

            for (j = 1; argv[i][j] && stay; j ++)
            {
                switch (toupper(argv[i][j]))
                {
                     case 'B':
                         beep = TRUE;
                         break;

                     case 'C':
                         center = TRUE;
                         break;

                     case 'D':
                         showdate = TRUE;
                         break;

                     case 'L':
                         longdate = TRUE;
                         break;

                     case 'P':
                         prompt = &argv[i][j + 1];
                         stay = FALSE;
                         break;

                     case 'V':
                         verbose = TRUE;
                         break;

                     default:
                         cprintf("Illegal option: %s\r\n", argv[i]);
                         cprintf("                %*c^\r\n", j, ' ');
                         return 15;
                }
            }
        }
        else if (isdigit(argv[i][0]))
        {
            int seconds;

            if (sscanf(argv[i], "%d", &seconds) != 1)
            {
                cprintf("Illegal time specification: %s", argv[i]);
                return 15;
            }
            timeout = time(NULL) + seconds;
        }
        else
        {
            int found = -1;

            for (j = 0; j < KEYS; j ++)
                if (stricmp(argv[i], key[j]) == 0)
                    found = j;

            if (-1 == found)
            {
                cprintf("Illegal key name: %s\r\n", argv[i]);
                return 15;
            }
            else
            {
                disable[found] = TRUE;
            }
        }
    }

    /* Check if we want the text to be centered */
    if (center)
    {
        /* TODO: Determine screen size */
        x = 40 - ((showdate ? (longdate ? 25 : 20) : 0) +
                  (prompt   ? strlen(prompt)       : 0)) / 2;

    }
    else
        x = 0;

    /* Wait for key */
    rc = -1;
    now = time(NULL);
    justthen = now;

    /* Inhibit Ctrl-Break */
    ctrlbrk(emitbeep);

    /* Print prompts */
    if (x) cprintf("%*c", x, ' ');
    if (showdate) printdate(now, longdate);
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
                if (x) cprintf("%*c", x, ' ');
                printdate(now, longdate);
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

            /* Check that we do not press disabled keys */
            if (rc >= 0 && rc < KEYS && disable[rc])
            {
                rc = -1;
                if (beep) emitbeep();
            }
        }
        now = time(NULL);
    } while (-1 == rc);

    if (verbose) cprintf("%s\n\r", key[rc]);

    return rc;
}
