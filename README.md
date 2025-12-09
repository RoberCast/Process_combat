# Process combat

## Introduction
A combat takes place between several child processes, arbitrated by the parent process. Each child process randomly decides whether to attack or defend. Attacking processes enter a defenseless state before initiating their attack and can be ambushed, setting their state to "KO". In the case of defense, the process can repel the attack, and its state is set to "OK". Attacking processes randomly choose a valid PID from the process list (non-zero and different from their own PID) to launch the attack. At the end of the round, the child process sends the parent process a message with the round's result, i.e., its state. The parent process reads the round's result, terminates the "KO" processes, updates the surviving processes, and starts a new round as long as two or more contenders remain; otherwise, the rounds end. The final result can be a tie, if all child processes have eliminated each other, or a victory for a child process. The final result is printed. At completion, the parent process demonstrates that the IPC resources have been released.

Additionally, a bash script called *run_processCombat.sh* is created that does the following:

* Compile using *gcc* the *father.c* and *child.c* sources, creating the *FATHER* and *CHILD* executables.
* Create a FIFO with the result called *result*.
* Launch a *cat* process in the background waiting to read the result from the FIFO *result* file.
* Execute *FATHER* by passing it 10 as an argument; this consists of creating 10 child processes, the contenders.
* After the *cat* command finishes executing, it deletes all the executable files created and the FIFO *result* file, leaving only the source files *father.c* and *child.c*.

## Instructions
To run *process combat*, you need to copy the files *father.c*, *child.c*, and *run_processCombat.sh* to a folder of your choice. Then, from the operating system's terminal, run the command *./run_processCombat.sh*.

## Demo
The following image shows the execution of *process combat*.

![processCombat_runs](Images/processCombat.jpg)

## License
This project is licensed under the GNU General Public License v3 (GPLv3).

© 2025 Roberto Castillejo Embid.
See the [LICENSE](./LICENSE) file for more details.
