/*
 *   $Id$
 *
 *   Copyright (c) 2000, Jesus Castagnetto <jmcastagnetto@zkey.com>
 *
 *   This source code is released for free distribution under the terms of the
 *   GNU General Public License.
 *
 *   This module contains functions for generating tags for the PHP web page
 *   scripting language. Only recognizes functions and classes, not methods or
 *   variables.
 *
 *   Parsing PHP defines by Pavel Hlousek <pavel.hlousek@seznam.cz>, Apr 2003.
 *   Rewrited from regex to parser by Danil Orlov <zargener@gmail.com>, Jun 2013.
 */

/*
 *   INCLUDE FILES
 */
#include "general.h"  /* must always come first */
#include "options.h"

#include <string.h>

#include "parse.h"
#include "read.h"
#include "vstring.h"
#include "entry.h"
#include <regex.h>


/* struct defines class name and position in file */
typedef struct  {
    char name[1024];
    unsigned int start;
    unsigned int end;
    unsigned int deep;
} class_nest;

/* struct defines namespace name and position in file */
typedef struct  {
    char name[1024];
    unsigned int start;
    unsigned int end;
    unsigned int deep;
} ns_nest;


/* maximum 1024 classes per file, sorry was too lazy to do it with linked list */
class_nest class_nests[1024];
class_nest ns_nests[1024];

/* for storing variables non bound to any class */
char free_vars[1024][1024];
int free_vars_count = 0;

/* global counter of found classes to know to which position next
 * found class will be written 
 */
int total_classes = 0;
int total_ns = 0;


/* this one defines data needed for creating extended attributes of tag. Not supported for now */
static kindOption PHPTypes[] = {
    {TRUE, 'c', "class",    "classes"},
    {TRUE, 'f', "function", "functions"},
    {TRUE, 'm', "member",   "class members"},
    {TRUE, 'v', "variable", "variables"},
    {TRUE, 'i', "namespace", "imports"}
};

/* this one defines data needed for creating extended attributes of tag. Not supported for now */
typedef enum {
    K_CLASS, K_FUNCTION, K_MEMBER, K_VARIABLE, K_IMPORT
} phpKind;


/**
 * List of PHP predefined variables
 */
static const char *php_predefined_vars[] = {
    "$GLOBALS",
    "$_SERVER",
    "$_GET",
    "$_POST",
    "$_FILES",
    "$_COOKIE",
    "$_SESSION",
    "$_REQUEST",
    "$_ENV"
};
static const char php_predefined_vars_count = 9;


/**
 * Gets lexem position in file and tries to find the nearest beginning
 * of class definition
 */
int find_ancestor(int fpos) {
    int i = 0;
    int minimal_gap = 1000000000;
    int ancestor = -1;
    for (i = 0; i < total_classes; i++) {
        if (
            class_nests[i].start <= fpos && !class_nests[i].end &&
            fpos - class_nests[i].start < minimal_gap
        ) {
            minimal_gap = fpos - class_nests[i].start;
            ancestor = i;
        }
    }

    return ancestor;
}


/**
 * Gets lexem namespace
 */
int find_namespace(int fpos) {
    int i = 0;
    int minimal_gap = 1000000000;
    int ns = -1;
    for (i = 0; i < total_ns; i++) {
        if (
            ns_nests[i].start <= fpos && !ns_nests[i].end &&
            fpos - ns_nests[i].start < minimal_gap
        ) {
            minimal_gap = fpos - ns_nests[i].start;
            ns = i;
        }
    }

    return ns;
}



/**
 * Be sure that var is not declared in some class
 */
int var_belongs_to_class(int fpos) {
    int i = 0;
    int class_var = 0;

    for (i = 0; i < total_classes; i++) {
        /* printf("%d %d %d\n", fpos, class_nests[i].start, class_nests[i].end); */
        if (
            fpos > class_nests[i].start && 
            ( !class_nests[i].end || fpos < class_nests[i].end )
        ) {
            class_var = 1;
            break;
        }
    }

    return class_var;
}


/**
 * Check if variable name is one of those global php variables which
 * are predefined
 */
int is_predefined_var(char* vname) {

    int i;
    char *cur_var;
    for (i = 0; i < php_predefined_vars_count; i++) {
        cur_var = (char*)php_predefined_vars[i];

        if (strncmp(cur_var, vname, 32) == 0) {
            return 1;
        }
    }


    return 0;
}


/**
 * Just shortcut for adding tag
 */
void PHPMakeTag(char *tagname, char* kname, char kind) {
    tagEntryInfo tag;
    int copy_start = 0;
    if (!Option.etags && tagname[0] == '$') {

        /* Do not index php standart vars */
        if (is_predefined_var(tagname)) {
            return;
        }

        copy_start = 1;
    }
    initTagEntry (&tag, &tagname[copy_start]);
	/* strncpy(tag.kindName, kname, 10); */
	tag.kind = kind;
    makeTagEntry (&tag);
}


