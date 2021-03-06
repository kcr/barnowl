#include "owl.h"
#include "ztext.h"

#include <assert.h>

static const char MATCH[256] = {
    ['('] = ')',
    ['<'] = '>',
    ['['] = ']',
    ['{'] = '}',
};

enum { /*noproto*/
    NEXT, // next slot is next state
    TEXTC,
    LABELC,
    UNSAVE,
    EMIT,
    CLEAR,
    PUSH,
    POP,
    JNMATCH, // next slot is where to go if the stack is not a match
    JNSAVED, // ""
};

static const unsigned char machine[2][256][16] = {
    [0] = {
        [0 ... 255] = {TEXTC, NEXT, 0},
        [0] = {EMIT, NEXT, 0},
        ['@'] = {LABELC, NEXT, 1},
        [')'] = {JNMATCH, 5, EMIT, POP, NEXT, 0, TEXTC, NEXT, 0},
        ['>'] = {JNMATCH, 5, EMIT, POP, NEXT, 0, TEXTC, NEXT, 0},
        [']'] = {JNMATCH, 5, EMIT, POP, NEXT, 0, TEXTC, NEXT, 0},
        ['}'] = {JNMATCH, 5, EMIT, POP, NEXT, 0, TEXTC, NEXT, 0},
    },
    [1] = {
        [0 ... 255] = {UNSAVE, CLEAR, TEXTC, NEXT, 0},
        [0] = {UNSAVE, EMIT, NEXT, 0},
        ['@'] = {JNSAVED, 7, EMIT, CLEAR, PUSH, POP, NEXT, 0, UNSAVE, CLEAR, TEXTC, NEXT, 0},
        ['0' ... '9'] = {LABELC, NEXT, 1},
        ['A' ... 'Z'] = {LABELC, NEXT, 1},
        ['_'] = {LABELC, NEXT, 1},
        ['a' ... 'z'] = {LABELC, NEXT, 1},
        [')'] = {JNMATCH, 7, UNSAVE, CLEAR, EMIT, POP, NEXT, 0, UNSAVE, CLEAR, TEXTC, NEXT, 0},
        ['>'] = {JNMATCH, 7, UNSAVE, CLEAR, EMIT, POP, NEXT, 0, UNSAVE, CLEAR, TEXTC, NEXT, 0},
        [']'] = {JNMATCH, 7, UNSAVE, CLEAR, EMIT, POP, NEXT, 0, UNSAVE, CLEAR, TEXTC, NEXT, 0},
        ['}'] = {JNMATCH, 7, UNSAVE, CLEAR, EMIT, POP, NEXT, 0, UNSAVE, CLEAR, TEXTC, NEXT, 0},
        ['('] = {EMIT, PUSH, CLEAR, NEXT, 0},
        ['<'] = {EMIT, PUSH, CLEAR, NEXT, 0},
        ['['] = {EMIT, PUSH, CLEAR, NEXT, 0},
        ['{'] = {EMIT, PUSH, CLEAR, NEXT, 0},
    },
};

typedef struct _ztext_stack { /*noproto*/
    char close;
    ztext_env *env;
    struct _ztext_stack *next;
} ztext_stack;

static ztext_env *ztext_env_new(char *label, char opener)
{
    ztext_env *p = g_slice_new(ztext_env);
    p->label = label;
    p->opener = opener;
    p->closer = 0;
    p->content = NULL;
    p->tail = NULL;
    return p;
}

void ztext_env_free(ztext_env *env)
{
    if (env->label)
        g_free(env->label);
    for (ztext_node *next, *p = env->content; p != NULL; p = next) {
        next = p->next;
        if (p->type == ZTEXT_NODE_STRING)
            g_free(p->string);
        else if (p->type == ZTEXT_NODE_ENV)
            ztext_env_free(p->env);
        else
            assert(!"freeing ztext_node, type not a string or a leaf");
        g_slice_free(ztext_node, p);
    }
    g_slice_free(ztext_env, env);
}

static void ztext_env_append(ztext_env *env, ztext_node *node)
{
    if (env->tail == NULL)
        env->content = node;
    else
        env->tail->next = node;
    env->tail = node;
}

static ztext_node *ztext_node_new(ztext_env *e, char *s) {
    ztext_node *p = g_slice_new(ztext_node);
    assert((s != NULL || e != NULL) && !(s != NULL && e != NULL));

    if (s != NULL) {
        p->type = ZTEXT_NODE_STRING;
        p->string = s;
    }

    if (e != NULL) {
        p->type = ZTEXT_NODE_ENV;
        p->env = e;
    }

    p->next = NULL;

    return p;
}

