// ============================================================================
// C-Lesh (Implementation)
// Programmed by Francois Lamini
// ============================================================================

#include "C_Lesh.h"

Codeloader::cC_Lesh* clsh = NULL;

bool Source_Process();
bool Process_Keys();

// **************************************************************************
// Program Entry Point
// **************************************************************************

int main(int argc, char** argv) {
  if (argc == 4) {
    std::string program = argv[1];
    int width = Codeloader::Text_To_Number(argv[2]);
    int height = Codeloader::Text_To_Number(argv[3]);
    try {
      Codeloader::cPicture_Processor pp(width, height);
      Codeloader::cAllegro_IO allegro(program, width, height, 2, "Game");
      clsh = new Codeloader::cC_Lesh(&pp, &allegro, "Config");
      allegro.Load_Resources("Resources");
      allegro.Load_Button_Names("Button_Names");
      allegro.Load_Button_Map("Buttons");
      clsh->Load_Program(program);
      allegro.Process_Messages(Source_Process, Process_Keys);
    }
    catch (Codeloader::cError error) {
      error.Print();
    }
    if (clsh) {
      delete clsh;
    }
  }
  else {
    std::cout << "Usage: " << argv[0] << " <program> <width> <height>" << std::endl;
  }
  std::cout << "Done." << std::endl;
  return 0;
}

// ****************************************************************************
// C-Lesh Processor
// ****************************************************************************

/**
 * Called when command needs to be processed.
 * @return True if the app needs to exit, false otherwise.
 */
bool Source_Process() {
  clsh->Execute(20);
  return false;
}

/**
 * Called when keys are processed.
 * @return True if the app needs to exit, false otherwise.
 */
bool Process_Keys() {
  return false;
}

namespace Codeloader {

  // **************************************************************************
  // C-Lesh Implementation
  // **************************************************************************

  /**
   * Creates the C-Lesh module.
   * @param pp The picture processor.
   * @param io The I/O control.
   * @param cp The co-processor.
   * @param config The name of the configuration file.
   * @throws An error if the configuration file could not be loaded.
   */
  cC_Lesh::cC_Lesh(cPicture_Processor* pp, cIO_Control* io, std::string config) {
    this->pp = pp;
    this->io = io;
    this->command_pointer = 0;
    this->stack_pointer = 400;
    this->memory = NULL;
    this->status = eSTATUS_IDLE;
    // Read the configuration file.
    std::ifstream config_file(config + ".txt");
    if (config_file) {
      int table_w = 1;
      int table_h = 1;
      int memory_size = 500;
      while (!config_file.eof()) {
        std::string line;
        std::getline(config_file, line);
        if (config_file.good()) {
          cArray<std::string> pair = Parse_Sausage_Text(line, "=");
          if (pair.Count() == 2) {
            if (pair[0] == "table") {
              cArray<std::string> dimensions = Parse_Sausage_Text(pair[1], "x");
              table_w = Text_To_Number(dimensions[0]);
              table_h = Text_To_Number(dimensions[1]);
            }
            else if (pair[0] == "memory") {
              memory_size = Text_To_Number(pair[1]);
            }
            else if (pair[0] == "program") {
              this->command_pointer = Text_To_Number(pair[1]);
            }
            else if (pair[0] == "stack") {
              this->stack_pointer = Text_To_Number(pair[1]);
            }
            else {
              throw cError("Invalid configuration property.");
            }
          }
          // Anything that is not a pair is a comment.
        }
      }
      // Apply settings.
      this->memory = new cMemory(memory_size, table_w, table_h);
    }
    else {
      throw cError("Could not load config file.");
    }
  }

  /**
   * Frees up C-Lesh.
   */
  cC_Lesh::~cC_Lesh() {
    if (this->memory) {
      delete this->memory;
    }
  }

  /**
   * Loads a program into C-Lesh.
   * @param name The name of the program to load.
   * @throws An error if the program could not be loaded.
   */
  void cC_Lesh::Load_Program(std::string name) {
    this->Load_Table_List(name + ".clshc", *(this->memory), this->command_pointer);
    this->status = eSTATUS_RUNNING;
  }

