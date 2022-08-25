#include <Arduino.h>

#include "uCLI.hpp"

using uCLI::CLI;
using uCLI::Command;
using uCLI::StreamEx;
using uCLI::Tokens;

StreamEx serial_ex(Serial);
CLI<> serial_cli(serial_ex);

void setup() {
  Serial.begin(9600);
  while (!Serial) {}
}

void do_add(Tokens);
void do_echo(Tokens);

void loop() {
  // command list can be global or static local
  static const Command commands[] = {
    { "add", do_add }, // call do_add when "add" is entered
    { "echo", do_echo }, // call do_echo when "echo" is entered
  };
  serial_cli.prompt(commands);
}

void do_add(Tokens args) {
  int a = atoi(args.next());
  int b = atoi(args.next());
  serial_ex.print(a);
  serial_ex.print(" + ");
  serial_ex.print(b);
  serial_ex.print(" = ");
  serial_ex.println(a + b);
}

void do_echo(Tokens args) {
  serial_ex.println(args.next());
}
