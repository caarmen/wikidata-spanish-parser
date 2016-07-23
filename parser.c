/**
 * -----------------------------------------------------------------------
 * Wikidata Spanish definition parser
 *
 * Copyright (c) 2016 Carmen Alvarez
 *
 * License: Do what you want with this, but be warned it's probably buggy. 
 * I don't guarantee that it works or does anything useful!
 *
 * -----------------------------------------------------------------------
 * This program parses a Wikidata dump file from https://dumps.wikimedia.org/wikidatawiki/entities/
 * and outputs a tab-separated-value file for the spanish definitions, of the format:
 *
 *     TAB-id-TAB-word-TAB-definition
 *
 * It relies on jsmn: http://zserge.com/jsmn.html
 *
 * Compilation: 
 *     # Build jsmn:
 *     hg clone http://bitbucket.org/zserge/jsmn jsmn
 *     cd jsmn
 *     make 
 *
 *     # Copy this file to the jsmn project folder (or set the -I and -L flags accordingly):
 *     gcc -g parser.c -o parser -ljsmn -L .
 *
 * Running:
 *     ./parser /path/to/wikidata-yyyyMMdd-all.json
 */
#include "jsmn.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

static void json_read_string(const char *line, jsmntok_t *token, char *out) {
    snprintf(out, token->end - token->start + 1, "%s", line + token->start);
} 

static bool json_equals(const char *line, jsmntok_t *token, const char *value) {
    return strncmp(line + token->start, value, token->end - token->start) == 0; 
}

static void dump_token(const char *line, jsmntok_t *token) {
    printf("%.*s\n", token->end - token->start, line+token->start);
}

static void json_read_attribute_value(const char *line, jsmntok_t *object_token, const char *attribute_name, char *attribute_value) {
    for (jsmntok_t *token = object_token; token->start < object_token->end; token++) {
        if (json_equals(line, token, attribute_name)) {
            jsmntok_t *value_token = token + 1;
            snprintf(attribute_value, value_token->end - value_token->start + 1, "%s", line + value_token->start);
        }
    }
}

static const char *json_get_type(jsmntok_t *token) {
    if (token->type == JSMN_PRIMITIVE) return "Primitive";
    if (token->type == JSMN_OBJECT) return "Object";
    if (token->type == JSMN_ARRAY) return "Array";
    if (token->type == JSMN_STRING) return "String";
    return "UNKNOWN";
}

int main(int argc, char **argv) {
    jsmn_parser parser;
    const char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    char *line = NULL;
    size_t linecap = 100000;
    ssize_t linelen;
    int MAX_TOKENS = 10000;
    jsmntok_t *tokens = (jsmntok_t*) malloc(MAX_TOKENS * sizeof(*tokens));
    while ((linelen = getline(&line, &linecap, file)) > 0) {
        jsmn_init(&parser);
        int num_tokens = jsmn_parse(&parser, line, strlen(line), tokens, MAX_TOKENS);
        char id[16];
        char word[64];
        char definition[1024];
        memset(id, 0, 16);
        memset(word, 0, 64);
        memset(definition, 0, 1024);
        for (int i=0; i < num_tokens; i++) {
            jsmntok_t *token = &tokens[i];
            if (token->type == JSMN_STRING) {
                // Get the word id
                if (strlen(id) == 0 && json_equals(line, token, "id") && tokens[i+1].type == JSMN_STRING) {
                    json_read_string(line, &tokens[++i], id);
                } else if (json_equals(line, token, "es") && tokens[i+1].type == JSMN_OBJECT) {
                    // Get the word
                    if (strlen(word) ==0) {
                        json_read_attribute_value(line, &tokens[i+1], "value", word);
                    } 
                    // Get the definition
                    else {
                        json_read_attribute_value(line, &tokens[i+1], "value", definition);
                    }
                }
            }
        }
        if (strlen(id) && strlen(word) && strlen(definition)) {
            printf("%s\t%s\t%s\n", id, word, definition);
        }
        linecap = 0;
        free(line);
        line = NULL;
    }
    free(tokens);
    return 0;
}