  /**
   * Executes a code.
   * @param timeout The time slice to execute.
   */
  void cC_Lesh::Execute(int timeout) {
    std::clock_t start = std::clock();
    while (this->status == eSTATUS_RUNNING) {
      std::clock_t end = std::clock();
      std::clock_t diff = (end - start) / CLOCKS_PER_SEC * 1000;
      if ((int)diff >= timeout) { // Time up!
        break;
      }
      else {
        try {
          this->Interpret();
        }
        catch (cError error) {
          this->status = eSTATUS_ERROR;
          throw error; // Throw the error again!
        }
      }
    }
  }

  /**
   * Interprets a single command.
   * @throws An error if the command is invalid.
   */
  void cC_Lesh::Interpret() {
    cTable& command = (*this->memory)[this->command_pointer++]; // Had to dereference!
    command.Rewind(); // Rewind so the command can be executed again.
    int code = command.Read_Column();
    command.Move_To_Next_Row();
    switch (code) { // Command is at origin.
      case eCODE_STORE: {
        this->Store(command);
        break;
      }
      case eCODE_DUMP: {
        this->Dump(command);
        break;
      }
      case eCODE_TEST: {
        this->Test(command);
        break;
      }
      case eCODE_JUMP: {
        this->Jump(command);
        break;
      }
      case eCODE_CALL: {
        this->Call(command);
        break;
      }
      case eCODE_RETURN: {
        this->Return(command);
        break;
      }
      case eCODE_PUSH: {
        this->Push(command);
        break;
      }
      case eCODE_POP: {
        this->Pop(command);
        break;
      }
      case eCODE_LOAD: {
        this->Load(command);
        break;
      }
      case eCODE_SAVE: {
        this->Save(command);
        break;
      }
      case eCODE_INPUT: {
        this->Input(command);
        break;
      }
      case eCODE_REFRESH: {
        this->Refresh(command);
        break;
      }
      case eCODE_SOUND: {
        this->Sound(command);
        break;
      }
      case eCODE_TIMEOUT: {
        this->Timeout(command);
        break;
      }
      case eCODE_OUTPUT: {
        this->Output(command);
        break;
      }
      case eCODE_STRING: {
        this->String(command);
        break;
      }
      case eCODE_PALETTE: {
        this->Palette(command);
        break;
      }
      case eCODE_DRAW: {
        this->Draw(command);
        break;
      }
      case eCODE_CLEAR: {
        this->Clear(command);
        break;
      }
      case eCODE_RESIZE: {
        this->Resize(command);
        break;
      }
      case eCODE_COLUMN: {
        this->Column(command);
        break;
      }
      case eCODE_STOP: {
        this->Stop(command);
        break;
      }
      default: {
        throw cError("Invalid command " + Number_To_Text(code) + ".");
      }
    }
  }

  /**
   * Evaluates an expression and returns a number.
   * @param command The command where the expression is.
   * @return The result of the evaluation.
   * @throws An error if the operator is invalid.
   */
  int cC_Lesh::Eval_Expression(cTable& command) {
    int result = this->Eval_Operand(command);
    int op = command.Read_Column();
    while (op != eOPERATOR_NONE) {
      int operand_result = this->Eval_Operand(command);
      switch (op) {
        case eOPERATOR_ADD: {
          result += operand_result;
          break;
        }
        case eOPERATOR_SUBTRACT: {
          result -= operand_result;
          break;
        }
        case eOPERATOR_MULTIPLY: {
          result *= operand_result;
          break;
        }
        case eOPERATOR_DIVIDE: {
          if (operand_result != 0) {
            result /= operand_result;
          }
          break;
        }
        case eOPERATOR_REMAINDER: {
          if (operand_result == 0) {
            result = 0;
          }
          else {
            result %= operand_result;
          }
          break;
        }
        case eOPERATOR_RANDOM: {
          result = this->io->Get_Random_Number(result, operand_result);
          break;
        }
        case eOPERATOR_COSINE: {
          result = (int)((double)result * std::cos((double)operand_result * 3.15 / 180.0));
          break;
        }
        case eOPERATOR_SINE: {
          result = (int)((double)result * std::sin((double)operand_result * 3.15 / 180.0));
          break;
        }
        case eOPERATOR_NONE: {
          break; // Do nothing.
        }
        default: {
          throw cError("Invalid operator " + Number_To_Text(op) + ".");
        }
      }
      op = command.Read_Column();
    }
    command.Move_To_Next_Row();
    return result;
  }

