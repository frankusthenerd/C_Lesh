// ============================================================================
// C-Lesh Compiler
// Programmed by Francois Lamini
// ============================================================================
var fs = require("fs");
var path = require("path");

const eCODE_STORE = 0;
const eCODE_DUMP = 1;
const eCODE_TEST = 2;
const eCODE_JUMP = 3;
const eCODE_CALL = 4;
const eCODE_RETURN = 5;
const eCODE_PUSH = 6;
const eCODE_POP = 7;
const eCODE_LOAD = 8;
const eCODE_SAVE = 9;
const eCODE_INPUT = 10;
const eCODE_REFRESH = 11;
const eCODE_SOUND = 12;
const eCODE_TIMEOUT = 13;
const eCODE_OUTPUT = 14;
const eCODE_STRING = 15;
const eCODE_PALETTE = 16;
const eCODE_DRAW = 17;
const eCODE_CLEAR = 18;
const eCODE_RESIZE = 19;
const eCODE_COLUMN = 20;
const eCODE_STOP = 21;

const eADDRESS_VALUE = 0;
const eADDRESS_IMMEDIATE = 1;
const eADDRESS_POINTER = 2;
const eADDRESS_STACK = 3;
const eADDRESS_OBJECT_IMMEDIATE = 4;
const eADDRESS_OBJECT_POINTER = 5;

var $tokens = [];
var $symtab = {};
var $placeholders = {};
var $pcode = [];
var $root = __dirname;

// ****************************************************************************
// Table Implementation
// ****************************************************************************

class cTable {

  /**
   * Creates a new table.
   * @param width The width of the table.
   * @param height The height of the table.
   */
  constructor(width, height) {
    this.width = width;
    this.height = height;
    // Allocate new rows.
    this.rows = [];
    for (var row_index = 0; row_index < height; row_index++) {
      var row = [];
      for (var col_index = 0; row_index < width; row_index++) {
        row.push(0);
      }
      this.rows.push(column);
    }
    this.row_pointer = 0;
    this.col_pointer = 0;
  }

  /**
   * Moves to the next row. Grows the table if needed.
   */
  Move_To_Next_Row() {
    if (this.row_pointer == this.height) {
      this.Resize(this.width, this.height + 1);
    }
    this.row_pointer++;
    this.col_pointer = 0;
  }

  /**
   * Writes a value to a column.
   * @param value The value to write.
   */
  Write_Column(value) {
    if (this.col_pointer == this.width) {
      this.Resize(this.width + 1, this.height);
    }
    this.rows[this.row_pointer][this.col_pointer++] = value;
  }

  /**
   * Resizes the table to new dimensions.
   * @param width The new width.
   * @param height The new height.
   */
  Resize(width, height) {
    var rows = [];
    for (var row_index = 0; row_index < height; row_index++) {
      var row = [];
      for (var col_index = 0; col_index < width; col_index++) {
        row.push((this.rows[row_index][col_index] != undefined) ? this.rows[row_index][col_index] : 0);
      }
      rows.push(row);
    }
    this.rows = rows;
    this.width = width;
    this.height = height;
  }

  /**
   * Resets the pointers so the first cell is pointed to.
   */
  Rewind() {
    this.row_pointer = 0;
    this.col_pointer = 0;
  }

  /**
   * Moves to a row.
   * @param index The row index. 
   * @throws An error if the row is out of bounds.
   */
  Move_To_Row(index) {
    if ((index >= 0) && (index < this.height)) {
      this.row_pointer = index;
    }
    else {
      throw new Error("Invalid row index " + index + ".");
    }
  }

  /**
   * Moves to a specified column.
   * @param index The column index.
   * @throws An error if the column is out of bounds.
   */
  Move_To_Column(index) {
    if ((index >= 0) && (index < this.width)) {
      this.col_pointer = index;
    }
    else {
      throw new Error("Invalid column index " + index + ".");
    }
  }

  /**
   * Returns a string representation of the table.
   */
  Serialize() {
    var text = [ String(this.width + "x" + this.height) ];
    var row_count = this.height;
    var col_count = this.width;
    for (var row_index = 0; row_index < row_count; row_index++) {
      var row = [];
      for (var col_index = 0; col_index < col_count; col_index++) {
        row.push(this.rows[row_index][col_index]);
      }
      text.push(row.join(" "));
    }
    return text.join("\n");
  }

}

// ****************************************************************************
// General API
// ****************************************************************************

/**
 * Initializes the compiler.
 */
function Init() {
  if (process.argv.length == 3) {
    try {
      var source = process.argv[2];
      Parse_Source(source);
      Parse_Statements();
    }
    catch (error) {
      console.log("Error: " + error.message);
    }
  }
  else {
    console.log("Usage: " + process.argv[1] + " <source>");
  }
}

/**
 * Parses a source file into tokens.
 * @param name The name of the file.
 * @throws An error if the source could not be parsed.
 */
