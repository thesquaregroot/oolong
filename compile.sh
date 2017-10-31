#/bin/bash

##
## This script is probably only temporary (oolong should do this eventually).
##
## Until then, ensure oolong is built (i.e. run "make") then run this script
##  with your input file (e.g. "./compile.sh samples/hello-world.ool").  The
##  program will be compiled and linked into a file called a.out.
##
## You can run the resulting program via "./a.out".
##

if [ $# -ne 1 ]; then
    echo "Usage: $0 input_file"
    exit 1
fi

INPUT_FILE="$1"

SCRIPT_DIR="$(dirname $0)"

cd $SCRIPT_DIR
./oolong < $1 && gcc output.o lib/*.o