ztext_env *ztext_tree(const char *s)
{
    int state = 0;
    int next;
    int len = strlen(s);
    char text[len + 1];
    char save[len + 1];
    int texti = 0;
    int savei = 0;
    ztext_stack *stack = NULL;
    ztext_stack *p;
    ztext_env *tree = ztext_env_new(g_strdup(""), 0);
    ztext_env *cur = tree;

    text[texti] = 0;
    save[savei] = 0;

    for(int i = 0; i <= len; i++) { /* <=: deliberately processing the '\0' */
        const unsigned char *line = machine[state][(unsigned)s[i]];
        next = -1;
        for (int ip = 0; next == -1; ip++) {
            switch (line[ip]) {
            case NEXT:
                next = line[ip + 1];
                break;
            case TEXTC:
                text[texti++] = s[i];
                text[texti] = 0;
                break;
            case LABELC:
                save[savei++] = s[i];
                save[savei] = 0;
                break;
            case UNSAVE:
                if (savei) {
                    texti += g_strlcpy(&text[texti], save,
                                       sizeof(text) - texti);
                }
                break;
            case EMIT:
                if (texti) {
                    texti = 0;
                    ztext_env_append(cur, ztext_node_new(NULL, g_strdup(text)));
                }
                break;
            case CLEAR:
                save[savei] = 0;
                savei = 0;
                break;
            case PUSH:
                p = g_slice_new(ztext_stack);
                p->close = MATCH[(unsigned)s[i]];
                p->env = cur;
                p->next = stack;
                stack = p;
                cur = ztext_env_new(g_strdup(save), s[i]);
                ztext_env_append(stack->env, ztext_node_new((void *)cur, NULL));
                break;
            case POP:
                cur->closer = s[i];
                cur = stack->env;
                p = stack;
                stack = p->next;
                g_slice_free(ztext_stack, p);
                break;
            case JNMATCH:
                if (stack == NULL || stack->close != s[i])
                    ip += line[ip + 1];
                else
                    ip += 1;
                break;
            case JNSAVED:
                if (savei > 1)
                    ip += line[ip + 1];
                else
                    ip += 1;
                break;
            default:
                assert(!"unknown operation in state machine");
                return NULL;
            }
        }
        state = next;
    }

    return tree;
}

char *ztext_strip(ztext_env *tree) {
    char *s, *t, *u;

    if (tree->closer == '@' && tree->content == NULL)
        return g_strdup("@");

    s = g_strdup("");

    for (ztext_node *p = tree->content; p != NULL; p = p->next) {
        t = s;
        if (p->type == ZTEXT_NODE_STRING) {
            s = g_strconcat(s, p->string, NULL);
            g_free(t);
        } else if (p->type == ZTEXT_NODE_ENV) {
            if (!(!strcasecmp(p->env->label, "@color") ||
                  !strcasecmp(p->env->label, "@font"))) {
                u = ztext_strip(p->env);
                s = g_strconcat(s, u, NULL);
                g_free(u);
                g_free(t);
            }
        }
    }

    return s;
}

static char *ztext_protect_string(const char *s)
{
    char buf[strlen(s)*4+1]; // worst case
    int i = 0;
    for(const char *p = s; *p; p++) {
        switch (*p) {
        case '}':
            i += g_snprintf(&buf[i], sizeof(buf) - i, "@(%c)", *p);
            break;
        case ')':
            i += g_snprintf(&buf[i], sizeof(buf) - i, "@<%c>", *p);
            break;
        case '>':
            i += g_snprintf(&buf[i], sizeof(buf) - i, "@[%c]", *p);
            break;
        case ']':
            i += g_snprintf(&buf[i], sizeof(buf) - i, "@{%c}", *p);
            break;
        default:
            buf[i++] = *p;
        }
    }
    buf[i] = 0;
    return g_strdup(buf);
}

static char *ztext_protect_tree(ztext_env *tree) {
    char *s, *t, *u;

    if (tree->closer == '@' && tree->content == NULL)
        return g_strdup("@@");

    if (tree->opener) {
        s = g_strdup_printf("%s%c", tree->label, tree->opener);
    } else {
        s = g_strdup("");
    }

    for (ztext_node *p = tree->content; p != NULL; p = p->next) {
        t = s;
        if (p->type == ZTEXT_NODE_STRING) {
            u = ztext_protect_string(p->string);
        } else if (p->type == ZTEXT_NODE_ENV) {
            u = ztext_protect_tree(p->env);
        } else {
            assert(!"unknown node type");
        }
        s = g_strconcat(s, u, NULL);
        g_free(u);
        g_free(t);
    }

    if (tree->opener) {
        t = s;
        s = g_strdup_printf("%s%c", s, MATCH[(unsigned)tree->opener]);
        g_free(t);
    }

    return s;
}

char *ztext_protect(const char *s)
{
    ztext_env *tree = ztext_tree(s);
    char *p = ztext_protect_tree(tree);
    ztext_env_free(tree);
    return p;
}
