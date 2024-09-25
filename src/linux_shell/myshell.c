#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#define STD_INPUT 0
#define STD_OUTPUT 1

int main(int argc, char *argv[]) {	
    while(1) {
	if(argc == 1 || strcmp(argv[1], "-n") != 0) {printf("myshell$ ");}
	char* buffer = (char *) malloc(512 * sizeof(char));
        pid_t pid;
        int status; //exit status of child process
	int delay = 0;
	char* execStr[10];
	char* execArgs[10][10]; //idea 1: eA[0] holds an array of arguemnts
	int fd[2]; //file descriptors for pipe [0] is read, [1] is write
	int gt = 0;
	int lt = 0;
	int amp = 0;
	int error = 0;
	char* infile;
	char* outfile;
	//char* args[50]; 
        fflush(stdout);
	if(fgets(buffer, 512 , stdin) == NULL) {
		printf("\n");
		break;	
	}

	char* commands[10];
	int counter = 0;
	int i = 0;
        //seperates to commands into their own strings in commands[]
        char* cmd = strtok(buffer, "|");
      	//printf("%s", cmd);
	if(buffer[0] != '\n') { 
	//seperate pipes
        while(cmd != NULL) {
	    while(isspace( (unsigned char) cmd[0])) {
                cmd++;
            }
	    int endOfCommand = strlen(cmd) - 1;
            while(endOfCommand > 0 && isspace( (unsigned char) cmd[endOfCommand])) {
                endOfCommand--;
            }
            cmd[endOfCommand + 1] = '\0';
	    //if(counter == 0 && strchr(cmd, "<") != NULL)  lt = 1;
	    //if(counter == 1 && strchr(cmd, ">") != NULL)  gt = 1; 
            commands[counter] = cmd;
	    //execStr[counter] = strtok(commands[counter], " ");
           counter++;
            cmd = strtok(NULL, "|");    
    	}
	//create string for command
	i = 0;
	//error check	
	for(i = 0;i < counter;i++) {
		if(i != counter-1 && strchr(commands[i], '>') != NULL) {
			error = 1;
			printf("ERROR: '>' is outside of the last command\n");
		}
		if(i != 0 && strchr(commands[i], '<') != NULL) {
                        error = 1;
                        printf("ERROR: '<' is outside of the first command\n");
                }
		if(i != counter-1 && strchr(commands[i], '&') != NULL) {
			error = 1;
			printf("ERROR: '&' is not at the end of the line\n");
		}
	}
	
	i = 0;
	int j = 0;
	int last = counter;
	int doubleat1 = 0;
	char* testGt = strchr(commands[counter-1], '>');
	char* testLt = strchr(commands[0], '<');
	//check for file redirects
	if(strchr(commands[counter-1], '&') != NULL) {
		amp = 1;
		commands[counter - 1] = strtok(commands[counter - 1], "&");
	}
	if(testGt != NULL && testLt != NULL && counter == 1) {
		//add here
		gt = 1;
		lt = 1;
		if(&testGt[0] > &testLt[0]) {
			//less than happens first
			//ex:  cat < x > y
			testGt++;
			while(isspace( (unsigned char) testGt[0])) testGt++;
			outfile = testGt;
			//printf("outfile is %s", outfile);
			testLt++;
			while(isspace( (unsigned char) testLt[0])) testLt++;
			infile = strtok(testLt, ">");
			int endOfCommand = strlen(infile) - 1;
            		while(endOfCommand > 0 && isspace( (unsigned char) infile[endOfCommand])) {
                		endOfCommand--;
            		}
		
			infile[endOfCommand + 1] = '\0';
			//printf("infile is %s", infile);
			commands[0] = strtok(commands[0], "<");
		}
		else {
			//greater than happens first
			//ex:  cat > x < y
			testLt++;
			while(isspace( (unsigned char) testLt[0])) testLt++;
			infile = testLt;
			testGt++;
			while(isspace( (unsigned char) testLt[0])) testLt++;
			outfile = strtok(testGt, "<");
			int endOfCommand = strlen(outfile) - 1;
			while(endOfCommand > 0 && isspace( (unsigned char) outfile[endOfCommand])) {
                                endOfCommand--;
                        }
                        outfile[endOfCommand + 1] = '\0';
			commands[0] = strtok(commands[0], ">");
		}
		doubleat1 = 1;
		//commands[0] = strtok(commands[0], "<");
		//commands[counter - 1] = strtok(commands[counter-1], 
	}
	//printf("counter is %i" , counter);
	if(testGt != NULL && doubleat1 == 0) {
		gt = 1;
		testGt++;
		last--; //no longer need next loop to go to the last one
		while(isspace( (unsigned char) testGt[0])) {
                	testGt++;
                }
		
		outfile = testGt;
		//printf("\n/%s/\n", outfile);
		commands[counter-1] = strtok(commands[counter-1], ">");
	//	printf("command0 %s", commands[0]);
	}
	if(testLt != NULL && doubleat1 == 0) {
                lt = 1;
                testLt++;
                j++; //no longer need next loop to go to the last one
                while(isspace( (unsigned char) testLt[0])) {
                        testLt++;
                }
                infile = testLt;
		commands[0] = strtok(commands[0], "<");
         //       printf("\n/%s/\n", infile);
        }
       // printf("about to seperate args");  
	//Seperate commands from args
	//printf("las cmd %s", commands[counter-1]);
	for(j = 0;j < counter; j++) {		
		char* word = strtok(commands[j], " ");
		//if((word = strtok(word, "&")) != NULL) delay = 1;
		execStr[j] = word;
		i = 0;
        	while(word != NULL) {
	     		while(isspace( (unsigned char) word[0])) {
                		word++;
            		}
			//remove leading a trailing whitespaces
        		int endOfCommand = strlen(word) - 1;
 	       		while(endOfCommand > 0 && (isspace( (unsigned char) word[endOfCommand]) || word[endOfCommand] == '\n' )) {
             		endOfCommand--;
        		}
        		word[endOfCommand + 1] = '\0';
			/*
			if(strcmp(word, "<") == 0) {
				infile = strtok(NULL, " ");
				lt = 2;
				printf("%s\n", infile);
			} 
			else if(strcmp(word, ">") == 0) { 
				//outfile = strtok(NULL, " "); 
				gt = 2;
				//printf("%s\n", outfile);
			}
			else {*/
				execArgs[j][i] = word;
				i++;
			
            		word = strtok(NULL, " ");
        	}
		if(execArgs[j][i-1] == "\n") execArgs[j][i-1] = NULL;
        	else execArgs[j][i] = NULL;
	}
	//remove ending newline and end string with NULL
	//if(execArgs[i-1] == "\n") execArgs[i-1] = NULL;
	//else execArgs[i] = NULL;	 
	
	j = 0;
 	
	//check for input/output redirection
	
	//need to do b4 this point
	//seperate each command into commands[] DONE
	//put each arg into an array DONE
	
	//need to do after this point
	//loop the pipe for multiple pipe inputs
	//add ampersand (should be the same as no pipe but without waitpid)
	//add < and >
	//printf("about to fork");
	fflush(stdout);
	int prevPipeWrite = STD_INPUT;
	//with no pipe
	//if(counter == 1) {
	i = 0;
	if(error == 0) {
	if((pid = fork() > 0)) {
            //parent
            if(amp == 1) waitpid(-1, &status, WNOHANG);
	    else waitpid(-1, &status, 0);
           // exit(status);
        } else {
            //child
	    if(counter != 1) {
		for(i = 0;i < counter-1;i++) {
		pipe(fd);
	
        	if((fork() == 0)) {
            		//child
			
			if(prevPipeWrite != STD_INPUT) {
				dup2(prevPipeWrite, STD_INPUT); //instead of reading from STD_INPUT, read from prevPipeWrite
				close(prevPipeWrite);
			}
			if(i == 0 && lt > 0) {
				int inRe = open(infile, O_RDONLY);
                		if(inRe == -1) {
                        		printf("ERROR: Could not open file\n");
                        		exit(1);
                		}
                		dup2(inRe, STD_INPUT);
			}
			dup2(fd[1], STD_OUTPUT); //write to fd[1] instead of STD_OUTPUT
			close(fd[1]); //dont need since duped
            		execvp(execStr[i], execArgs[i]);
            		perror("ERROR: execvp");
        	}
		else {
			waitpid(-1, &status, 0);
		}
		close(fd[1]); //no longer need to write
		close(prevPipeWrite); //no longer need to read from past fd
		prevPipeWrite = fd[0]; //saves read fd
		}
	    //after for loop / parent
  	    //dup2(prevPipeWrite, STD_INPUT); //read from current 
	    //close(prevPipeWrite);
	    if(prevPipeWrite != STD_INPUT) {
            	dup2(prevPipeWrite, STD_INPUT); //instead of reading from STD_INPUT, read from prevPipeWrite
                close(prevPipeWrite); //should also close fd[0]	
            }
	    //dup2(STD_OUTPUT, STD_OUTPUT); //set output to stdout
	    
	    //close(fd[0]);
	    }
	    //will always print last command, so redirect if contains >
	    //note if counter = 1, will also be first count
	    if(gt > 0) {
		int outRe = open(outfile, O_WRONLY | O_CREAT, 0666); //open file to write to
		if(outRe == -1) {
                        printf("ERROR: Could not open file %s\n", outfile);
			exit(1);
			//outRe =  open(outfile, O_CREAT);
                }
		dup2(outRe, STD_OUTPUT);
		close(outRe);
	    }

	    if(lt > 0 && counter == 1) {
		int inRe = open(infile, O_RDONLY);
		if(inRe == -1) {
			printf("ERROR: Could not open file %s\n", infile);
			exit(1);
			//inRe = create(infile, O_RDONLY);
		}
	    }
	    execvp(execStr[i], execArgs[i]);
	    perror("ERROR: execvp");
	
        }
	}
	}
    	free(buffer);
	//free(infile);
	//free(outfile);
    }
    
}
