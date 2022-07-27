#!/usr/bin/env ruby

print "Enter number1: ";
num1 = gets.chomp.to_i;  

print "Enter number2: ";
num2 = gets.chomp.to_i;  

print "Enter number3: ";
num3 = gets.chomp.to_i;  

if(num1>num2)
    if(num1>num3)
        large=num1;
    else
        large=num3;
    end
elsif(num2>num3)
    large=num2;
else
    large=num3;
end

print "Largest number is: ",large;

