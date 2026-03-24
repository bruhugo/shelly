## SHELLY - interactive shell for Linux in C++

In order to learn a bit more about system calls and processes on UNIX systems, I decided to create my own shell from scratch

To make sure it was working, all the git related stuff in the project was done using the very shell.

It provides some basic to (kind of) advanced features such as:
- Change working directory
- Execute commands
- Quote based arguments
- Pipes 
- Redirecting stdout/stderr


## Running programs

Just type the command name and the arguments. Shell will look for the program in the path env variable directories.

Works with quoted args to.

```
echo "Hello World!"
```

## Pipes

Just like im bash. Put a '|' character between the commands, and the shell will create a pipe between them.

```
cat main.cpp | grep if
```

## Redirecting stdout/stderr

You can redirect the stdout with "o>>" for appending to a file, or "o>" for overriding it.

The same goes for "e>>" and "e>" for stderr

```
echo "10-10-2032 DEBUG log message" o>> logs
```

## How to use it on your machine
You will need Linux, a C++ compiler and git and make installed.

First, clone the repo:
```
git clone https://github.com/bruhugo/shelly
```

Open the project:
```
cd shelly
```

Now just run:
```
make run
```

