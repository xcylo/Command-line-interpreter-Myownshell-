GCCFLAGS=	-Wall
SHELLHEADER=	my_shell.h
FIXPERMISSIONS=	chmod 755 $@
DATE=	$$(date +%F)

MyOwnShell:	$(SHELLHEADER) makeargv.c my_shell.c
	gcc $(GCCFLAGS) my_shell.c -o $@
	$(FIXPERMISSIONS)

#creates a duplicate shell source code file. Intended for simple version control within file.
backupshell:	my_shell.h makeargv.c my_shell.c
	gcc $(GCCFLAGS) my_shell.c -o $@
	$(FIXPERMISSIONS)

#Create a backup version of shell with todays date. (for daily progress)
storeshell:	my_shell.c
	cp my_shell.c my_shell_$(DATE).c

printdate:	
	echo $(DATE)

clean:	
	rm MyOwnShell* backupshell*