  /**
   * Evaluates an operand.
   * @param command The command where the operand is in.
   * @return The result of the operand.
   * @throws An error if the addressing mode is invalid.
   */
  int cC_Lesh::Eval_Operand(cTable& command) {
    int mode = command.Read_Column();
    int result = 0;
    switch (mode) {
      case eADDRESS_VALUE: {
        result = command.Read_Column();
        break;
      }
      case eADDRESS_IMMEDIATE: {
        int address = command.Read_Column();
        cTable& value = this->Get_Table_At_Immediate_Address(address);
        result = value.Read_Column();
        break;
      }
      case eADDRESS_POINTER: {
        int address = command.Read_Column();
        cTable& value = this->Get_Table_At_Pointer(address);
        result = value.Read_Column();
        break;
      }
      case eADDRESS_STACK: {
        int address = command.Read_Column();
        cTable& value = this->Get_Table_At_Stack(address);
        result = value.Read_Column();
        break;
      }
      case eADDRESS_OBJECT_IMMEDIATE: {
        int address = command.Read_Column();
        int prop_index = command.Read_Column();
        cTable& value = this->Get_Object_At_Immediate_Address(address, prop_index);
        result = value.Read_Column();
        break;
      }
      case eADDRESS_OBJECT_POINTER: {
        int address = command.Read_Column();
        int prop_index = command.Read_Column();
        cTable& value = this->Get_Object_At_Pointer(address, prop_index);
        result = value.Read_Column();
        break;
      }
      default: {
        throw cError("Invalid address mode " + Number_To_Text(mode) + ".");
      }
    }
    return result;
  }

  /**
   * Evaluates a conditional expression.
   * @param command The command containing the conditional expression.
   * @return The result of the conditional.
   * @throws An error if the logic operator is invalid.
   */
  int cC_Lesh::Eval_Conditional(cTable& command) {
    int result = this->Eval_Condition(command);
    int logic_operator = command.Read_Column(); // One operator per row.
    command.Move_To_Next_Row();
    while (logic_operator != eLOGIC_NONE) {
      int cond_result = this->Eval_Condition(command);
      switch (logic_operator) {
        case eLOGIC_AND: {
          result *= cond_result; // Multiply for AND.
          break;
        }
        case eLOGIC_OR: {
          result += cond_result; // Add for OR.
          break;
        }
        case eLOGIC_NONE: {
          break; // Do nothing.
        }
      }
      logic_operator = command.Read_Column();
      command.Move_To_Next_Row();
    }
    return result;
  }

  /**
   * Evaluates a condition.
   * @param command The command containing the condition.
   * @return The result of the condition test.
   * @throws An error if the test is invalid.
   */
  int cC_Lesh::Eval_Condition(cTable& command) {
    int result = 0;
    int left_operand = this->Eval_Expression(command);
    int test = command.Read_Column(); // Test is on single row.
    command.Move_To_Next_Row();
    int right_operand = this->Eval_Expression(command);
    int diff = right_operand - left_operand;
    switch (test) {
      case eTEST_EQUALS: {
        result = (diff == 0);
        break;
      }
      case eTEST_NOT: {
        result = (diff != 0);
        break;
      }
      case eTEST_LESS: {
        result = (diff < 0);
        break;
      }
      case eTEST_GREATER: {
        result = (diff > 0);
        break;
      }
      case eTEST_LESS_OR_EQUAL: {
        result = (diff <= 0);
        break;
      }
      case eTEST_GREATER_OR_EQUAL: {
        result = (diff >= 0);
        break;
      }
      default: {
        throw cError("Invalid test operator " + Number_To_Text(test) + ".");
      }
    }
    return result;
  }

