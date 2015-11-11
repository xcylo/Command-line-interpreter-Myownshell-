Created by Aleksandr Dobrev

This is a command-line-interpreter that I have built. 

Includes:
+Redirection handling
	-in (<)
	-out (>)
	-piping (|)
+Backgrouding support (&)
+Built-in commands
	-myecho
	-kill
	-cd
	-exit
+Colors ;)
+Other shell stuff

---------------------------------

All libraries needed for this to work are in included in the /include folder if needed.

Makefile is provided for convienence. Also creates backup copies if necessary (see makefile)

my_shell.c is the source code created by me.

makeargv.c is a support file not created by me which simple program that breaks up the user 'string' input into an array of chunchs using a space as a delimiter.


