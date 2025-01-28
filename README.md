# satoriNOW

This project is a CLI application intended to offer command line functionality to users of the Satori Network.

https://satorinet.io/

This project is not associated with the Satori project or the Satori Association.

# build

make ;

make install

# run

This project is still in development, but requires the execution of the server:

/path/to/project/build/satorinow

and a client-side CLI

/path/to/project/build/satoricli help

# valgrind

valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./build/satorinow 