  /**
   * Gets a table at an immediate address.
   * @param address The address where the table is.
   * @return The table reference.
   */
  cTable& cC_Lesh::Get_Table_At_Immediate_Address(int address) {
    cTable& table = (*this->memory)[address];
    table.Rewind(); // Rewind to get origin column.
    return table;
  }

  /**
   * Gets a table at a pointer.
   * @param address The address of the pointer.
   * @return The table reference.
   */
  cTable& cC_Lesh::Get_Table_At_Pointer(int address) {
    cTable& pointer = (*this->memory)[address];
    pointer.Rewind();
    int ptr_value = pointer.Read_Column();
    cTable& table = (*this->memory)[ptr_value];
    table.Rewind();
    return table;
  }

  /**
   * Gets a table at a stack location.
   * @param The address of the table on the stack. These addresses are reversed an 1-based.
   * @return The table reference.
   */
  cTable& cC_Lesh::Get_Table_At_Stack(int address) {
    int stack_address = this->stack_pointer - address; // Access variables in reverse.
    cTable& table = (*this->memory)[stack_address];
    table.Rewind();
    return table;
  }

  /**
   * Gets an object at an immediate address. The object property value can then be read or written.
   * @param address The address of the object.
   * @param prop_index The index of the property to read or written.
   * @return A reference to the table containing the object.
   */
  cTable& cC_Lesh::Get_Object_At_Immediate_Address(int address, int prop_index) {
    cTable& object = (*this->memory)[address];
    object.Rewind(); // Rewind to get origin column.
    object.Move_To_Row(prop_index); // Move to row where property is.
    return object;
  }

  /**
   * Gets an object at a pointer.
   * @param address The address of the pointer.
   * @param prop_index The index of the property to be read or written.
   * @return A reference to the table containing the object.
   */
  cTable& cC_Lesh::Get_Object_At_Pointer(int address, int prop_index) {
    cTable& pointer = (*this->memory)[address];
    pointer.Rewind();
    int ptr_value = pointer.Read_Column();
    cTable& object = (*this->memory)[ptr_value];
    object.Rewind();
    object.Move_To_Row(prop_index);
    return object;
  }

  /**
   * Gets the table at an address given the mode.
   * @param command The command to get the address meta data from.
   * @throws An error if the mode is invalid.
   */
  cTable& cC_Lesh::Get_Table_At_Address(cTable& command) {
    // Read meta data row.
    int mode = command.Read_Column();
    int address = command.Read_Column();
    command.Move_To_Next_Row();
    switch (mode) {
      case eADDRESS_VALUE: {
        break; // Return the command itself. The row has the value.
      }
      case eADDRESS_IMMEDIATE: {
        return this->Get_Table_At_Immediate_Address(address);
      }
      case eADDRESS_POINTER: {
        return this->Get_Table_At_Pointer(address);
      }
      case eADDRESS_STACK: {
        return this->Get_Table_At_Stack(address);
      }
      case eADDRESS_OBJECT_IMMEDIATE: {
        int prop_index = command.Read_Column();
        command.Move_To_Next_Row();
        return this->Get_Object_At_Immediate_Address(address, prop_index);
      }
      case eADDRESS_OBJECT_POINTER: {
        int prop_index = command.Read_Column();
        command.Move_To_Next_Row();
        return this->Get_Object_At_Pointer(address, prop_index);
      }
      default: {
        throw cError("Invalid store mode " + Number_To_Text(mode) + ".");
      }
    }
    return command; // Just return the command.
  }

  /**
   * Executes the store command.
   * @param command The command reference.
   * @throws An error if the store mode is invalid.
   */
  void cC_Lesh::Store(cTable& command) {
    cTable& table = this->Get_Table_At_Address(command);
    int result = this->Eval_Expression(command);
    table.Rewind(); // Store result at beginning of table.
    table.Write_Column(result);
  }