function Parse_Source(name) {
  try {
    var data = fs.readFileSync(name + ".clsh", "utf8");
    var lines = Split(data);
    var line_count = lines.length;
    for (var line_index = 0; line_index < line_count; line_index++) {
      var line = lines[line_index];
      if (line.length > 0) {
        if (line.charAt(0) == ":") { // Code line.
          var code = line.substr(1);
          if (code.match(/^\-\s+\w+\s+\-$/)) { // Import
            var source = code.replace(/^\-\s+(\w+)\s+\-$/, "$1");
            Parse_Source(source);
          }
          else { // Code
            var tokens = Parse_Tokens(code);
            var token_count = tokens.length;
            for (var token_index = 0; token_index < token_count; token_index++) {
              var token = tokens[token_index];
              $tokens.push({
                token: token,
                line_no: line_index,
                source: name
              });
            }
          }
        }
      }
    }
  }
  catch (error) {
    throw new Error("Could not read source " + name + ". (Error: " + error.message + ")");
  }
}

/**
 * Splits data into platform independent lines.
 * @param data The data string to split.
 * @return An array of lines without blanks.
 */
function Split(data) {
  var lines = data.split(/\r\n|\r|\n/);
  // Remove any carrage return at the end.
  var line_count = lines.length;
  var blanks = 0;
  for (var line_index = line_count - 1; line_index >= 0; line_index--) { // Start from back.
    var line = lines[line_index];
    if (line.length == 0) {
      blanks++;
    }
    else {
      break;
    }
  }
  return lines.slice(0, line_count - blanks);
}

/**
 * Parses tokens from a line of text.
 * @param line The line containing the tokens.
 * @return An array of parsed tokens.
 */
function Parse_Tokens(line) {
  var tokens = line.split(/\s+/);
  var cleaned_tokens = [];
  var token_count = tokens.length;
  for (var token_index = 0; token_index < token_count; token_index++) {
    var token = tokens[token_index];
    if (token.length > 0) { // Don't add NULL tokens.
      cleaned_tokens.push(token);
    }
  }
  return cleaned_tokens;
}

/**
 * Parses the statements in the code.
 */
function Parse_Statements() {
  while ($tokens.length > 0) {
    var command = Parse_Token();
    if (command.token == "define") {
      var name = Parse_Token();
      Parse_Keyword("as");
      var number = Parse_Token();
      $symtab["[" + name.token + "]"] = Text_To_Number(number.token);
    }
    else if (command.token == "map") {
      var entry = Parse_Token();
      var index = 0;
      while (entry.token != "end") {
        $symtab["[" + entry.token + "]"] = index++;
        entry = Parse_Token();
      }
    }
    else if (command.token == "object") {
      var name = Parse_Token();
      var entry = Parse_Token();
      var index = 0;
      while (entry.token != "end") {
        $symtab["[" + name + "->" + entry.token + "]"] = index++;
        entry = Parse_Token();
      }
    }
    else if (command.token == "label") {
      var name = Parse_Token();
      $symtab[name.token] = $pcode.length;
    }
    else if (command.token == "string") {
      Parse_C_Lesh_String();
    }
    else if (command.token == "number") {
      var number = Text_To_Number(Parse_Token().token);
      Write_Number(number);
    }
    else if (command.token == "list") {
      var count = Text_To_Number(Parse_Token().token);
      for (var item_index = 0; item_index < count; item_index++) {
        Write_Number(0);
      }
    }
    else if (command.token == "matrix") {
      var width = Text_To_Number(Parse_Token().token);
      var height = Text_To_Number(Parse_Token().token);
      var table = new cTable(width, height);
      $pcode.push(table);
    }
    else if (command.token == "matrices") {
      var width = Text_To_Number(Parse_Token().token);
      var height = Text_To_Number(Parse_Token().token);
      Parse_Keyword("count");
      var count = Text_To_Number(Parse_Token().token);
      for (var matrix_index = 0; matrix_index < count; matrix_index++) {
        var table = new cTable(width, height);
        $pcode.push(table);
      }
    }
  }
}

/**
 * Parses a token from the token stack.
 * @throws An error if there are no more tokens.
 */
function Parse_Token() {
  var token = {};
  if ($tokens.length > 0) {
    token = $tokens.shift();
  }
  else {
    throw new Error("No more tokens!");
  }
  return token;
}

/**
 * Parses a keyword.
 * @param keyword The keyword to check for.
 * @throws An error if the keyword is not there.
 */
function Parse_Keyword(keyword) {
  var token = Parse_Token();
  if (token.token != keyword) {
    Generate_Parse_Error("Missing keyword " + keyword + ".", token);
  }
}

/**
 * Generates a parse error.
 * @param message The error message.
 * @param token The token being parsed.
 * @throws An error.
 */
function Generate_Parse_Error(message, token) {
  throw new Error("Token: " + token.token + "\nLine: " + token.line_no + "\nSource: " + token.source + "\nError: " + message);
}

