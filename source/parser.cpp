#include "parser.h"
#include <cstdlib>
#include <functional>

void PrintToken(token *Token);
static void error(const char *S, token &T) {
  printf("Error:%d:%d: %s\n", T.Line, T.Offset, S);
  PrintToken(&T);
  exit(-1);
}

parser::parser(lexer_state &L) : Lex(L) { SymbolTable = Lex.Table; }

void parser::Match(int T) {
  if (T == Token.Type) {
    Token = Lex.GetToken();
  } else {
    printf("Expected %d:%c\n", T, T);
    token N = Lex.PeekToken();
    printf("Next token ");
    PrintToken(&N);
    error("Unexpected token", Token);
  }
}

parse_node parser::ParseTypeSpecifier() {
  parse_node N;
  if (IsPrecisionQualifier(Token.Type)) {
    N.Children.push_back(ParsePrecisionQualifier());
  }
  N.Children.push_back(ParseTypeSpecifierNoPrecision());
  return N;
}

parse_node parser::ParseTypeQualifier() {
  parse_node N;
  if (Token.Type == token::INVARIANT) {
    N.Children.push_back(parse_node(Token));
    Match(token::INVARIANT);
    N.Children.push_back(parse_node(Token));
    Match(token::VARYING);
  } else if (IsTypeQualifier(Token.Type)) {
    N.Children.push_back(parse_node(Token));
    Match(Token.Type);
  } else {
    error("Unexpected token", Token);
  }

  return N;
}

parse_node parser::ParseFullySpecifiedType() {
  parse_node N;
  if (IsTypeQualifier(Token.Type)) {
    N.Children.push_back(ParseTypeQualifier());
  }
  N.Append(ParseTypeSpecifier());
  return N;
}

parse_node parser::ParseParameterDeclaration() {}

parse_node parser::ParseFunctionHeaderWithParameters() {}

parse_node parser::ParseFunctionHeader() {
  parse_node N;
  N.Append(ParseFullySpecifiedType());
  N.Children.push_back(Token);
  Match(token::IDENTIFIER);
  N.Children.push_back(parse_node(Token));
  Match(token::LEFT_PAREN);
  return N;
}

parse_node parser::ParseFunctionDeclarator() {
  parse_node N = ParseFunctionHeader();
  if (Token.Type != token::RIGHT_PAREN) {
    N.Children.push_back(ParseParameterDeclaration());
  } else {
    N.Children.push_back(parse_node());
  }
  return N;
}

parse_node parser::ParseFunctionPrototype() {
  parse_node N;
  N.Append(ParseFunctionDeclarator());
  N.Children.push_back(parse_node(Token));
  Match(token::RIGHT_PAREN);
  return N;
}

bool parser::IsAssignmentOp(int T) {
  switch (T) {
  case token::EQUAL:
  case token::MUL_ASSIGN:
  case token::DIV_ASSIGN:
  case token::ADD_ASSIGN:
  case token::SUB_ASSIGN:
  case token::MOD_ASSIGN:
  case token::LEFT_ASSIGN:
  case token::RIGHT_ASSIGN:
  case token::AND_ASSIGN:
  case token::XOR_ASSIGN:
  case token::OR_ASSIGN:
    return true;
  default:
    return false;
  }
}

bool parser::IsTypeQualifier(int T) {
  switch (T) {
  case token::CONST:
  case token::ATTRIBUTE:
  case token::VARYING:
  case token::INVARIANT:
  case token::UNIFORM:
    return true;
  }

  return false;
}

bool parser::IsPrecisionQualifier(int T) {
  switch (T) {
  case token::HIGH_PRECISION:
  case token::MEDIUM_PRECISION:
  case token::LOW_PRECISION:
    return true;
  }
  return false;
}

parse_node parser::ParseAssignmentOperator() {
  switch (Token.Type) {
  case token::EQUAL:
  case token::MUL_ASSIGN:
  case token::DIV_ASSIGN:
  case token::ADD_ASSIGN:
  case token::SUB_ASSIGN:

  case token::MOD_ASSIGN:
  case token::LEFT_ASSIGN:
  case token::RIGHT_ASSIGN:
  case token::AND_ASSIGN:
  case token::XOR_ASSIGN:
  case token::OR_ASSIGN: {
    parse_node N = parse_node(Token);
    Match(Token.Type);
    return N;
  }

  default:
    error("Unexpected token", Token);
    return parse_node();
  }
}

parse_node parser::ParseFunctionCall() {
  // TODO
  return parse_node();
}

parse_node parser::ParseIntegerExpression() { return ParseExpression(); }