  /**
   * Dump the memory, stack, screen, pointers, and status.
   * @param command The command reference.
   */
  void cC_Lesh::Dump(cTable& command) {
    // Dump the pointers.
    std::cout << "command=" << this->command_pointer << std::endl;
    std::cout << "stack=" << this->stack_pointer << std::endl;
    // Dump the status.
    std::cout << "status=" << this->status << std::endl;
    // Dump the memory.
    for (int table_index = 0; table_index < this->memory->count; table_index++) {
      (*this->memory)[table_index].Dump();
    }
    // Dump the screen.
    this->pp->Dump();
  }

  /**
   * Executes a test command.
   * @param command The command reference.
   * @throws An error if the test command is missing something.
   */
  void cC_Lesh::Test(cTable& command) {
    int result = this->Eval_Conditional(command);
    int passed_address = this->Eval_Expression(command);
    int failed_address = this->Eval_Expression(command);
    if (passed_address != TAKE_NO_JUMP) {
      if (result) {
        this->command_pointer = passed_address;
      }
    }
    if (failed_address != TAKE_NO_JUMP) {
      if (!result) {
        this->command_pointer = failed_address;
      }
    }
  }

  /**
   * Executes a jump command.
   * @param command The command reference.
   */
  void cC_Lesh::Jump(cTable& command) {
    this->command_pointer = this->Eval_Expression(command);
  }

  /**
   * Executes a call command.
   * @param command The command reference.
   */
  void cC_Lesh::Call(cTable& command) {
    // Push the command pointer to the stack.
    this->Stack_Push(this->command_pointer);
    // Jump to location.
    this->command_pointer = this->Eval_Expression(command);
  }

  /**
   * Executes a push command.
   * @param command The command reference.
   * @throws An error if there is no more stack space.
   */
  void cC_Lesh::Push(cTable& command) {
    int value = this->Eval_Expression(command);
    this->Stack_Push(value);
  }

  /**
   * Executes a pop command.
   * @param command The command reference.
   * @throws An error if the stack pointer is out of bounds.
   */
  void cC_Lesh::Pop(cTable& command) {
    int value = this->Stack_Pop();
    cTable& location = this->Get_Table_At_Address(command);
    location.Write_Column(value);
  }

  /**
   * Executes a return command.
   * @param command The command reference.
   */
  void cC_Lesh::Return(cTable& command) {
    (*this->memory)[this->stack_pointer - 1].Rewind();
    this->command_pointer = (*this->memory)[this->stack_pointer - 1].Read_Column();
    this->stack_pointer--;
  }

  /**
   * Loads a file from disk.
   * @param command The command reference.
   * @throws An error if the file mode is incorrect.
   */
  void cC_Lesh::Load(cTable& command) {
    cTable& text = this->Get_Table_At_Address(command);
    std::string name = C_Lesh_String_To_Cpp_String(text);
    int mode = this->Eval_Expression(command);
    int address = this->Eval_Expression(command);
    switch (mode) {
      case eFILE_LIST: {
        this->Load_File_List(name, *this->memory, address);
        break;
      }
      case eFILE_TABLE: {
        this->Load_Table_List(name, *this->memory, address);
        break;
      }
      default: {
        throw cError("Invalid file mode " + Number_To_Text(mode) + ".");
      }
    }
  }

  /**
   * Saves a file to disk.
   * @param command The command reference.
   * @throws An error if the file could not be saved.
   */
  void cC_Lesh::Save(cTable& command) {
    cTable& object = this->Get_Table_At_Address(command);
    cTable& text = this->Get_Table_At_Address(command);
    std::string name = C_Lesh_String_To_Cpp_String(text);
    object.Save_To_File(name);
  }

  /**
   * Executes an input command.
   * @param the command reference.
   */
  void cC_Lesh::Input(cTable& command) {
    cTable& value = this->Get_Table_At_Address(command);
    sSignal signal = this->io->Read_Signal();
    value.Write_Column(signal.code);
  }

