#include <mruby.h>
#include <mruby/compile.h>

int
main(void)
{
  mrb_state *mrb = mrb_open();
  if (!mrb) { /* handle error */ }
  // mrb_load_string(mrb, str) to load from NULL terminated strings
  // mrb_load_nstring(mrb, str, len) for strings without null terminator or with known length
  mrb_load_string(mrb, "puts \"Enter first value: \"\
num1=gets.chomp.to_i\
puts \"Enter second value: \"\
num2=gets.chomp.to_i\
\
sum=num1+num2\
\
puts \"The sum is #{sum}\"");
  mrb_close(mrb);
  return 0;
}

