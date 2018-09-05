#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>

typedef int bool;
#define true 1
#define false 0


static const struct option longOpts[] = {
    { "variables", no_argument, NULL, 'v'},
    { "help", no_argument, NULL, 'h'},
    { "version", no_argument, NULL, 'V'},
    { NULL, 0, NULL, 0},
};

typedef struct env_node_t {
    char * name;
    TAILQ_ENTRY (env_node_t) next;
} env_node_t;
TAILQ_HEAD (env_head_t, env_node_t);

typedef struct var_t {
    char *buf;
    int  len;
    bool has_braket;
    struct env_head_t list;
} var_t;

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


struct env_head_t env_list;

static int
is_variable_allowed_char(char c)
{
    return (isalnum(c) || c == '_');
}


static int
check_variable_name(char * var, int len)
{
    int cnt = 0;

    if (!isalpha(*var)) {
        return (0);
    }

    for (char *c = var; *c != '\0' && (!len || (cnt < len)); c++) {

        if(! is_variable_allowed_char(*c)) {
            return (len ? 0 : cnt);
        }
        cnt++;
    }
    return (cnt);
}

static int
do_expand(char * var)
{
    char *val = NULL;
    struct env_node_t *it;

    if (! TAILQ_EMPTY(&env_list)) {
        TAILQ_FOREACH (it, &env_list, next) {
            if (strcmp(var, it->name) == 0) {
                val = getenv(var);
            }
        }
    } else {
        val = getenv(var);
    }

    if (val) {
        fputs(val,stdout);
    }
    return val?1:0;
}

static void
expand (var_t *v)
{
    v->buf[v->len] = 0;
    if (v->len) {
        v->buf[v->len] = 0;
        do_expand(v->buf+((v->has_braket ? 2:1)));
    }
    v->len = 0;
    v->has_braket = false;
}

static void clean (var_t *v)
{
    if (v->len) {
        v->buf[v->len] = 0;
        fputs(v->buf, stdout);
    }
    v->len = 0;
    v->has_braket = false;
}

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
        if (strcmp(name, it->name) == 0) {
            free(name);
            return;
        }
    }

    node = malloc(sizeof(struct env_node_t));
    if(node == NULL) {
        exit (ENOMEM);
    }

    node->name = name;

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

void
eval_char(char  c, var_t * v)
{
    if(c  == '$') {
        if (!v->has_braket) {
            expand(v);
        } else {
            clean(v);
        }
        v->buf[v->len++] = c;
        return;
    }

    if (v->len == 1 && c == '{') {
        v->buf[v->len++] = c;
        v->has_braket = true;
        return;
    }

    if (v->len && v->has_braket && c == '}') {
        expand(v);
        return;
    }

    if (v->len) {
        if (is_variable_allowed_char(c)) {
            if (v->len < PATH_MAX) {
                v->buf[v->len++] = c;
                return;
            } else {
                clean(v);
            }
        } else {
            if (!v->has_braket) {
                expand(v);
            } else {
                clean(v);
            }
        }
    }

    putc(c, stdout);
}

int
subst_stdin()
{
    char varbuf[PATH_MAX + 1];
    var_t var = { varbuf, 0, false };
    char c;

    while((c = getc(stdin)) != EOF) {
        eval_char(c, &var);
    }
    if (var.len) {
        if (! var.has_braket) {
            expand(&var);
        } else {
            clean(&var);
        }
    }
    if (ferror(stdin)) {
        print_error("error while reading from the \"standard input\"");
        /* Don't die here, we must clean the variable list before */
        return (-EXIT_FAILURE);
    }
    return 1;
}

void
sort(struct env_head_t * list)
{
    int change;
    struct env_node_t *prev, *it, *nxt;

    do {
        prev = NULL;
        change = 0;
        TAILQ_FOREACH_SAFE (it, list, next, nxt) {
            if (prev && strcmp(prev->name, it->name) > 0) {
                TAILQ_REMOVE(list, prev, next);
                TAILQ_INSERT_AFTER(list, it, prev, next);
                change++;
            }
            prev = it;
        }
    } while (change);
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
           "Operation mode:\n"
           "  -v, --variables             output the variables occurring in SHELL-FORMAT\n\n"
           "Informative output:\n"
           "  -h, --help                  display this help and exit\n"
           "  -V, --version               output version information and exit\n\n"
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
           "Written by Rodrigo Osorio <rodrigo@freebsd.org>\n",
           getprogname());

}

int
main(int argc, char * argv[])
{
    int ch  = 0;
    int v_flag = 0;
    int longIndex;

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
            print_info("Try '%s --help' for more information", getprogname());
            exit (EPERM);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0 && v_flag) {
        print_error("missing arguments");
        exit (EPERM);
    }

    if (argc > 1) {
        print_error("too many arguments");
        exit (EPERM);
    }

    /* If a shell format variable is passed, parse it */
    if (argc == 1) {
        parse_shell_string(&env_list, argv[0]);
        sort(&env_list);
        if (v_flag) {
            struct env_node_t *it;
            TAILQ_FOREACH (it, &env_list, next) {
                printf("%s\n",it->name);
            }
            exit (0);
        }
    }

    if (! v_flag) {
        if( subst_stdin() < 0) {
            exit (EXIT_FAILURE);
        }
    }

    exit (EXIT_SUCCESS);
}
