# Infix/RPN calculator

A mini terminal calculator that combines infix and reverse polish notation

## Usage

After building with `make`, running `./calc` will start the program.

This calc takes any floating point value and the following operators/symbols: `+ - * / ( )`

### example

```
[/] % make
[/] % ./calc
% 1 + 2 * 3
= 7.000000
% 
```

## Operator Precidence

1. `+ -`
2. `* /`
3. `RPN`
4. `( )`
