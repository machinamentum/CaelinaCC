#include <fstream>
#include "parser.h"

char *SlurpFile(const char *FilePath, long *FileSize) {
  std::ifstream is = std::ifstream(FilePath);
  is.seekg(0, std::ios::end);
  long Length = is.tellg();
  is.seekg(0, std::ios::beg);
  char *Buffer = new char[Length];
  is.read(Buffer, Length);
  is.close();

  if (FileSize)
    *FileSize = Length;
  return Buffer;
}

void PrintToken(token *Token) {
  switch (Token->Type) {
  case token::FLOAT:
    printf("%f\n", Token->FloatValue);
    break;

  case token::IDENTIFIER:
    printf("%s\n", Token->Id.c_str());
    break;

  case token::END:
    printf("EOF\n");
    break;

  default:
    printf("%c\n", Token->Type);
    break;
  }
}

void PrintParseTree(parse_node *Node, int Depth) {
  for (int i = 0; i < Depth; ++i) {
    printf("   ");
  }
  if (Node->Type == parse_node::E) {
    printf("E%d\n", Depth);
    for (int i = 0; i < Node->Children.size(); ++i) {
      PrintParseTree(&Node->Children[i], Depth + 1);
    }
  } else {
    printf("T%d: ", Depth);
    PrintToken(&Node->Token);
  }
}

int main(int argc, char **argv) {
  lexer_state Lexer;
  long Size;
  char *Source = SlurpFile(argv[1], &Size);
  LexerInit(&Lexer, Source, Source + Size);
  parse_node RootNode = ParserGetNode(&Lexer, token::END);
  PrintParseTree(&RootNode, 0);
  return 0;
}
