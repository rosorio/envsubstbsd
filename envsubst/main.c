#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

static const struct option longOpts[] = {
    { "variables", no_argument, NULL, 'v'},
    { "help", no_argument, NULL, 'h'},
    { "version", no_argument, NULL, 'V'},
};


int
check_variable_name(char * var, int len)
{
    int cnt = 0;

    if (!isalpha(*var)) {
        return (0);
    }

    for (char *c = var; *c != '\0' && (!len || (cnt < len)); c++) {
        if(!isalpha(*c) && !isnumber(*c) && *c != '_') {
            return (len ? 0 : cnt);
        }
        cnt++;
    }

    return (cnt);

}

void
print_variable(char * name, int len)
{
    char * dest;
    dest = malloc(len+1);
    strlcpy(dest,name,len+1);
    printf("%s\n",dest);
    free(dest);
}

void parse_args(char *val)
{
    char *sepv = "$";
    char *sepbr = "}";
    char *word, *end, *brkt, *brkb;
    int vlen;

    for (word = strstr(val, sepv);
         word;
         word = strstr(word, sepv)) {
        word++;
        if (word[0] != '{'){
            /* looking for variable breaker */
            vlen = check_variable_name(word, 0);
            if(vlen) {
                print_variable(word,vlen);
            }
        } else {
            word++;
            /* looking for closing  braket */
            if ((end = strstr(word, sepbr)) != NULL){
                /* Looking for a variable breaker */
                vlen = check_variable_name(word, (end - word));
                if(vlen) {
                    print_variable(word,vlen);
                }
            }
        }
    }
}

void print_error(const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: ", getprogname());
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

void print_info(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

static void
usage()
{
    printf("Usage: %s [OPTION] [SHELL-FORMAT]\n\n"
           "Substitutes the values of environment variables.\n\n"
           "In normal operation mode, standard input is copied to standard output,\n"
           "with references to environment variables of the form $VARIABLE or ${VARIABLE}\n"
           "being replaced with the corresponding values.  If a SHELL-FORMAT is given,\n"
           "only those environment variables that are referenced in SHELL-FORMAT are\n"
           "substituted; otherwise all environment variables references occurring in\n"
           "standard input are substituted.\n\n"
           "When --variables is used, standard input is ignored, and the output consists\n"
           "of the environment variables that are referenced in SHELL-FORMAT, one per line.\n",
           getprogname());

}

static void
version()
{
    printf("%s (envsubsd BSD version) 0.0.1\n"
           "License BSD: New BSD licence\n"
           "written by Rodrigo Oosorio <rodrigo@freebsd.org>\n",
           getprogname());

}

int
main(int argc, char * argv[])
{
    int ch  = 0;
    int v_flag = 0;
    int longIndex;

    while ((ch = getopt_long(argc, argv, "vhV", longOpts,&longIndex)) != -1) {
        switch (ch) {
        case 'v':
                v_flag = 1;
            break;
        case 'h':
            usage();
            exit (0);
            break;
        case 'V':
            version();
            exit (0);
        default:
            print_info("Try 'bsdenvsubst --help' for more information");
            exit (EPERM);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0 && v_flag) {
        print_error("missing arguments");
        exit (EPERM);
    }

    if (argc == 1) {
        parse_args(argv[0]);
    } else {
        print_error("too many arguments");
        exit (EPERM);
    }
}
