
#include "codegen_neo.h"
#include "lexer.h"

#include <sstream>
namespace std {
template <typename T> string to_string(T Value) {
  stringstream ss;
  ss << Value;
  return ss.str();
}
};

const neocode_variable ReturnReg = {"", "", 0, 15 + 0x10, 0, {0}};

static int GetInstructionFromIdentifier(std::string Name) {
  if (Name.compare("mov") == 0) {
    return neocode_instruction::MOV;
  }
  if (Name.compare("mul") == 0) {
    return neocode_instruction::MUL;
  }
  if (Name.compare("rsq") == 0) {
    return neocode_instruction::RSQ;
  }
  if (Name.compare("rcp") == 0) {
    return neocode_instruction::RCP;
  }
  if (Name.compare("nop") == 0) {
    return neocode_instruction::NOP;
  }
  if (Name.compare("end") == 0) {
    return neocode_instruction::END;
  }
  return neocode_instruction::EMPTY;
}

static std::string RegisterName(neocode_variable &Var, int UseRaw = 0) {
  if (Var.Name.size() && !UseRaw)
    return Var.Name;
  if (Var.RegisterType == 0) {
    int Register = Var.Register;
    if (Register < 0x10)
      return std::string("v") + std::to_string(Register);
    if (Register < 0x20)
      return std::string("r") + std::to_string(Register - 0x10);

    return std::string("c") + std::to_string(Register - 0x20);
  } else {
    int Register = Var.Register;
    if (Register < 0x10)
      return std::string("o") + std::to_string(Register);
    if (Register < 0x20)
      return std::string("r") + std::to_string(Register - 0x10);

    return std::string("c") + std::to_string(Register - 0x20);
  }
}

neocode_variable *neocode_function::GetVariable(std::string Name) {

  for (neocode_variable &V : Variables) {
    const std::string FuncName = this->Name + "_" + Name;
    if (V.Name.compare(FuncName) == 0) {
      return &V;
    }
  }

  for (neocode_variable &V : Program->Globals) {
    if (V.Name.compare(Name) == 0) {
      return &V;
    }
  }
}

