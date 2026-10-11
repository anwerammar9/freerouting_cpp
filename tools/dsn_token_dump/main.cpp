// Dumps the raw token stream of the C++ DSN lexer in the format of the Java
// reference harness (K:<keyword> / S:<string> / I:<int> / D:<double> / E:<error>),
// so the two lexers can be compared line by line (tools/dsn_token_compare.py).

#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "designforms/specctra/parser/Keyword.h"
#include "designforms/specctra/parser/SpecctraDsnStreamReader.h"

using freerouting::designforms::specctra::parser::SpecctraDsnStreamReader;
using freerouting::designforms::specctra::parser::Token;

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "usage: dsn_token_dump <in.dsn> <out.tok>\n";
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in)
    {
        std::cerr << "cannot open " << argv[1] << "\n";
        return 1;
    }
    std::ofstream out(argv[2], std::ios::binary);
    SpecctraDsnStreamReader scanner(in);
    for (;;)
    {
        Token token;
        try
        {
            token = scanner.next_token();
        }
        catch (const std::exception& e)
        {
            out << "E:" << e.what() << "\n";
            break;
        }
        if (token.is_eof())
        {
            break;
        }
        switch (token.kind)
        {
        case Token::Kind::KEYWORD: out << "K:" << token.keyword->get_name() << "\n"; break;
        case Token::Kind::STRING: out << "S:" << token.string_value << "\n"; break;
        case Token::Kind::INTEGER: out << "I:" << token.int_value << "\n"; break;
        case Token::Kind::DOUBLE:
        {
            char buf[64];
            std::snprintf(buf, sizeof buf, "%.17g", token.double_value);
            out << "D:" << buf << "\n";
            break;
        }
        default: break;
        }
    }
    return 0;
}