parse_node parser::ParseOperatorExpression(ParseFuncPtr R,
                                           std::vector<int> TokenTypes) {
#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))
  auto IsOfType = [](int T, std::vector<int> &v) -> bool {
    for (int &i : v) {
      if (i == T)
        return true;
    }
    return false;
  };
  std::function<parse_node(parse_node &)> ParseExtL =
      [&](parse_node &P) -> parse_node {
    parse_node N;
    if (IsOfType(Token.Type, TokenTypes)) {
      N.Children.push_back(P);
      N.Children.push_back(parse_node(Token));
      Match(Token.Type);
      N.Children.push_back(CALL_MEMBER_FN(*this, R)());
      parse_node C = ParseExtL(N);
      if (!C.Empty()) {
        return C;
      }
    }
    return N;
  };
  parse_node RP = CALL_MEMBER_FN(*this, R)();
  parse_node LP = ParseExtL(RP);
  if (!LP.Empty()) {
    return LP;
  }
  return RP;
#undef CALL_MEMBER_FN
}

parse_node parser::ParseMultiplicativeExpression() {
  return ParseOperatorExpression(&parser::ParseUnaryExpression,
                                 {token::STAR, token::SLASH, token::PERCENT});
}

parse_node parser::ParseAdditiveExpression() {
  return ParseOperatorExpression(&parser::ParseMultiplicativeExpression,
                                 {token::PLUS, token::DASH});
}

parse_node parser::ParseShiftExpression() {
  return ParseOperatorExpression(&parser::ParseAdditiveExpression,
                                 {token::LEFT_OP, token::RIGHT_OP});
}

parse_node parser::ParseRelationalExpression() {
  return ParseOperatorExpression(
      &parser::ParseShiftExpression,
      {token::LEFT_ANGLE, token::RIGHT_ANGLE, token::LE_OP, token::GE_OP});
}

parse_node parser::ParseEqualityExpression() {
  return ParseOperatorExpression(&parser::ParseRelationalExpression,
                                 {token::EQ_OP, token::NE_OP});
}

parse_node parser::ParseAndExpression() {
  return ParseOperatorExpression(&parser::ParseEqualityExpression,
                                 {token::AMPERSAND});
}

parse_node parser::ParseExclusiveOrExpression() {
  return ParseOperatorExpression(&parser::ParseAndExpression, {token::CARET});
}

parse_node parser::ParseInclusiveOrExpression() {
  return ParseOperatorExpression(&parser::ParseExclusiveOrExpression,
                                 {token::VERTICLE_BAR});
}

parse_node parser::ParseLogicalAndExpression() {
  return ParseOperatorExpression(&parser::ParseInclusiveOrExpression,
                                 {token::AND_OP});
}

parse_node parser::ParseLogicalXOrExpression() {
  return ParseOperatorExpression(&parser::ParseLogicalAndExpression,
                                 {token::XOR_OP});
}

parse_node parser::ParseLogicalOrExpression() {
  return ParseOperatorExpression(&parser::ParseLogicalXOrExpression,
                                 {token::OR_OP});
}

parse_node parser::ParsePrimaryExpression() {
  parse_node N;
  if (Token.Type == token::LEFT_PAREN) {
    Match(token::LEFT_PAREN);
    N.Children.push_back(ParseExpression());
    Match(token::RIGHT_PAREN);
    return N;
  }
  switch (Token.Type) {
  case token::INTCONSTANT:
  case token::FLOATCONSTANT:
  case token::BOOLCONSTANT:
  case token::IDENTIFIER:
    N = parse_node(Token);
    Match(Token.Type);
    return N;

  default: {
    error("Unexpected token", Token);
    return parse_node();
  }
  }
}

parse_node parser::ParsePostfixExpression() {
  std::function<parse_node(parse_node &)> ParseExtPostfix =
      [&](parse_node &P) -> parse_node {
    parse_node N;
    if (Token.Type == token::LEFT_BRACKET) {
      N.Children.push_back(P);
      Match(token::LEFT_BRACKET);
      N.Children.push_back(ParseIntegerExpression());
      Match(token::RIGHT_BRACKET);
    } else if (Token.Type == token::DEC_OP || Token.Type == token::INC_OP) {
      N.Children.push_back(parse_node(Token));
      Match(Token.Type);
    } else if (Token.Type == token::DOT) {
      N.Children.push_back(parse_node(Token));
      Match(token::DOT);
      N.Children.push_back(parse_node(Token));
      Match(token::FIELD_SELECTION);
    }
    return N;
  };
  parse_node Main;
  if (Lex.PeekToken().Type == token::LEFT_PAREN) {
    Main = ParseFunctionCall();
  } else {
    Main = ParsePrimaryExpression();
  }
  parse_node Ext = ParseExtPostfix(Main);
  if (!Ext.Empty()) {
    return Ext;
  }
  return Main;
}

