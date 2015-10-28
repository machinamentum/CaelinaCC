#include "lexer.h"
#include <cstdlib>

void LexerInit(lexer_state *State, char *Source, char *End) {
  State->SourcePtr = State->CurrentPtr = Source;
  State->EndPtr = End - 1;
  State->LineCurrent = 1;
  State->OffsetCurrent = 0;
}

token LexerGetToken(lexer_state *State) {
  token ReturnToken;

  auto IsWhiteSpace = [](char C) {
    return (C == ' ') || (C == '\n') || (C == '\t') || (C == '\r') ||
           (C == '\f');
  };

  char *Current = State->CurrentPtr;
  if (Current >= State->EndPtr)
    return {token::END};
_CheckWhiteSpace:
  while (IsWhiteSpace(Current[0]) && (Current < State->EndPtr)) {
    ++State->OffsetCurrent;
    if (Current[0] == '\n') {
      ++State->LineCurrent;
      State->OffsetCurrent = 0;
    }
    ++Current;
  }

  if (Current[0] == '/' && (Current < State->EndPtr)) {
    if (Current[1] == '/') {
      while (*Current != '\n' && (Current < State->EndPtr)) {
        ++Current;
      }
      goto _CheckWhiteSpace;
    }
  }

  auto IsAsciiLetter = [](char C) {
    return (C == '_') || ((C >= 'A') && (C <= 'Z')) ||
           ((C >= 'a') && (C <= 'z'));
  };

  if (IsAsciiLetter(Current[0])) {
    auto IsAsciiLetterOrNumber = [](char C) {
      return (C == '_') || ((C >= '0') && (C <= '9')) ||
             ((C >= 'A') && (C <= 'Z')) || ((C >= 'a') && (C <= 'z'));
    };
    char *End = Current + 1;
    while (IsAsciiLetterOrNumber(*End) && (End < State->EndPtr)) {
      ++End;
    }
    ReturnToken.Id = std::string(Current, End - Current);
    ReturnToken.Type = token::IDENTIFIER;
    ReturnToken.Line = State->LineCurrent;
    ReturnToken.Offset = State->OffsetCurrent;
    State->OffsetCurrent += End - Current;
    Current = End;
    goto _Exit;
  }

  static auto IsNumberOrDot =
      [](char C) { return ((C >= '0') && (C <= '9')) || (C == '.'); };

  if (IsNumberOrDot(Current[0])) {
    bool IsFloat = false;
    char *End = Current;
    while (IsNumberOrDot(*End) && (End < State->EndPtr)) {
      if (*End == '.') {
        IsFloat = true;
        break;
      }
      ++End;
    }
    if (IsFloat) {
      ReturnToken.Type = token::FLOAT;
      ReturnToken.FloatValue = strtod(Current, &End);
    } else {
      ReturnToken.Type = token::INT;
      ReturnToken.IntValue = strtoul(Current, &End, 10);
    }
    ReturnToken.Line = State->LineCurrent;
    ReturnToken.Offset = State->OffsetCurrent;
    State->OffsetCurrent += End - Current;
    Current = End;
    goto _Exit;
  }

  static auto GetEsacpedChar = [](char Char) {
    switch (Char) {
    case 't':
      return '\t';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 'f':
      return '\f';
    case '"':
      return '\"';
    case '\'':
      return '\'';
    case '\\':
      return '\\';
    }

    return Char;
  };
  if (Current[0] == '\"') {
    ReturnToken.Type = token::DQSTRING;
    char *End = Current + 1;
    while ((*End != '\"') && (End < State->EndPtr)) {
      switch (*End) {
      case '\\':
        if (End + 1 < State->EndPtr) {
          ReturnToken.Id += GetEsacpedChar(*(End + 1));
          End += 2;
        }
        break;

      default:
        ReturnToken.Id += *End;
        ++End;
        break;
      }
    }

    State->OffsetCurrent += End - Current;
    Current = End + 1;
    ReturnToken.Line = State->LineCurrent;
    ReturnToken.Offset = State->OffsetCurrent;
    ++State->OffsetCurrent;
    goto _Exit;
  }

  if (Current[0] == '\'') {
    ReturnToken.Type = token::DQSTRING;

    char *End = Current + 1;
    while ((*End != '\'') && (End < State->EndPtr)) {
      switch (*End) {
      case '\\':
        if (End + 1 < State->EndPtr) {
          ReturnToken.Id += GetEsacpedChar(*(End + 1));
          End += 2;
        }
        break;

      default:
        ReturnToken.Id += *End;
        ++End;
        break;
      }
    }

    State->OffsetCurrent += End - Current;
    Current = End + 1;
    ReturnToken.Line = State->LineCurrent;
    ReturnToken.Offset = State->OffsetCurrent;
    ++State->OffsetCurrent;
    goto _Exit;
  }

  switch (Current[0]) {
  case '<': {
    if (Current < State->EndPtr) {
      if (Current[1] == '<') {
        ReturnToken.Type = token::SHIFTLEFT;
        ++State->OffsetCurrent;
      } else if (Current[1] == '=') {
        ReturnToken.Type = token::LESSEQ;
        ++State->OffsetCurrent;
      }
    }
    goto _BuildToken;
  }

  default:
  _BuildToken:
    ReturnToken.Line = State->LineCurrent;
    ReturnToken.Offset = State->OffsetCurrent;
    ++State->OffsetCurrent;
    ReturnToken.Type = Current[0];
  }

  ++Current;

_Exit:
  State->CurrentPtr = Current;
  return ReturnToken;
}
