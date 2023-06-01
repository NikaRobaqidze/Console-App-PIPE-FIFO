/*
 * Author: Nikoloz Robakidze
 * Email: nika42568@gmail.com

 * Console application in which the user can perform activities on the file system
 * using process abstraction. Namely, creating a file/directory, copying/moving data,
 * changing rights, viewing contents.

 * Program work scenario:

 * 1. The user interacts with the main process of the program (main function) and
 * selects the appropriate action (creating a file/directory,
 * copying/moving a file/directory to another directory, copying file data to another file,
 * writing accounting data to a file (for example, information about the contents of a directory),
 * for a file Change permissions for /directory, view contents of file/directory)

 * 2. The appropriate action performed by the user must be transmitted
 * in the form of a command (text) to the process created for this purpose.
 * Use the pipe channel to transmit the command, and use the fifo channel to specify
 * the exact amount of data to be transferred (to_string() - converting an integer value
 * into a string and stoi() - converting a string into an integer value,
 * a function on the child process side).
*/

#include <iostream>
#include <string>
#include <unistd.h>   // read
#include <sys/wait.h> // wait
#include <sys/stat.h> // umask mkfifo
#include <fcntl.h>    // open

using namespace std;

/*
 * Commander - class for working with terminal command.
 * Printing list of commands to generate and execute in future.
 * For command executing using std::system.
 * Overred input >> and output << operators.
 */
class Commander
{

    string command; // Terminal command.

public:
    Commander() : command("echo \"Invalid value.\""){};

    Commander(string usrCommand) : command(usrCommand){};

    void printCommandList() const
    {

        // Create interface for application.

        cout << endl
             << "Make the appropriate selection:" << endl
             << " 1. Create directory" << endl
             << " 2. Create file" << endl

             << " 3. Copy the file" << endl
             << " 4. Copy directory" << endl

             << " 5. Moving a file / directory" << endl

             << " 6. Copying file data to another file" << endl
             << " 7. Changing rights for a file / directory" << endl

             << " 8. Viewing file contents" << endl
             << " 9. Browse the contents of the directory" << endl

             << " 10. Recording accounting data in the file" << endl
             << " 11. Exit the program" << endl;

        cout << endl
             << "Your choice: ";
    }

    void setCommand(int commandNumber)
    {

        // commandNumber - number from interface to generate terminal command.

        // Set commands to numbers and set value to command key.
        switch (commandNumber)
        {

        case 1:
        {
            cout << endl
                  << "Enter directory name: ";

            string newDirectoryName;
            cin >> newDirectoryName;

            this->command = "mkdir " + newDirectoryName;
            break;
        }

        case 2:
        {
            cout << endl
                  << "Enter filename: ";

            string newFileName;
            cin >> newFileName;

            this->command = "touch " + newFileName;
            break;
        }

        case 3:
        {
            cout << endl
                  << "Enter filename: ";

            string fileName, destinationDirectory;
            cin >> fileName;

            cout << endl
                  << "Enter the name of the destination directory for the file copy: ";
            cin >> destinationDirectory;

            this->command = "cp \"" + fileName + "\" \"" + destinationDirectory + "/" + fileName + "\"";
            break;
        }

        case 4:
        {
            cout << endl
                  << "Enter directory name: ";

            string directoryName, destinationDirectory;
            cin >> directoryName;

            cout << endl
                  << "Enter the directory copy destination name: ";
            cin >> destinationDirectory;

            this->command = "cp -r \"" + directoryName + "\" \"" + destinationDirectory + "\"";
            break;
        }

        case 5:
        {
            cout << endl
                  << "Enter file/directory name: ";

            string objectName, newDirectoryName;
            cin >> objectName;

            cout << endl
                  << "Enter the new directory name of the file / directory: ";
            cin >> newDirectoryName;

            this->command = "mv \"" + objectName + "\" \"" + newDirectoryName + "\"";
            break;
        }

        case 6:
        {
            cout << endl
                  << "Enter filename: ";

            string fileName, copyFileName;
            cin >> fileName;

            cout << endl
                  << "Import a copy of the file filename: ";
            cin >> copyFileName;

            this->command = "cp \"" + fileName + "\" \"" + copyFileName + "\"";
            break;
        }

        case 7:
        {
            cout << endl
                  << "Enter file/directory name: ";

            string objectName, modeCode;
            cin >> objectName;

            cout << endl
                  << "Enter file/directory permissions: ";
            cin >> modeCode;

            this->command = "chmod " + modeCode + " \"" + objectName + "\"";
            break;
        }

        case 8:
        {
            cout << endl
                  << "Enter filename: ";

            string fileName;
            cin >> fileName;

            this->command = "cat \"" + fileName + "\"";
            break;
        }

        case 9:
        {
            cout << endl
                  << "Enter directory name: ";

            string directoryName;
            cin >> directoryName;

            this->command = "ls -al \"" + directoryName + "\"";
            break;
        }

        case 10:
        {
            cout << endl
                  << "Enter file name to record accounting data: ";

            string fileName, directoryName;
            cin >> fileName;

            cout << endl
                  << "Enter the name of the accounting directory: ";
            cin >> directoryName;

            // echo "$(pwd) - $(ls -l)" > information.txt

            this->command = "echo \"" + directoryName + " - $(ls -l \"" + directoryName + "\")\" > " + "\"" + fileName + "\"";
            break;
        }

        case 11:
        {
            cout << endl
                  << "Successfully exited the program." << endl;
            exit(0);
        }

        default:
        {
            cout << endl
                  << "Such action is not defined. Try again." << endl;
            break;
        }
        }
    }

