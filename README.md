# Console Application (PIPE-FIFO)
## Usage
1) g++ (GCC) 12.2.1 20230201

## Libraries
  - iostream (For printing data on screen);
  - string
  - unistd.h   // read
  - sys/wait.h // wait
  - sys/stat.h // umask mkfifo
  - fcntl.h    // open

# Description
Console application in which the user can perform activities on the file system
using process abstraction. Namely, creating a file/directory, copying/moving data,
changing rights, viewing contents.

Program work scenario:

1. The user interacts with the main process of the program (main function) and
selects the appropriate action (creating a file/directory,
copying/moving a file/directory to another directory, copying file data to another file,
writing accounting data to a file (for example, information about the contents of a directory),
for a file Change permissions for /directory, view contents of file/directory)

2. The appropriate action performed by the user must be transmitted
in the form of a command (text) to the process created for this purpose.
Use the pipe channel to transmit the command, and use the fifo channel to specify
the exact amount of data to be transferred (to_string() - converting an integer value
into a string and stoi() - converting a string into an integer value,
a function on the child process side).