/**
 * Index all found lexems with non zero size
 */
void handle_tokens(char *prev_token,char *token, unsigned int pos, unsigned int deep) {
    if (strlen(token) > 0) {

        /* index namespace */
        if (strncmp(prev_token, "namespace", 1023) == 0) {
            if (total_ns < 1024) {
                strncpy(ns_nests[total_ns].name, token, 1023);
                ns_nests[total_ns].deep = deep;
                ns_nests[total_ns].start = pos;
                ns_nests[total_ns].end = 0;
                total_ns++;
            }
            /* close previous namespace */
            if (total_ns > 1) {
                ns_nests[total_ns-1].end = pos;
            }
            PHPMakeTag(token, "namespace", 'i');
            
        }
        /* index class */
        if ( (strncmp(prev_token, "class",1023) == 0) ||
             (strncmp(prev_token, "interface",1023) == 0)
        )
        {
            if (total_classes < 1024) {
                strncpy(class_nests[total_classes].name, token, 1023);
                class_nests[total_classes].deep = deep;
                class_nests[total_classes].start = pos;
                class_nests[total_classes].end = 0;
                total_classes++;
            }
            PHPMakeTag(token, "class", 'c');
        }

        /* index function */
        else if (strncmp(prev_token, "function",1023) == 0)
        {
            int ancestor = find_ancestor(pos);
            char tagname[1024];

            if (ancestor >= 0) {
                PHPMakeTag(token, "function", 'f');

                sprintf(tagname, "%s<%s>", token, class_nests[ancestor].name);
                PHPMakeTag(tagname, "function", 'f');
            }
            else {
                strncpy(tagname, token, 1023);
                PHPMakeTag(tagname, "function", 'f');
            }
        }

        /* index variables and consts */
        else if ( token[0] == '$' || strncmp(prev_token, "const", 16) == 0 ) {
            /* index class variables */
            if  (
                strncmp(prev_token, "var", 1023) == 0 ||
                strncmp(prev_token, "public", 1023) == 0 ||
                strncmp(prev_token, "static", 1023) == 0 ||
                strncmp(prev_token, "protected", 1023) == 0 ||
                strncmp(prev_token, "private", 1023) == 0
            )
            {
                int ancestor = find_ancestor(pos);
                char tagname[1024];

                if (ancestor >= 0) {
                    PHPMakeTag(token, "variable", 'v');
                    sprintf(tagname, "%s<%s>", token, class_nests[ancestor].name);
                    PHPMakeTag(tagname, "variable", 'v');
                } else {
                    strncpy(tagname, token, 1023);
                    PHPMakeTag(tagname, "variable", 'v');
                }
            }
            /* index other variables */
            else {
                /* first try to find this var in indexed */
                /* TODO: It will miss same named vars declared in one file */
                int j = 0;
                char var_exists = 0;
                for (j = 0; j < free_vars_count; j++) {
                    if (strncmp(free_vars[j], token, 127) == 0) {
                        var_exists = 1;
                        break;
                    }
                }

                if (!var_exists && 
                    !var_belongs_to_class(pos) &&
                    free_vars_count < 1024
                ) {
                    strncpy(free_vars[free_vars_count++], token, 127);

                    int ns = find_namespace(pos);
                    char tagname[1024];
                    if (ns >= 0) {
                        PHPMakeTag(token, "variable", 'v');

                        sprintf(tagname, "%s<%s>", token, ns_nests[ns].name);
                        PHPMakeTag(tagname, "variable", 'v');
                    }
                    else {
                        PHPMakeTag(token, "variable", 'v');
                    }
                }
            }
            
        }
    }
};


/**
 * called every time '}' encountered in file. Needed to calculate
 * class definition endings.
 */
void close_block_hook(unsigned int brace_deep, unsigned int fpos, int line_number) {

    int i = 0;
    int scope = 0;
    int minimal_scope = 1000000000;  /* MMMMAXIMUM HARDCOCE */
    int closing_block = -1;
    for (i = 0; i < total_classes; i++) {
        scope = fpos - class_nests[i].start;
        if (class_nests[i].deep == brace_deep && scope < minimal_scope) {
            minimal_scope = scope;
            closing_block = i;
        }
    }

    if (closing_block >= 0 && !class_nests[closing_block].end) {
        class_nests[closing_block].end = fpos;
    }

}

/**
 * Main parser function, called each time new php file being processed.
 */