    void setCommandFromBuffer(char *buff, size_t size)
    {

        /*
         * Convert C string to C++ standart and set value to command key.
         * It neccessary for reading data from PIPE.
         */

        this->command = "";

        for (int i = 0; i < size; i++)
        {

            this->command += buff[i];
        }
    }

    string getCommand() const { return command; }

    void executeCommand() const { system(this->command.c_str()); }

    friend istream &operator>>(istream &in, Commander &obj)
    {

        int commandNumber;
        in >> commandNumber;

        obj.setCommand(commandNumber);

        return in;
    }

    friend ostream &operator<<(ostream &out, Commander &obj)
    {

        out << obj.getCommand();

        return out;
    }
};

/*
 * myPipe - using PIPE to move string data between processes.
 * In this case PIPE using to transfer terminal command string from main
 * to created process for command executing. Input command in main and
 * executing it in child process.
 */
class myPipe
{
    // Declare keys to save stream value of data transfer.
    int rd_stream, // Read stream.
        wr_stream; // Write stream.

public:
    myPipe()
    {
        int tmp[2]; // Array for data trasfering.

        // Try to create PIPE chain.
        if (pipe(tmp) == -1)
        {

            cout << "Pipe chanel create unabled." << endl;
            exit(-1); // Stop application if PIPE chain not created.
        }

        // Set data array for streams.
        rd_stream = tmp[0];
        wr_stream = tmp[1];
    }

    ssize_t write_data(char *data, size_t size) const
    {

        /* Write data into stream. */

        return write(wr_stream, data, size);
    }

    ssize_t write_data_from_str(string data, size_t size) const
    {

        /* Convert C++ string to C standart then write data into stream. */

        char buff[size];

        for (int i = 0; i < data.size(); i++)
        {
            buff[i] = data[i];
        }

        return write_data(buff, size);
    }

    ssize_t read_data(char *data, size_t size) const
    {

        /* Reading data from stream. */
        return read(rd_stream, data, size);
    }

    ~myPipe()
    {
        /* Close transfer stream at object destoying. */
        close(rd_stream);
        close(wr_stream);
    }
};

/*
 * myFifo - using FIFO to move size (count of char) of string of terminal command
 * to get correct size of data transfered by PIPE chain. Size of data must be converted
 * to C++ string standart to move it through FIFO chain.
 */
class myFifo
{
    int fd;     // Key to save descriptor of rendered FIFO file. (open)
    char *file; // Name of rendered FIFO file.

public:
    myFifo(char *fileName) : file(fileName)
    {
        umask(0); // Change the access permissions of rendered FIFO file.

        mkfifo(file, S_IFIFO | 0660); // Create FIFO channel.
        fd = open(file, O_RDWR);      // Open rendered FIFO file.
    }

    ssize_t write_data(char *data)
    {
        /* Write data to transfer it. */
        return write(fd, data, sizeof(data));
    }

    ssize_t read_data(char *data)
    {
        /* Write data to receive it. */
        return read(fd, data, sizeof(data));
    }

    ssize_t write_data_from_str(string data)
    {

        /* Convert C++ string to C standart then write data to transfer it. */

        char buff[data.size()];

        for (int i = 0; i < data.size(); i++)
        {

            buff[i] = data[i];
        }

        return write_data(buff);
    }

    ~myFifo()
    {
        close(fd);    /* Close opened in FIFO file at object destoying. */
        unlink(file); /* Deleting rendered FIFO file. */
    }
};

int main()
{

    char file[] = "size.fifo";
    myFifo mf(file);           // Initialize FIFO channel.
    myPipe *pp = new myPipe(); // Initialize PIPE channel.

    // Fork proccess for applications working.
    switch (fork())
    {

    case -1:
    {

        cout << "Child process could not be created." << endl;
        break;
    }

    case 0:
    {
        /*
         * Child process.
         * For executing of selected by user terminal command.
         */

        wait(nullptr);
        // Block process, waiting while main process will end.

        char data[1024];    // Array to save data recieved from FIFO channel.
        mf.read_data(data); // Saving data recieved from FIFO channel.

        string str(data);           // Convert recieved data to string.
        const size_t N = stoi(str); // Parse string to integer.

        char buff[N];           // Array to save data recieved from PIPE channel.
        pp->read_data(buff, N); // Saving data recieved from PIPE channel.

        Commander *commander = new Commander();

        commander->setCommandFromBuffer(buff, N);
        commander->executeCommand();

        break;
    }

    default:
    {

        /*
         * Parent process. (main)
         * To input and generate terminal command.
         */

        Commander *comander = new Commander();

        (*comander).printCommandList();
        cin >> *comander; // Input number of command.

        // Convert generated command C++ string to C standart.
        string data = (*comander).getCommand().c_str();
        ssize_t size = data.size(); // Count of chars int string.

        // Write stringified size into FIFO channel.
        mf.write_data_from_str(to_string(size));

        // Write generated terminal command into PIPE channel.
        pp->write_data_from_str(data, size);

        break;
    }
    }
}