  /**
   * Executes a refresh command.
   * @param command The command reference.
   */
  void cC_Lesh::Refresh(cTable& command) {
    this->io->Update_Display(this->pp);
  }

  /**
   * Executes a sound command.
   * @param command The command reference.
   */
  void cC_Lesh::Sound(cTable& command) {
    cTable& text = this->Get_Table_At_Address(command);
    std::string name = C_Lesh_String_To_Cpp_String(text);
    this->io->Play_Sound(name);
  }

  /**
   * Executes a timeout command.
   * @param command The command reference.
   */
  void cC_Lesh::Timeout(cTable& command) {
    int delay = this->Eval_Expression(command);
    this->io->Timeout(delay);
  }

  /**
   * Executes an output command.
   * @param command The command reference.
   */
  void cC_Lesh::Output(cTable& command) {
    cTable& text = this->Get_Table_At_Address(command);
    std::string output = C_Lesh_String_To_Cpp_String(text);
    int x = this->Eval_Expression(command);
    int y = this->Eval_Expression(command);
    int red = this->Eval_Expression(command);
    int green = this->Eval_Expression(command);
    int blue = this->Eval_Expression(command);
    this->io->Output_Text(output, x, y, red, green, blue);
  }

  /**
   * Executes a string command.
   * @param command The command reference.
   */
  void cC_Lesh::String(cTable& command) {
    cTable& text_1 = this->Get_Table_At_Address(command);
    cTable& text_2 = this->Get_Table_At_Address(command);
    std::string str_1 = C_Lesh_String_To_Cpp_String(text_1);
    std::string str_2 = C_Lesh_String_To_Cpp_String(text_2);
    cTable& result = this->Get_Table_At_Address(command);
    bool test = (str_1 == str_2);
    result.Write_Column((int)test);
  }

  /**
   * Executes a palette command.
   * @param command The command reference.
   */
  void cC_Lesh::Palette(cTable& command) {
    cTable& text = this->Get_Table_At_Address(command);
    std::string name = C_Lesh_String_To_Cpp_String(text);
    this->pp->Load_Palette(name);
  }

  /**
   * Executes a draw command.
   * @param command The command reference.
   */
  void cC_Lesh::Draw(cTable& command) {
    cTable& picture = this->Get_Table_At_Address(command);
    int x = this->Eval_Expression(command);
    int y = this->Eval_Expression(command);
    int mode = this->Eval_Expression(command);
    this->pp->Draw_Picture(picture, x, y, mode);
  }

  /**
   * Executes a clear command.
   * @param command The command reference.
   */
  void cC_Lesh::Clear(cTable& command) {
    sColor color;
    color.red = this->Eval_Expression(command);
    color.green = this->Eval_Expression(command);
    color.blue = this->Eval_Expression(command);
    this->pp->Clear_Screen(color);
  }

  /**
   * Executes a resize command.
   * @param command The command reference.
   */
  void cC_Lesh::Resize(cTable& command) {
    cTable& table = this->Get_Table_At_Address(command);
    int width = this->Eval_Expression(command);
    int height = this->Eval_Expression(command);
    table.Resize(width, height);
  }

  /**
   * Executes a column command.
   * @param command The command reference.
   */
  void cC_Lesh::Column(cTable& command) {
    cTable& table = this->Get_Table_At_Address(command);
    int index = this->Eval_Expression(command);
    cTable& result = this->Get_Table_At_Address(command);
    table.Move_To_Column(index);
    int number = table.Read_Column();
    result.Write_Column(number);
  }

  /**
   * Executes a stop command.
   * @param command The command reference.
   */
  void cC_Lesh::Stop(cTable& command) {
    this->status = eSTATUS_DONE;
  }

  /**
   * Pushes a value on the stack.
   * @param value The value to push.
   */
  void cC_Lesh::Stack_Push(int value) {
    (*this->memory)[this->stack_pointer].Rewind();
    (*this->memory)[this->stack_pointer].Write_Column(value);
    this->stack_pointer++;
  }