parse_node parser::ParseUnaryExpression() {
  parse_node N;
  switch (Token.Type) {
  case token::INC_OP:
  case token::DEC_OP:
  case token::PLUS:
  case token::DASH:
  case token::BANG:
  case token::TILDE:
    N.Children.push_back(parse_node(Token));
    Match(Token.Type);
    N.Children.push_back(ParseUnaryExpression());
    return N;
  }
  N.Children.push_back(ParsePostfixExpression());
  return N;
}

parse_node parser::ParseConditionalExpression() {
  parse_node N;
  parse_node L = ParseLogicalOrExpression();
  if (Token.Type != token::QUESTION) {
    return L;
  }
  Match(token::QUESTION);
  N.Children.push_back(ParseExpression());
  N.Children.push_back(parse_node(Token));
  Match(token::COLON);
  N.Children.push_back(ParseAssignmentExpression());
  return N;
}

parse_node parser::ParseAssignmentExpression() {
  lexer_state S(Lex);
  token Tok(Token);
  parse_node U = ParseUnaryExpression();
  if (!IsAssignmentOp(Token.Type)) {
    Lex = S;
    Token = Tok;
    parse_node Cond = ParseConditionalExpression();
    if (!IsAssignmentOp(Token.Type))
      return Cond;
    U = Cond;
  }
  parse_node N;
  N.Children.push_back(U);

  N.Children.push_back(ParseAssignmentOperator());
  N.Children.push_back(ParseAssignmentExpression());
  return N;
}

parse_node parser::ParsePrecisionQualifier() {
  if (IsPrecisionQualifier(Token.Type)) {
    parse_node N = parse_node(Token);
    Match(Token.Type);
    return N;
  }

  error("Unexpected token", Token);
  return parse_node();
}

parse_node parser::ParseStructDeclarator() {
  parse_node N;
  N.Children.push_back(parse_node(Token));
  Match(token::IDENTIFIER);
  if (Token.Type == token::LEFT_BRACKET) {
    N.Children.push_back(parse_node(Token));
    Match(token::LEFT_BRACKET);
    N.Children.push_back(ParseConstantExpression());
    N.Children.push_back(parse_node(Token));
    Match(token::RIGHT_BRACKET);
  }
  return N;
}

parse_node parser::ParseStructDeclaratorList() {
  parse_node N;
  N.Children.push_back(ParseStructDeclarator());
  while (Token.Type == token::COMMA) {
    N.Children.push_back(ParseStructDeclarator());
  }
  return N;
}

parse_node parser::ParseStructDeclaration() {
  parse_node N;
  N.Children.push_back(ParseTypeSpecifier());
  N.Children.push_back(ParseStructDeclaratorList());
  N.Children.push_back(parse_node(Token));
  Match(token::SEMICOLON);
  return N;
}

parse_node parser::ParseStructDeclarationList() {
  parse_node N;
  while (Token.Type != token::RIGHT_BRACE) {
    N.Children.push_back(ParseStructDeclaration());
  }
  return N;
}

parse_node parser::ParseStructSpecifier() {
  parse_node N;
  N.Children.push_back(parse_node(Token));
  Match(token::STRUCT);
  if (Token.Type == token::IDENTIFIER) {
    N.Children.push_back(parse_node(Token));
    Match(token::IDENTIFIER);
  }
  N.Children.push_back(parse_node(Token));
  Match(token::LEFT_BRACE);
  N.Children.push_back(ParseStructDeclarationList());
  N.Children.push_back(parse_node(Token));
  Match(token::RIGHT_BRACE);
  return N;
}

parse_node parser::ParseTypeSpecifierNoPrecision() {
  switch (Token.Type) {
  case token::VOID:
  case token::FLOAT:
  case token::INT:
  case token::BOOL:
  case token::VEC2:
  case token::VEC3:
  case token::VEC4:
  case token::BVEC2:
  case token::BVEC3:
  case token::BVEC4:
  case token::IVEC2:
  case token::IVEC3:
  case token::IVEC4:
  case token::MAT2:
  case token::MAT3:
  case token::MAT4:
  case token::SAMPLER2D:
  case token::SAMPLERCUBE:
  case token::TYPE_NAME: {
    parse_node N = parse_node(Token);
    Match(Token.Type);
    return N;
  }

  case token::STRUCT:
    return ParseStructSpecifier();
  }

  error("Expected type specifier", Token);
  return parse_node();
}

parse_node parser::ParseInitializer() { return ParseAssignmentExpression(); }

parse_node parser::ParseConstantExpression() {
  return ParseConditionalExpression();
}

