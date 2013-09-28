#ifndef BML_PARSER_H
#define BML_PARSER_H

#include <vector>

class Bml_Node
{
    char * name;
    char * value;

    std::vector<Bml_Node> children;

    static Bml_Node emptyNode;

public:
    Bml_Node();
    Bml_Node(Bml_Node const& in);

    ~Bml_Node();

    void clear();

    void setLine(const char * line);
    void addChild(Bml_Node const& child);

    const char * getName() const;
    const char * getValue() const;

    size_t getChildCount() const;
    Bml_Node const& getChild(size_t index) const;

    Bml_Node & walkToNode( const char * path );
    Bml_Node const& walkToNode( const char * path ) const;
};

class Bml_Parser
{
    Bml_Node document;

public:
    Bml_Parser() { }

    void parseDocument(const char * document);

    const char * enumValue(const char * path) const;

#if 0
    void print(Bml_Node const* node = 0, unsigned int indent = 0) const;
#endif
};

#endif // BML_PARSER_H
