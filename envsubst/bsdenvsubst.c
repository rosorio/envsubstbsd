#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>

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
        
        if(!isalnum(*c) && *c != '_') {
            return (len ? 0 : cnt);
        }
        cnt++;
    }
    return (cnt);
}

typedef struct env_node_t {
    char * var_name;
    char * var_value;
    TAILQ_ENTRY (env_node_t) next;
} env_node_t;
TAILQ_HEAD (env_head_t, env_node_t);

void
add_variable(struct env_head_t * el, char * str, int len)
{
    struct env_node_t *node, *it;
    char *name;
    char *value;

    name = strndup(str, len + 1);
    if (name == NULL) {
        exit (ENOMEM);
    }
    name[len] = '\0';
    value = getenv(name);

    TAILQ_FOREACH (it, el, next) {
        if (strcmp(name, it->var_name) == 0) {
            free(name);
            return;
        }
    }

    node = malloc(sizeof(struct env_node_t));
    if(node == NULL) {
        exit (ENOMEM);
    }

    node->var_name = name;
    node->var_value = strdup(value ? value : "");

    if (node->var_value == NULL) {
        exit (ENOMEM);
    }

    TAILQ_INSERT_TAIL(el, node, next);
}

int
parse_shell_string(struct env_head_t * el, char * shell)
{
    char *ms = "$";
    char *me   = "}";
    char *ws, *we;
    int wl;
    
    for (ws = strstr(shell, ms); ws; ws = strstr(ws, ms)) {
        ws++;
        if (ws[0] != '{'){
            wl = check_variable_name(ws, 0);
            if(wl) {
                add_variable(el, ws, wl);
            }
        } else {
            ws++;
            if ((we = strstr(ws, me)) != NULL){
                wl = check_variable_name(ws, (we - ws));
                if(wl) {
                    add_variable(el, ws, wl);
                }
            }
        }
    }
    return (0);
}

static void
print_error(const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: ", getprogname());
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

static void
print_info(const char *fmt, ...)
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
    struct env_head_t env_list;

    TAILQ_INIT (&env_list);

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
        parse_shell_string(&env_list, argv[0]);
        if (v_flag) {
            struct env_node_t *it;
            TAILQ_FOREACH (it, &env_list, next) {
                printf("%s\n",it->var_name);
            }
            exit (0);
        }
    } else {
        print_error("too many arguments");
        exit (EPERM);
    }
}