static void findPHPTags (void) {

    /* must zero it for every file or you get wonderful class mixture
     * from different files referenced into one */
    total_classes = 0;
    total_ns = 0;
    free_vars_count = 0;
    
    char prev_char = 0;
    int curr_char = 0;

    int line_number = 0;
    unsigned int fpos = 0;
    unsigned int brace_deep = 0;


    char token[1024];
    char prev_token[1024];
    int token_len = 0;
    char in_php = 0;

    /* magic number 6 because of len of "<?php" */
    char cb[6];

#define CYCLE_BUFFER  cb[5] = 0; cb[0] = cb[1]; cb[1] = cb[2]; cb[2] = cb[3]; cb[3] = cb[4]; cb[4] = curr_char; 
    
    while ((curr_char = fileGetc()) != EOF )    {
        fpos++;
        CYCLE_BUFFER;

        /* skip all stuff between ?> <?php */
        if ( strncmp(cb, "?>", 2) == 0 ) {
            in_php = 0;
        }
        while ( !in_php && ( strncmp(cb, "<?php", 5) != 0 ) ) {
            curr_char = fileGetc();
            CYCLE_BUFFER;

            if (curr_char == EOF)
                break;
            prev_char = curr_char;
            fpos++;
        }
        in_php = 1;


        /* handle new lines */
        if (curr_char == '\n') {
            line_number++;
            handle_tokens(prev_token, token, fpos, brace_deep);
            strncpy(prev_token, token, 1023);
            token_len = 0; token[0] = 0x00;
            prev_char = curr_char;
            continue;
        }

        /* skip multiline comments  code start */
        if (curr_char == '*' && prev_char == '/' ) {
            /* printf ("ENTERED /\* ON %d\n", fpos); */
            while ( (curr_char = fileGetc()) != EOF) {
                fpos++;
                if (curr_char == '/' && prev_char == '*')
                    break;
                prev_char = curr_char;
            }
            /* printf ("EXITED /\* ON %d\n", fpos); */
            continue;
        }
        /* skip multiline comments  code end */

        /* skip one line comments */
        if ( 
            (curr_char == '/' && prev_char == '/')  || /* new style comments */
            (curr_char == '#')  /* old style comments */
        )
        {
            while ( (curr_char = fileGetc()) != EOF) {
                prev_char = curr_char;
                fpos++;
                if (curr_char == '\n')
                    break;
            }
            continue;
        }

        /* string literal skipping code start */
        if (curr_char == '\'') {
            /* printf ("ENTERED \' ON %d\n", fpos); */
            while ( (curr_char = fileGetc()) != EOF) {
                fpos++;
                if (curr_char == '\'' && prev_char != '\\')
                    break;
                prev_char = curr_char;
            }
            /* printf ("EXITED \' ON %d\n", fpos); */
            continue;
        }
        if (curr_char == '\"') {
            while ( (curr_char = fileGetc()) != EOF) {
                fpos++;
                if (curr_char == '\"' && prev_char != '\\')
                    break;
                prev_char = curr_char;
            }
            continue;
        }

        /* string literal skipping code end */


        /* detect braces to track how deep in code structure we are */
        if (curr_char == '{') {
            brace_deep++;
        }
        if (curr_char == '}') {
            brace_deep--;
            close_block_hook(brace_deep, fpos, line_number);
        }

        if (
            curr_char == ' ' || curr_char == '(' || curr_char == '\t' || curr_char == ';' ||
            curr_char == '=' || curr_char == '+' || curr_char == '-' || curr_char == '*' ||
            curr_char == '/' || curr_char == '[' || curr_char == '{' || curr_char == '}' ||
            curr_char == ')' || curr_char == '<' || curr_char == '.' || curr_char == ','
        ) {
            /* printf("%s %s\n", prev_token, token); */
            handle_tokens(prev_token, token, fpos, brace_deep);
            if (token_len > 0) {
                strncpy(prev_token, token, 1023);
                token_len = 0; token[0] = 0x00;
            }

            prev_char = curr_char;

            continue;
        }
        /* if we here we just ended reading of lexem like class, function or so*/
        token[token_len] = curr_char;
        token[++token_len] = 0x00;

        prev_char = curr_char;
    }

    /* int i = 0; */
    /* for (i = 0; i < total_classes; i++) */
    /* { */
    /*     printf("%s [%d,%d]\n", class_nests[i].name, class_nests[i].start, class_nests[i].end); */
    /* } */
}




/* Create parser definition structure */
extern parserDefinition* PhpParser (void)
{
    static const char *const extensions [] = { "php", "php3", "phtml", NULL };
    parserDefinition* def = parserNew ("PHP");
    def->extensions = extensions;
    def->kinds = PHPTypes;
    def->kindCount = KIND_COUNT (PHPTypes);
    def->parser = findPHPTags;
    return def;
}

/* vi:set tabstop=4 shiftwidth=4: */