neocode_instruction CGNeoBuildInstruction(neocode_function *Function,
                                          ast_node *ASTNode) {

  if ((ASTNode->Type == ast_node::VARIABLE) &&
      (ASTNode->Modifiers & ast_node::DECLARE)) {
    neocode_variable Var;
    Var.Name = Function->Name + "_" + ASTNode->Id;
    Var.Type = ASTNode->VarType;
    Var.Register = Function->Program->Registers.AllocTemp();
    Var.RegisterType = 0;
    Function->Variables.push_back(Var);
    neocode_instruction In;
    In.Type = neocode_instruction::EMPTY;
    In.Dst = Var;
    Function->Instructions.push_back(In);
    return In;
  }

  if (ASTNode->VarType == ast_node::FLOAT_LITERAL) {
    neocode_variable Constant;
    Constant.Type = ast_node::FLOAT_LITERAL;
    Constant.RegisterType = 0;
    Constant.Register = Function->Program->Registers.AllocConstant();
    Constant.Name =
        std::string("Anonymous_float") + "_" + RegisterName(Constant, 1);
    Constant.Const.Float.X = ASTNode->FloatValue;
    Constant.Const.Float.Y = ASTNode->FloatValue;
    Constant.Const.Float.Z = ASTNode->FloatValue;
    Constant.Const.Float.W = ASTNode->FloatValue;
    Function->Program->Globals.push_back(Constant);
    neocode_instruction In;
    In.Type = neocode_instruction::EMPTY;
    In.Dst = Constant;
    Function->Instructions.push_back(In);
    return In;
  }

  if (ASTNode->Type == ast_node::VARIABLE) {
    neocode_instruction In;
    In.Type = neocode_instruction::EMPTY;
    In.Dst = *Function->GetVariable(ASTNode->Id);
    Function->Instructions.push_back(In);
    return In;
  }

  if (ASTNode->Type == ast_node::FUNCTION_CALL) {
    if (ASTNode->Id.compare("asm") == 0) {
      lexer_state LexerState;

      auto GetNextFromTokenSpecifier = [&LexerState, &Function, &ASTNode]() {
        token Token = LexerGetToken(&LexerState);
        if (Token.Type == ',')
          Token = LexerGetToken(&LexerState);

        if (Token.Type == '@') {
          Token = LexerGetToken(&LexerState);
          return CGNeoBuildInstruction(Function,
                                       &ASTNode->Children[Token.IntValue])
              .Dst;
        }
        if (Token.Type == token::END) {
          return neocode_variable();
        }

        return *Function->GetVariable(Token.Id);
      };

      char *Source = (char *)ASTNode->Children[0].Id.c_str();
      LexerInit(&LexerState, Source, Source + strlen(Source) + 1);
      token Token = LexerGetToken(&LexerState);
      neocode_instruction In;
      In.Type = GetInstructionFromIdentifier(Token.Id);
      In.Dst = GetNextFromTokenSpecifier();
      In.Src1 = GetNextFromTokenSpecifier();
      In.Src2 = GetNextFromTokenSpecifier();
      Function->Instructions.push_back(In);
      return In;
    } else {
      //      ast_node *Type = ASTNode->LookupType(ASTNode->Id);
      if (true) {
        // generate constant
        neocode_variable Constant;
        Constant.Type = ast_node::STRUCT;
        Constant.RegisterType = 0;
        Constant.Register = Function->Program->Registers.AllocConstant();
        Constant.Name = std::string("Anonymous_") + ASTNode->Id + "_" +
                        RegisterName(Constant, 1);
        Constant.Const.Float.X = ASTNode->Children[0].FloatValue;
        Constant.Const.Float.Y = ASTNode->Children[1].FloatValue;
        Constant.Const.Float.Z = ASTNode->Children[2].FloatValue;
        Constant.Const.Float.W = ASTNode->Children[3].FloatValue;
        Function->Program->Globals.push_back(Constant);
        neocode_instruction In;
        In.Type = neocode_instruction::EMPTY;
        In.Dst = Constant;
        Function->Instructions.push_back(In);
        return In;
      }
    }
  }

  if (ASTNode->Type == ast_node::MULTIPLY) {
    neocode_instruction In;
    In.Type = neocode_instruction::MUL;
    In.Dst = (neocode_variable){"", "", ast_node::STRUCT,
                                Function->Program->Registers.AllocTemp(), 0};
    In.Src1 = CGNeoBuildInstruction(Function, &ASTNode->Children[0]).Dst;
    In.Src2 = CGNeoBuildInstruction(Function, &ASTNode->Children[1]).Dst;
    Function->Instructions.push_back(In);
    Function->Program->Registers.Free(In.Dst.Register);
    return In;
  }

  if (ASTNode->Type == ast_node::DIVIDE) {
    neocode_instruction In;
    In.Type = neocode_instruction::RCP;
    In.Dst = (neocode_variable){"", "", ast_node::STRUCT,
                                Function->Program->Registers.AllocTemp(), 0};
    In.Src1 = CGNeoBuildInstruction(Function, &ASTNode->Children[1]).Dst;
    Function->Instructions.push_back(In);
    Function->Program->Registers.Free(In.Dst.Register);
    if (ASTNode->Children[0].VarType == ast_node::FLOAT_LITERAL &&
        ASTNode->Children[0].FloatValue != 1.0) {
      In.Type = neocode_instruction::MUL;
      In.Src1 = In.Dst;
      In.Src2 = CGNeoBuildInstruction(Function, &ASTNode->Children[0]).Dst;
      Function->Instructions.push_back(In);
    }
    return In;
  }

  if (ASTNode->Type == ast_node::ASSIGNMENT) {
    neocode_instruction In;
    In.Type = neocode_instruction::MOV;
    In.Dst = CGNeoBuildInstruction(Function, &ASTNode->Children[0]).Dst;
    In.Src1 = CGNeoBuildInstruction(Function, &ASTNode->Children[1]).Dst;
    Function->Instructions.push_back(In);
    return In;
  }

  if (ASTNode->Type == ast_node::RETURN) {
    neocode_instruction In;
    In.Type = neocode_instruction::MOV;
    In.Dst = ReturnReg;
    In.Src1 =
        CGNeoBuildInstruction(Function, &ASTNode->Children[0].Children[0]).Dst;
    Function->Instructions.push_back(In);
    return In;
  }

  return neocode_instruction();
}

void CGNeoBuildStatement(neocode_function *Function, ast_node *ASTNode) {
  CGNeoBuildInstruction(Function, ASTNode);
}

neocode_function CGNeoBuildFunction(neocode_program *Program,
                                    ast_node *ASTNode) {
  neocode_function Function = neocode_function(Program);
  Function.Name = ASTNode->Id;
  for (int i = 0; i < ASTNode->Children.size(); ++i) {
    if (ASTNode->Children[i].Children.size()) {
      CGNeoBuildStatement(&Function, &ASTNode->Children[i].Children[0]);
    }
  }
  Program->Registers.FreeAllTemp();
  return Function;
}

