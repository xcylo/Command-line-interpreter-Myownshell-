/*
Created by: Aleksandr Dobrev
March of 2015

My shell interpreter
*/

#include "my_shell.h"
#include "textcolor.h"

//MACROS//
#define INITIAL_BLOCK 16 //Initial size for user input (testing getlines realloc)

#define DEBUGSWITCH	0 //1 to print debug statements, 0 to turn off.
#define DEBUGSTRING(line)        if(DEBUGSWITCH) printf("%s\n",line);
#define DEBUGINT(comment,line)   if(DEBUGSWITCH) printf("%s%i\n", comment,line);
#define DEBUGSIZET(comment,line) if(DEBUGSWITCH) printf("%s%zu\n", comment,line);
///////////////////////////////////////////////////////////////////////////////////

//General functions dealing with startup/cleanup of the shell itself.
int shell_body();
int cleanup(void**);
int spawn_process(char*[], int);

//Functions dealing with redirects and handling forked processes as well as backrounding.
int multiprocess_handler(char*[], int );
int rd_pipe_handler(char** arg_ptrs, int argindex);
int rd_in_handler(char*);
int rd_out_handler(char*);
int contain_rd(char* );

//Built-in handlers
int built_in_handler(const char*[], int);
int is_built_in(char*);

//Built-in commands//
int my_echo_0(const char*[]);
int my_kill_1(const char*[]);
int my_exit_2(const char*[]);
int my_cd_3(const char*[]);
//////////////////////////////////

//Global vars.
char* built_ins[] = {"myecho","kill","exit","cd", NULL}; //built-in strings
int runstatus = 1; 
//////////////////////////////////

int main(int argc, char const *argv[])
{
 	printf(ANSI_COLOR_YELLOW"Welcome to my Shell!\nCreated by: Aleksandr Dobrev"ANSI_COLOR_RESET "\n");
	int shell_errcode = shell_body();
	
	return shell_errcode;
}

/*
=== shell_body function ===
>>> heart of the program. Initializes memory blocks, sets path, hostname, current directory,
> 	and handles user input by calling appropriate functions to deal with the request.
*/
int shell_body(){
	//Startup preperation//

	//ALL Pointer variables//
	char* cwd_path = malloc(MAX_PATH_SIZE*sizeof(char)); //path of current working directory.
	char* input = malloc(INITIAL_BLOCK*sizeof(char)); //user input into shell.
	char** arg_ptrs = NULL; //where makearv holds args parsed.
	char* hostname = malloc(64*sizeof(char)); //holds hostname of machine.
	void* ptrvector[] = {cwd_path, input, hostname, arg_ptrs}; //all ptrs that needs to be freed, go here.
	/////////////////////////////////////////////

	//Declared Variables//
	pid_t shells_pid = getpid(); //process id.
	size_t n_getline = (INITIAL_BLOCK*sizeof(char)); //getline n arguement.
	int num_of_args; //return of makeargv with '\n', '\t', ' '.
	int builtin_result; //return of is_built_in.
	/////////////////////////////////////////////////

	//error handling variables//
	int errorexit = 0;
	char* cwd_overflow; //used for error handling of getcwd().
	///////////////////////////////////////////////////////////////

	gethostname(hostname, 64); //gets hostname of machine
	//if your hostname > 64chars, you are part of the problem.

	//Let the shell begin...
	while(runstatus){
	//	signal(SIGCHLD, SIG_IGN);
		waitpid(WNOHANG);
		cwd_overflow = getcwd(cwd_path,MAX_PATH_SIZE);
		if(cwd_overflow == NULL) perror("getcwd"); //means cwd is overflowing if true.

		printf(BOLD_ON ANSI_COLOR_GREEN"%s@%s"ANSI_COLOR_BLUE" %s [PID:%li] $ "
			ANSI_COLOR_RESET BOLD_OFF,getenv("USER"),hostname,cwd_path,shells_pid);
		getline(&input, &n_getline, stdin);

		num_of_args = makeargv(input, " \n\t", &arg_ptrs); //split input.

		if(num_of_args != 0){
			builtin_result = is_built_in(arg_ptrs[0]);

			if(builtin_result >= 0){
				built_in_handler(arg_ptrs,builtin_result);
			} else{
				if((*arg_ptrs[num_of_args-1] == '&') || (contain_rd(input) != 0)){
					signal(SIGCHLD, SIG_DFL);
					pid_t newpid = fork();
					if(newpid > 0){ //if parent.
						if(*arg_ptrs[num_of_args-1] == '&') printf("Putting process %li in background.\n",newpid+1 );
						else {
							waitpid(newpid); //must have both waits in this order otherwise death.
							wait(NULL);
						}
					} else{ //if child 
						if(*arg_ptrs[num_of_args-1] == '&') arg_ptrs[num_of_args-1] = '\0';
						multiprocess_handler(arg_ptrs,num_of_args-1);
						return 1; //1 represents return of child process as exit code.
						exit(0); //you can never be too safe...
					}
				} else{
					//no backgrounding or redirects. Deals with request in initial shell process.
					multiprocess_handler(arg_ptrs,num_of_args); 
				}
			}
		}
	}
	cleanup(ptrvector);
	return errorexit;
}

