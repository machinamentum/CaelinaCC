
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <vector>

struct parse_node {

  enum { E, T };

  std::vector<parse_node> Children;
  token Token;
  int Type;

  parse_node() {
      Type = E;
  }

  parse_node(int Tok) {
    Type = T;
    Token.Type = Tok;
  }

  parse_node(token &Tok) {
    Token = Tok;
    Type = T;
  }

  void Append(const parse_node &P) {
    Children.insert(Children.end(), P.Children.begin(), P.Children.end());
  }

  bool Empty() {
      return Children.size() == 0 && Type == E;
  }
};

struct parser {
  lexer_state &Lex;
  token Token;
  symtable *SymbolTable;

  typedef parse_node (parser::*ParseFuncPtr)();

  parser(lexer_state &L);
  void Match(int T);

  static bool IsAssignmentOp(int T);

  parse_node ParseTypeSpecifier();
  parse_node ParseFullySpecifiedType();
  parse_node ParseParameterDeclaration();
  parse_node ParseFunctionHeaderWithParameters();
  parse_node ParseFunctionHeader();
  parse_node ParseFunctionDeclarator();
  parse_node ParseFunctionPrototype();
  parse_node ParseFunctionDefinition();

  parse_node ParseFunctionCall();
  parse_node ParseIntegerExpression();
  parse_node ParsePostfixExpression();
  parse_node ParsePrimaryExpression();
  parse_node ParseUnaryExpression();
  parse_node ParseOperatorExpression(ParseFuncPtr R, std::vector<int> TokenTypes);
  parse_node ParseEqualityExpression();
  parse_node ParseMultiplicativeExpression();
  parse_node ParseAdditiveExpression();
  parse_node ParseRelationalExpression();
  parse_node ParseShiftExpression();
  parse_node ParseAndExpression();
  parse_node ParseExclusiveOrExpression();
  parse_node ParseInclusiveOrExpression();
  parse_node ParseLogicalAndExpression();
  parse_node ParseLogicalXOrExpression();
  parse_node ParseAssignmentOperator();
  parse_node ParseLogicalOrExpression();
  parse_node ParseConditionalExpression();
  parse_node ParseAssignmentExpression();
  parse_node ParseExpression();
  parse_node ParseExpressionStatement();
  parse_node ParseSimpleStatement();
  parse_node ParseStatementNoNewScope();
  parse_node ParseCompoundStatementWithScope();
  parse_node ParseStatementList();
  parse_node ParseCompoundStatementNoNewScope();

  parse_node Parse();
};


#endif