parse_node parser::ParseExpression() {
  parse_node N;
expr_reset:
  N.Append(ParseAssignmentExpression());
  if (Token.Type == token::COMMA) {
    N.Children.push_back(parse_node(Token));
    Match(token::COMMA);
    goto expr_reset;
  }
  return N;
}

parse_node parser::ParseExpressionStatement() {
  parse_node N;
  if (Token.Type != token::SEMICOLON) {
    N.Append(ParseExpression());
  }
  Match(token::SEMICOLON);
  return N;
}

parse_node parser::ParseSimpleStatement() {
  // TODO other simple statements
  return ParseExpressionStatement();
}

parse_node parser::ParseCompoundStatementWithScope() {
  parse_node N;
  N.Children.push_back(Token);
  Match(token::LEFT_BRACE);
  if (Token.Type != token::RIGHT_BRACE) {
    N.Children.push_back(ParseStatementList());
  }
  N.Children.push_back(Token);
  Match(token::RIGHT_BRACE);
  return N;
}

parse_node parser::ParseStatementNoNewScope() {
  if (Token.Type == token::LEFT_BRACE) {
    return ParseCompoundStatementWithScope();
  }
  return ParseSimpleStatement();
}

parse_node parser::ParseStatementList() {
  parse_node N;
  while (Token.Type != token::RIGHT_BRACE) {
    N.Children.push_back(ParseStatementNoNewScope());
  }
  return N;
}

parse_node parser::ParseCompoundStatementNoNewScope() {
  parse_node N;
  N.Children.push_back(Token);
  Match(token::LEFT_BRACE);
  if (Token.Type != token::RIGHT_BRACE) {
    N.Children.push_back(ParseStatementList());
  } else {
    N.Children.push_back(parse_node());
  }
  N.Children.push_back(Token);
  Match(token::RIGHT_BRACE);
  return N;
}

parse_node parser::ParseFunctionDefinition() {
  parse_node N;
  N.Append(ParseFunctionPrototype());
  N.Append(ParseCompoundStatementNoNewScope());
  return N;
}

parse_node parser::ParseSingleDeclaration() {
  parse_node N;
  N.Children.push_back(ParseFullySpecifiedType());
  N.Children.push_back(parse_node(Token));
  Match(token::IDENTIFIER);
  return N;
}

parse_node parser::ParseInitDeclaratorList() {
  parse_node N;
  N.Children.push_back(ParseSingleDeclaration());
  if (Token.Type != token::COMMA)
    return N;
  while (Token.Type == token::COMMA) {
    N.Children.push_back(parse_node(Token));
    Match(token::COMMA);
    N.Children.push_back(parse_node(Token));
    Match(token::IDENTIFIER);
    if (Token.Type == token::LEFT_BRACKET) {
      N.Children.push_back(parse_node(Token));
      Match(token::LEFT_BRACKET);
      N.Children.push_back(ParseConstantExpression());
      N.Children.push_back(parse_node(Token));
      Match(token::RIGHT_BRACKET);
    } else if (Token.Type == token::EQUAL) {
      N.Children.push_back(parse_node(Token));
      Match(token::EQUAL);
      N.Children.push_back(ParseInitializer());
    }
  }
  return N;
}

parse_node parser::ParseDeclaration() {
  if (Token.Type == token::PRECISION) {
    parse_node N;
    N.Children.push_back(parse_node(Token));
    Match(token::PRECISION);
    N.Children.push_back(ParsePrecisionQualifier());
    N.Children.push_back(ParseTypeSpecifierNoPrecision());
    Match(token::SEMICOLON);
    return N;
  }
  lexer_state S(Lex);
  token Tok(Token);
  ParseSingleDeclaration();
  if (Token.Type == token::LEFT_PAREN) {
    Lex = S;
    Token = Tok;
    return ParseFunctionPrototype();
  }

  Lex = S;
  Token = Tok;
  parse_node N = ParseInitDeclaratorList();
  N.Children.push_back(parse_node(Token));
  Match(token::SEMICOLON);
  return N;
}

parse_node parser::ParseExternalDeclaration() {
  lexer_state S(Lex);
  token Tok(Token);
  ParseSingleDeclaration();
  if (Token.Type == token::LEFT_PAREN) {
    Lex = S;
    Token = Tok;
    ParseFunctionPrototype();
    if (Token.Type == token::LEFT_BRACE) {
      Lex = S;
      Token = Tok;
      return ParseFunctionDefinition();
    }
  }
  Lex = S;
  Token = Tok;
  return ParseDeclaration();
}

parse_node parser::ParseTranslationUnit() {
  Token = Lex.GetToken();
  parse_node N;
  while (Token.Type != token::END) {
    N.Children.push_back(ParseExternalDeclaration());
  }

  return N;
}
