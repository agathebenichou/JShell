CONTENTS OF THIS FILE
---------------------
* Introduction
* Getting Started
* Authors
* Contributors

Introduction
---------------------
The assignment was to implement a mini shell for Linux. The purpose of the project was to improve our understanding of C programming and our system programming skills, as well as better our knowledge of the Unix programming environment and shells in particular. Tasks associated with the assignment include creating pipelines and implementing stdin, stdout, stderr, PATH, and environment variables.

JShell is an implementation of a mini shell for Linux. This mini shell, which was written in the C programming language, inherits environment variables from the process that starts it. It is possible to set and remove environment variables, as well as print out the value of the environment variable. Any process created by the shell to execute a command inherits all the shells current environment variables.  

JShell can support a pipeline, which  is a sequence of processes chained together by their standard streams so that the output of each process feeds directly as the input to the next. For example, a command like 'ls -l | more' sends the output of the ls command to the input of the more command. This particular pipeline allows a user to page through a directory listing. 

JShell has the ability to enable process statistics on each external command executed, using the built-in command stats and a series of flags. Additionally, the mini shell is able to execute any default Linux commands, such as ls, more, cat, echo, etc.

Getting Started
---------------------
To run the program, there are several compilation steps to take - all of which should be run in the JShell/src directory:

First, run the make command to build the executable programs and libraries from the source code.
Next, run ./jshell to launch the executable program for the mini shell. 

The shell will prompt lsh> which means that is it ready to execute user text commands. 

Authors
---------------------
Written by Agathe Benichou (benichoa@lafayette.edu) and Shira Wein (weins@lafayette.edu) at Lafayette College in Easton, PA.

Contributors
---------------------
Thank you to our professor, Joann Ordille, for her assistance.