  /**
   * Pops a value from the stack.
   * @return The value at the top of the stack.
   */
  int cC_Lesh::Stack_Pop() {
    (*this->memory)[this->stack_pointer - 1].Rewind();
    int value = (*this->memory)[this->stack_pointer - 1].Read_Column();
    this->stack_pointer--;
    return value;
  }

  /**
   * Loads a file into memory.
   * @param name The name of the file to load.
   * @param memory The memory to load the file into.
   * @param address The address to load the file into.
   * @throws An error if the file could not be loaded.
   */
  void cC_Lesh::Load_File_List(std::string name, cMemory& memory, int address) {
    cFile file(name);
    file.Read();
    while (file.Has_More_Lines()) {
      std::string fname = file.Get_Line();
      cTable& table = memory[address++];
      table.Load_From_File(fname);
    }
  }

  /**
   * Loads a list of tables from a single file.
   * @param name The name of the file to load the tables from.
   * @param memory The memory to load the tables to.
   * @param address The address to load the tables to.
   * @throws An error if the tables could not be loaded.
   */
  void cC_Lesh::Load_Table_List(std::string name, cMemory& memory, int address) {
    cFile file(name);
    file.Read();
    while (file.Has_More_Lines()) {
      cTable& table = memory[address++];
      table.Rewind();
      // Read dimensions of table.
      std::string dimensions = file.Get_Line();
      cArray<std::string> pair = Parse_Sausage_Text(dimensions, "x");
      if (pair.Count() == 2) {
        int width = Text_To_Number(pair[0]);
        int height = Text_To_Number(pair[1]);
        table.Resize(width, height);
        for (int row_index = 0; row_index < height; row_index++) {
          std::string line = file.Get_Line();
          cArray<std::string> columns = Parse_Sausage_Text(line, " ");
          int column_count = columns.Count();
          for (int column_index = 0; column_index < column_count; column_index++) {
            table.Write_Column(Text_To_Number(columns[column_index]));
          }
          table.Move_To_Next_Row();
        }
      }
      else {
        throw cError("Missing width or height of table in " + name + ".");
      }
      table.Rewind(); // Reset the table after writing to it.
    }
  }

  /**
   * Extracts a C-Lesh string and converts it to a C++ string.
   * @param table The table containing the string.
   * @return The C++ string.
   */
  std::string cC_Lesh::C_Lesh_String_To_Cpp_String(cTable& table) {
    std::string text = "";
    int letter_count = table.Read_Column();
    while (letter_count > 0) {
      int letter = table.Read_Column();
      if (letter == '@') { // Placeholder
        int number = this->Stack_Pop();
        text += Number_To_Text(number); // Replace with number.
      }
      else {
        text += (char)letter;
      }
    }
    return text;
  }

  // **************************************************************************
  // Memory Implmentation
  // **************************************************************************

  /**
   * Creates a new memory module of the specified size.
   * @param count The number of tables in memory.
   * @param width The width of each table.
   * @param height The height of each table.
   */
  cMemory::cMemory(int count, int width, int height) {
    this->count = count;
    this->tables = new cTable* [count];
    for (int table_index = 0; table_index < count; table_index++) {
      this->tables[table_index] = new cTable(width, height);
    }
  }

  /**
   * Frees the memory module.
   */
  cMemory::~cMemory() {
    for (int table_index = 0; table_index < this->count; table_index++) {
      delete this->tables[table_index];
    }
    delete[] this->tables;
  }

  /**
   * Clears out the memory.
   */
  void cMemory::Clear() {
    for (int table_index = 0; table_index < this->count; table_index++) {
      this->tables[table_index]->Clear();
    }
  }

  /**
   * Accesses a table from memory given an address.
   * @param address The address of the table to access.
   * @return A reference to the table at the address.
   * @throws An error if the address is invalid.
   */
  cTable& cMemory::operator[](int address) {
    if ((address < 0) || (address >= this->count)) {
      throw cError("Invalid address accessed at " + Number_To_Text(address) + ".");
    }
    return *(this->tables[address]);
  }

}