#!/bin/sh
echo "Basic to SASM..."
./sbasc
echo "SASM to SBI..."
./sasmc -i test.sasm
echo "Running..."
./sbi out.sbi
exit