/*
===cleanup function===
+takes in an array of pointers that need to be freed.
+ptrvectory must have a null terminating pointer.
>>>frees all the pointers requested.
=returns 0 on successful completion.
*/
int cleanup(void* ptrvector[]){
	int i = 0;
	while(ptrvector[i] != NULL){
		free(ptrvector[i]);
		i++;
	}
	return 0;
}

/*
=== multiprocesss_handler function ===
+++ Takes in a array of pointers to args and an iterator value.
>>> Iterator value defines current arg processed and decrements after
> 	it is dealt with. Handles which redirect/pipe function to call
>	if needed and lastly execs when leftmost arg is reached.
>	Closes all open file descriptors prior to returning.
*/
int multiprocess_handler(char** arg_ptrs, int itr){
	int fd_in = -1, fd_out = -1, fd_pipe = -1;
	
	while(itr >= 0){
		if(itr == 0){ //leftmost command.
				spawn_process(arg_ptrs, itr);
	
				if(fd_out != -1) close(fd_out); //cleanup.
				if(fd_pipe != -1) close(fd_pipe);
				if(fd_in != -1) close(fd_in); 
 	
			return 0;
		}
		itr--;
		switch (*arg_ptrs[itr]){
			case '>':
				arg_ptrs[itr] = '\0'; //Null >.
				fd_out = rd_out_handler(arg_ptrs[itr+1]);
				break;
			case '<': 
				arg_ptrs[itr] = '\0'; //Null <.
				fd_in = rd_in_handler(arg_ptrs[itr+1]);
				break;	
			case '|':
				arg_ptrs[itr] = '\0'; //Null |.
				fd_pipe = rd_pipe_handler(arg_ptrs, itr);
				break;	
		}
	}
	return 1; //should never get here unless stack overflow.
}


/*
=== rd_pipe_handler ===
+++takes in array of strings with args and and index of current arg being dealt with.
>>> creates a pipe between current arg and the next one preceding it.
RET: returns file descriptor of open end of pipe. If -1 is returned
> 	then there was an error with opening pipe.
*/
int rd_pipe_handler(char** arg_ptrs, int argindex){
	int pipefd[2];
	pipe(pipefd);
	pid_t pipe_pid = fork();
	int retfd = -1; //ret of open pipe. set as error by default.

	if (pipe_pid == 0){ //Going out of pipe (parent).
		pipefd[1] = dup2(pipefd[1], 1); //1 is standard OUTPUT.
		close(pipefd[0]);
		retfd = pipefd[1];
	} else{ //Going into the pipe (child).
		pipefd[0] = dup2(pipefd[0], 0); //0 is standard INPUT.
		if (close(pipefd[1]) < 0) perror("close");
		retfd = pipefd[1];
		execvp(arg_ptrs[argindex+1],&arg_ptrs[argindex+1]);
		fflush(1); //flush std output so other crap doesn't end up as input.
		waitpid(pipe_pid);
	}
	return retfd;
}

/*
=== rd_out_handler ===
+++ ARGS: Path of output file.
>>> Redirect stdout to given outputfile. If file doesn't exist, then creates it.
RET: returns file descriptor of opened file.
*/
int rd_out_handler(char* f_path){
	int fd;
	int open_flags = (O_CREAT | O_RDWR | O_TRUNC);
	mode_t mode = S_IRWXU;
	fd = open(f_path,open_flags,mode);
	if(fd < 0) perror("open");
	if(dup2(fd,1) < 0) perror("dup");

	return fd;
}

