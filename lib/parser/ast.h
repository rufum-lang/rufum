#ifndef RUFUM_PARSER_AST_H
#define RUFUM_PARSER_AST_H

#define NODE_TYPE enum ast_node_type type;

enum ast_node_type
{
    AST_ROOT_NODE
};

typedef struct
{
    NODE_TYPE
} ast_node_t;

typedef struct
{
    NODE_TYPE
    ast_node_t *child;
} ast_root_node_t;



#endif