neocode_program CGNeoBuildProgramInstance(ast_node *ASTNode) {
  neocode_program Program;
  Program.Globals.push_back(
      (neocode_variable){"gl_Position",
                         "vec4",
                         ast_node::STRUCT,
                         Program.Registers.AllocOutput(),
                         neocode_variable::OUTPUT_POSITION,
                         {0}});
  Program.Globals.push_back((neocode_variable){"gl_FrontColor",
                                               "vec4",
                                               ast_node::STRUCT,
                                               Program.Registers.AllocOutput(),
                                               neocode_variable::OUTPUT_COLOR,
                                               {0}});
  Program.Globals.push_back((neocode_variable){"gl_Vertex",
                                               "vec4",
                                               ast_node::STRUCT,
                                               Program.Registers.AllocVertex(),
                                               0,
                                               {0}});
  Program.Globals.push_back((neocode_variable){"gl_Color",
                                               "vec4",
                                               ast_node::STRUCT,
                                               Program.Registers.AllocVertex(),
                                               0,
                                               {0}});
  Program.Globals.push_back(
      (neocode_variable){"gl_ModelViewProjectionMatrix",
                         "mat4x4",
                         ast_node::STRUCT,
                         Program.Registers.AllocConstant(),
                         neocode_variable::INPUT_UNIFORM,
                         {0}});
  for (ast_node &Node : ASTNode->Children) {
    if (Node.Type == ast_node::FUNCTION)
      Program.Functions.push_back(CGNeoBuildFunction(&Program, &Node));
  }
  return Program;
}

void CGNeoGenerateInstruction(neocode_instruction *Instruction,
                              std::ostream &os) {
  switch (Instruction->Type) {
  case neocode_instruction::EMPTY:
    break;
  case neocode_instruction::INLINE:
    os << " " << Instruction->ExtraData << std::endl;
    break;

  case neocode_instruction::MOV:
    os << " "
       << "mov " << RegisterName(Instruction->Dst) << ", "
       << RegisterName(Instruction->Src1) << std::endl;
    break;

  case neocode_instruction::MUL:
    os << " "
       << "mul " << RegisterName(Instruction->Dst) << ", "
       << RegisterName(Instruction->Src1) << ", "
       << RegisterName(Instruction->Src2) << std::endl;
    break;

  case neocode_instruction::RSQ:
    os << " "
       << "rsq " << RegisterName(Instruction->Dst) << ", "
       << RegisterName(Instruction->Src1) << std::endl;
    break;

  case neocode_instruction::RCP:
    os << " "
       << "rcp " << RegisterName(Instruction->Dst) << ", "
       << RegisterName(Instruction->Src1) << std::endl;
    break;

  case neocode_instruction::NOP:
    os << " "
       << "nop" << std::endl;
    break;

  case neocode_instruction::END:
    os << " "
       << "end" << std::endl;
    break;

  default:
    os << "// Unknown instruction: " << Instruction->Type << std::endl;
    break;
  }
}

void CGNeoGenerateFunction(neocode_function *Function, std::ostream &os) {
  os << Function->Name << ":" << std::endl;
  for (neocode_instruction &Instruction : Function->Instructions) {
    CGNeoGenerateInstruction(&Instruction, os);
  }
  os << Function->Name << "_end:" << std::endl;
}

static std::string OutputName(int Register) {
  const std::string ONames[7] = {
      "position",  "quaternion", "color", "texcoord0",
      "texcoord1", "texcoord2",  "view",
  };

  return ONames[Register - 1];
}

void CGNeoGenerateCode(neocode_program *Program, std::ostream &os) {
  os << ".alias CaelinaCCVersion c95 as (0.0, 0.0, 0.0, 0.1)" << std::endl;
  for (neocode_variable &V : Program->Globals) {
    if (V.RegisterType > 0) {
      os << ".alias " << V.Name << " " << RegisterName(V, 1) << " as "
         << OutputName(V.RegisterType) << std::endl;
    } else if (V.Register < 0x20) {
      os << ".alias " << V.Name << " " << RegisterName(V, 1) << std::endl;
    } else if (V.RegisterType == 0) {
      neocode_constant Const = V.Const;
      os << ".alias " << V.Name << " " << RegisterName(V, 1) << " as ("
         << Const.Float.X << "," << Const.Float.Y << "," << Const.Float.Z << ","
         << Const.Float.W << ")" << std::endl;
    } else {
      os << ".alias " << V.Name << " " << RegisterName(V, 1) << std::endl;
    }
  }
  for (neocode_function &Function : Program->Functions) {
    for (neocode_variable &V : Function.Variables) {
      if (V.RegisterType > 0) {
        os << ".alias " << V.Name << " " << RegisterName(V, 1) << " as "
           << OutputName(V.RegisterType) << std::endl;
      } else if (V.Register < 0x20) {
        os << ".alias " << V.Name << " " << RegisterName(V, 1) << std::endl;
      } else if (V.RegisterType == 0) {
        neocode_constant Const = V.Const;
        os << ".alias " << V.Name << " " << RegisterName(V, 1) << " as ("
           << Const.Float.X << "," << Const.Float.Y << "," << Const.Float.Z
           << "," << Const.Float.W << ")" << std::endl;
      } else {
        os << ".alias " << V.Name << " " << RegisterName(V, 1) << std::endl;
      }
    }
    CGNeoGenerateFunction(&Function, os);
  }
}
