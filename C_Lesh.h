// ============================================================================
// C-Lesh (Definitions)
// Programmed by Francois Lamini
// ============================================================================

#include "..\Code_Helper\Codeloader.hpp"
#include "..\Code_Helper\Allegro.hpp"

namespace Codeloader {

  enum eCode {
    eCODE_STORE,
    eCODE_DUMP,
    eCODE_TEST,
    eCODE_JUMP,
    eCODE_CALL,
    eCODE_RETURN,
    eCODE_PUSH,
    eCODE_POP,
    eCODE_LOAD,
    eCODE_SAVE,
    eCODE_INPUT,
    eCODE_REFRESH,
    eCODE_SOUND,
    eCODE_TIMEOUT,
    eCODE_OUTPUT,
    eCODE_STRING,
    eCODE_PALETTE,
    eCODE_DRAW,
    eCODE_CLEAR,
    eCODE_RESIZE,
    eCODE_COLUMN,
    eCODE_STOP
  };

  enum eOperator {
    eOPERATOR_NONE,
    eOPERATOR_ADD,
    eOPERATOR_SUBTRACT,
    eOPERATOR_MULTIPLY,
    eOPERATOR_DIVIDE,
    eOPERATOR_REMAINDER,
    eOPERATOR_RANDOM,
    eOPERATOR_COSINE,
    eOPERATOR_SINE
  };

  enum eAddress {
    eADDRESS_VALUE,
    eADDRESS_IMMEDIATE,
    eADDRESS_POINTER,
    eADDRESS_STACK,
    eADDRESS_OBJECT_IMMEDIATE,
    eADDRESS_OBJECT_POINTER
  };

  enum eLogic_Operator {
    eLOGIC_NONE,
    eLOGIC_AND,
    eLOGIC_OR
  };

  enum eTest {
    eTEST_EQUALS,
    eTEST_NOT,
    eTEST_LESS,
    eTEST_GREATER,
    eTEST_LESS_OR_EQUAL,
    eTEST_GREATER_OR_EQUAL
  };

  enum eFile_Mode {
    eFILE_LIST,
    eFILE_TABLE
  };

  class cMemory {

    public:
      cTable** tables;
      int count;

      cMemory(int count, int width, int height);
      ~cMemory();
      void Clear();
      cTable& operator[](int address);

  };
  
  class cC_Lesh {

    public:
      cMemory* memory;
      cPicture_Processor* pp;
      cIO_Control* io;
      int command_pointer;
      int stack_pointer;
      int status;

      cC_Lesh(cPicture_Processor* pp, cIO_Control* io, std::string config);
      ~cC_Lesh();
      void Load_Program(std::string name);
      void Execute(int timeout);
      void Interpret();
      int Eval_Expression(cTable& command);
      int Eval_Operand(cTable& command);
      int Eval_Conditional(cTable& command);
      int Eval_Condition(cTable& command);
      cTable& Get_Table_At_Immediate_Address(int address);
      cTable& Get_Table_At_Pointer(int address);
      cTable& Get_Table_At_Stack(int address);
      cTable& Get_Object_At_Immediate_Address(int address, int prop_index);
      cTable& Get_Object_At_Pointer(int address, int prop_index);
      cTable& Get_Table_At_Address(cTable& command);
      void Store(cTable& command);
      void Dump(cTable& command);
      void Test(cTable& command);
      void Jump(cTable& command);
      void Call(cTable& command);
      void Push(cTable& command);
      void Pop(cTable& command);
      void Return(cTable& command);
      void Load(cTable& command);
      void Save(cTable& command);
      void Input(cTable& command);
      void Refresh(cTable& command);
      void Sound(cTable& command);
      void Timeout(cTable& command);
      void Output(cTable& command);
      void String(cTable& command);
      void Palette(cTable& command);
      void Draw(cTable& command);
      void Clear(cTable& command);
      void Resize(cTable& command);
      void Column(cTable& command);
      void Stop(cTable& command);
      void Stack_Push(int value);
      int Stack_Pop();
      void Load_File_List(std::string name, cMemory& memory, int address);
      void Load_Table_List(std::string name, cMemory& memory, int address);
      std::string C_Lesh_String_To_Cpp_String(cTable& table);

  };

}