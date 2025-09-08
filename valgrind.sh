#!/bin/bash
#valgrind  --suppressions=valgrind.supp --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./build/Desktop-Release/caos
#valgrind  --suppressions=valgrind.supp --leak-check=full --verbose ./build/Desktop-Release/caos
valgrind  --suppressions=valgrind.supp --leak-check=full --verbose ./build/Desktop-Debug/caos