/*
=== rd_in_handler ===
+++ ARGS: Path of input file.
>>> Redirect given input file to stdin
RET: returns file descriptor of opened file.
*/
int rd_in_handler(char* f_path){
	int fd;
	int open_flags = O_RDONLY;
	mode_t mode = S_IRWXU;
	fd = open(f_path,open_flags,mode);
	if(fd < 0) perror("open");
	if(dup2(fd,0) < 0) perror("dup");

	return fd;
}

/*
=== contain_rd function
>Simply checks if the input has a ridirect or pipe.
>Returns 0 if not found or a number > 0 if rd is found.
*/
int contain_rd(char* input){
	if(strstr(input,">") != NULL) return 1;
	if(strstr(input,"<") != NULL) return 2;
	if(strstr(input,"|") != NULL) return 3;
	return 0;
}

/*
=== swawn_process function ===
+++ Args: array of string containing args, and index of arg to execute.
>>> Properly executes a process and handles all error checking.
RET: only returns if unsuccesfull exec or if parent process.
*/
int spawn_process(char** arg_ptrs, int argindex){
	pid_t spawned_pid = fork();
	if(spawned_pid == -1){
		printf("%s\n", "Failed to fork");
	} else if (spawned_pid == 0){ //then it is a child process.
		if(execvp(arg_ptrs[argindex], &arg_ptrs[argindex]) < 0){
			perror("execvp"); //error handling.
			exit(0);
		}
	} else{
		wait(NULL); //wait for child process to terminate.
		waitpid(spawned_pid); //MUST HAVE BOTH WAITS otherwise output lagged behind.
	}
	return 0;
}

/*
=== is_built_in function ===
+ takes in a command arg as string.
>>> Tests if it is a built-in command
= if it is built-in then returns its code;
= else returns -1;
*/
int is_built_in(char* arg){
	int i = 0;

	while(built_ins[i] != NULL){
		if(strcmp(built_ins[i], arg) == 0) return i;
		i++;
	}
	return -1;
}


/*
=== built_in_handler ===
+ takes in args and code of built-in command.
>>> executes the designated built-in command.
*/
int built_in_handler(const char** arg_ptrs, int built_in_command){
	switch(built_in_command){
		case 0: my_echo_0(arg_ptrs); break;
		case 1: my_kill_1(arg_ptrs); break;
		case 2: my_exit_2(arg_ptrs); break;
		case 3: my_cd_3(arg_ptrs);   break;
	}
	return 0;
}

///////////////////////
// BUILT-IN Commands //
///////////////////////
int my_echo_0(const char* arg_ptrs[]){
	printf("%s\n", arg_ptrs[1]);
	//Testing first built-ins. Use "myecho" to use this.
	return 2;
}

int my_kill_1(const char* arg_ptrs[]){
	if (arg_ptrs[1] != NULL){ //prevent user from segfault as no pid is given.
		char* signalparam = strstr(arg_ptrs[1], "-");
		if(signalparam == NULL){
			pid_t pid = atol(arg_ptrs[1]);
			if(kill(pid, 15) < 0) perror("kill"); //Default kill signal is TERM
			
		} else{
			if(arg_ptrs[2] != NULL){ //prevent user from segfault.
				pid_t pid = atol(arg_ptrs[2]);
				kill(pid,atoi(signalparam+1));
				perror("kill");
			}
		}
	}
	return 2;
}

int my_exit_2(const char* arg_ptrs[]){
	runstatus = 0;
	printf(ANSI_COLOR_YELLOW"It has been a pleasure serving you."ANSI_COLOR_RESET "\n");
	return 2;
}

int my_cd_3(const char* arg_ptrs[]){
	struct passwd *pw = getpwuid(getuid());
	const char *homedir = pw->pw_dir; //get home folder dir.

	if(arg_ptrs[1] == NULL){ //if no input then go to home dir.
		chdir(homedir);
	} else if(chdir(arg_ptrs[1]) < 0) { //attmept to move to given dir.
		perror(ANSI_COLOR_RED"chdir"ANSI_COLOR_RESET); //error handling.
	}
	return 2;
}