/**
 * Converts a text to a number.
 * @param text The numeric text.
 * @return A number.
 * @throws An error if the number is not valid.
 */
function Text_To_Number(text) {
  var number = 0;
  if (text == "0") {
    number = 0;
  }
  else if (text.match(/^[1-9][0-9]*$/)) {
    number = parseInt(text);
  }
  else if (text.match(/^\-[1-9][0-9]*$/)) {
    number = parseInt(text);
  }
  else {
    throw new Error("Number " + text + " is not value.");
  }
  return number;
}

/**
 * Parses a C-Lesh String.
 * @throws An error if the string could not be parsed.
 */
function Parse_C_Lesh_String() {
  var string = "";
  var token = Parse_Token();
  if (token.token.match(/^"[^"]*"$/)) { // Single token string.
    string = token.token.substr(1, token.token.length - 2);
  }
  else if (token.token.match(/^"[^"]+/)) { // Multitoken string.
    string = token.token.substr(1) + " "; // Leave out first quote.
    token = Parse_Token();
    while (!token.token.match(/^[^"]+"$/)) {
      if (token.token.match(/"/)) {
        Generate_Parse_Error("String is invalid.", token);
      }
      string += String(token.token + " ");
      token = Parse_Token();
    }
    string += token.token.substr(0, token.token.length - 1); // Leave out last quote.
  }
  else {
    Generate_Parse_Error("String is invalid.", token);
  }
  // Write out the string.
  Write_C_Lesh_String(string);
}

/**
 *  Writes a pseudo code number.
 * @param number The number to write to the code.
 */
function Write_Number(number) {
  var table = new cTable(1, 1);
  table.Write_Column(number);
  $pcode.push(table);
}

/**
 * Writes a C-Lesh string.
 * @param text The text to write.
 */
function Write_C_Lesh_String(text) {
  var count = text.length;
  var table = new cTable(count + 1, 1); // Extra space for count.
  table.Write_Column(count);
  // Write out characters.
  for (var letter_index = 0; letter_index < count; letter_index++) {
    var letter = string.charCodeAt(letter_index);
    table.Write_Column(letter);
  }
  $pcode.push(table);
}

/**
 * Parses a reference.
 * @param command The command where the reference is.
 * @throws An error if the reference is invalid.
 */
function Parse_Reference(command) {
  var token = Parse_Token();
  if (token.token.length > 1) {
    var mode = token.token.substr(0, 1);
    var address = token.token.substr(1);
    if (mode == "$") { // Immediate value.
      command.Write_Column(eADDRESS_VALUE);
      command.Write_Column(Parse_Number_Or_Placeholder(address, command));
    }
    else if (mode == "#") { // Immediate address.
      command.Write_Column(eADDRESS_IMMEDIATE);
      command.Write_Column(Parse_Number_Or_Placeholder(address, command));
    }
    else if (mode == "@") { // Pointer
      command.Write_Column(eADDRESS_POINTER);
      command.Write_Column(Parse_Number_Or_Placeholder(address, command));
    }
    else if (mode == "^") { // Stack address.
      command.Write_Column(eADDRESS_STACK);
      command.Write_Column(Parse_Number_Or_Placeholder(address, command));
    }
    else if (mode == "&") { // Object immediate address.
      command.Write_Column(eADDRESS_OBJECT_IMMEDIATE);
    }
    else if (mode == "%") { // Object pointer address.
      command.Write_Column(eADDRESS_OBJECT_POINTER);
    }
    else {
      Generate_Parse_Error("Invalid address mode.", command);
    }
  }
  else {
    Generate_Parse_Error("Reference is too short.", command);
  }
}

/**
 * Parses a number or placeholder from text.
 * @param text The number or placeholder text.
 * @parma command The command where to placeholder or number is.
 * @return A number. It may be zero.
 */
function Parse_Number_Or_Placeholder(text, command) {
  var number = 0;
  try {
    number = Text_To_Number(text);
  }
  catch (error) { // We have a placeholder.
    var placeholder = {
      y: command.row_pointer,
      x: command.col_pointer,
      text: text
    };
    var address = $pcode.length;
    if ($placeholders[address]) {
      $placeholders[address].push(placeholder);
    }
    else {
      $placeholders[address] = [ placeholder ];
    }
  }
  return number;
}

/**
 * Parses an object reference.
 * @param text The text representing the address.
 * @param command The associated command.
 * @return A pair of numbers.
 * @throws An error if the object reference is invalid.
 */
function Parse_Object_Reference(text, command) {
  var addresses = [ 0, 0 ];
  var obj_address = text.split(/:/);
  if (obj_address.length == 2) {
    // TODO: Need to make this work with Eval_Expression() as well!
    var address = Parse_Number_Or_Placeholder(obj_address[0], command);
    var index = Parse_Number_Or_Placeholder(obj_address[1], command);
  }
  else {
    
  }
}

// ****************************************************************************
// Program Entry Point
// ****************************************************************************

Init();
