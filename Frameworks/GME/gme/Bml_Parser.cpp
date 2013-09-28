#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "Bml_Parser.h"

Bml_Node Bml_Node::emptyNode;

Bml_Node::Bml_Node()
{
    name = 0;
    value = 0;
}

Bml_Node::Bml_Node(const Bml_Node &in)
{
    size_t length;
    name = 0;
    if (in.name)
    {
        length = strlen(in.name);
        name = new char[length + 1];
        memcpy(name, in.name, length + 1);
    }
    value = 0;
    if (in.value)
    {
        length = strlen(in.value);
        value = new char[length + 1];
        memcpy(value, in.value, length + 1);
    }
    children = in.children;
}

Bml_Node::~Bml_Node()
{
    delete [] name;
    delete [] value;
}

void Bml_Node::clear()
{
    delete [] name;
    delete [] value;

    name = 0;
    value = 0;
    children.resize( 0 );
}

void Bml_Node::setLine(const char *line)
{
    delete [] name;
    delete [] value;

    name = 0;
    value = 0;

    const char * line_end = strchr(line, '\n');
    if ( !line_end ) line_end = line + strlen(line);

    const char * first_letter = line;
    while ( first_letter < line_end && *first_letter <= 0x20 ) first_letter++;

    const char * colon = strchr(first_letter, ':');
    if (colon >= line_end) colon = 0;
    const char * last_letter = line_end - 1;

    if (colon)
    {
        const char * first_value_letter = colon + 1;
        while (first_value_letter < line_end && *first_value_letter <= 0x20) first_value_letter++;
        last_letter = line_end - 1;
        while (last_letter > first_value_letter && *last_letter <= 0x20) last_letter--;

        value = new char[last_letter - first_value_letter + 2];
        memcpy(value, first_value_letter, last_letter - first_value_letter + 1);
        value[last_letter - first_value_letter + 1] = '\0';

        last_letter = colon - 1;
    }

    while (last_letter > first_letter && *last_letter <= 0x20) last_letter--;

    name = new char[last_letter - first_letter + 2];
    memcpy(name, first_letter, last_letter - first_letter + 1);
    name[last_letter - first_letter + 1] = '\0';
}

void Bml_Node::addChild(const Bml_Node &child)
{
    children.push_back(child);
}

const char * Bml_Node::getName() const
{
    return name;
}

const char * Bml_Node::getValue() const
{
    return value;
}

size_t Bml_Node::getChildCount() const
{
    return children.size();
}

Bml_Node const& Bml_Node::getChild(size_t index) const
{
    return children[index];
}

Bml_Node & Bml_Node::walkToNode(const char *path)
{
    Bml_Node * node = this;
    while ( *path )
    {
        bool item_found = false;
        const char * next_separator = strchr( path, ':' );
        if ( !next_separator ) next_separator = path + strlen(path);
        for ( std::vector<Bml_Node>::iterator it = node->children.end(); it != node->children.begin(); )
        {
            --it;
            if ( next_separator - path == strlen(it->name) &&
                 strncmp( it->name, path, next_separator - path ) == 0 )
            {
                node = &(*it);
                item_found = true;
                break;
            }
        }
        if ( !item_found ) return emptyNode;
        if ( *next_separator )
        {
            path = next_separator + 1;
        }
        else break;
    }
    return *node;
}

Bml_Node const& Bml_Node::walkToNode(const char *path) const
{
    Bml_Node const* next_node;
    Bml_Node const* node = this;
    while ( *path )
    {
        bool item_found = false;
        size_t array_index = ~0;
        const char * array_index_start = strchr( path, '[' );
        const char * next_separator = strchr( path, ':' );
        if ( !next_separator ) next_separator = path + strlen(path);
        if ( array_index_start && array_index_start < next_separator )
        {
            char * temp;
            array_index = strtoul( array_index_start + 1, &temp, 10 );
        }
        else
        {
            array_index_start = next_separator;
        }
        for ( std::vector<Bml_Node>::const_iterator it = node->children.begin(), ite = node->children.end(); it != ite; ++it )
        {
            if ( array_index_start - path == strlen(it->name) &&
                 strncmp( it->name, path, array_index_start - path ) == 0 )
            {
                next_node = &(*it);
                item_found = true;
                if ( array_index == 0 ) break;
                --array_index;
            }
        }
        if ( !item_found ) return emptyNode;
        node = next_node;
        if ( *next_separator )
        {
            path = next_separator + 1;
        }
        else break;
    }
    return *node;
}

void Bml_Parser::parseDocument( const char * source )
{
    std::vector<size_t> indents;
    std::string last_name;
    std::string current_path;

    document.clear();

    size_t last_indent = ~0;

    Bml_Node node;

    while ( *source )
    {
        const char * line_end = strchr( source, '\n' );
        if ( !line_end ) line_end = source + strlen( source );

        if ( node.getName() ) last_name = node.getName();

        node.setLine( source );

        size_t indent = 0;
        while ( source < line_end && *source <= 0x20 )
        {
            source++;
            indent++;
        }

        if ( last_indent == ~0 ) last_indent = indent;

        if ( indent > last_indent )
        {
            indents.push_back( last_indent );
            last_indent = indent;
            if ( current_path.length() ) current_path += ":";
            current_path += last_name;
        }
        else if ( indent < last_indent )
        {
            while ( last_indent > indent && indents.size() )
            {
                last_indent = *(indents.end() - 1);
                indents.pop_back();
                size_t colon = current_path.find_last_of( ':' );
                if ( colon != ~0 ) current_path.resize( colon );
                else current_path.resize( 0 );
            }
            last_indent = indent;
        }

        document.walkToNode( current_path.c_str() ).addChild( node );

        source = line_end;
        while ( *source && *source == '\n' ) source++;
    }
}

const char * Bml_Parser::enumValue(const char *path) const
{
    return document.walkToNode(path).getValue();
}

#if 0
void Bml_Parser::print(Bml_Node const* node, unsigned int indent) const
{
    if (node == 0) node = &document;

    for (unsigned i = 0; i < indent; ++i) printf("  ");

    printf("%s", node->getName());
    if (node->getValue()) printf(":%s", node->getValue());
    printf("\n");

    indent++;

    for (unsigned i = 0, j = node->getChildCount(); i < j; ++i)
    {
        Bml_Node const& child = node->getChild(i);
        print( &child, indent );
    }
}
#endif
