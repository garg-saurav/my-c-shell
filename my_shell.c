#include  <stdio.h>
#include  <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<stdbool.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

bool sigint_called = false;

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
		tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
		strcpy(tokens[tokenNo++], token);
		tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

void reap_children(int* arr){
	for(int i=0; i<65; i++){
		if(arr[i]!=-1){
			int res = waitpid(arr[i], NULL, WNOHANG);
			if(res!=0){
				// printf("%d %d %d",i, arr[i],res);
				printf("Shell: Background process finished\n");
				arr[i]=-1;
			}
		}
	}
}

void sigintHandler(int signum){
	sigint_called = true;
	printf("\n");
}

int main(int argc, char* argv[]) {

	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	FILE* fp;
	if(argc == 2) {
		fp = fopen(argv[1],"r");
		if(fp < 0) {
			printf("File doesn't exists.");
			return -1;
		}
	}

	int bg_list[65]; // array to store backgroud processes pid
	for ( i = 0; i < 65; i++ ) {
		bg_list[i] = -1; // no backgroung process initially
	}

	signal(SIGINT, sigintHandler); // set Ctrl+C signal handler

	while(1) {			
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		if(argc == 2) { // batch mode
			if(fgets(line, sizeof(line), fp) == NULL) { // file reading finished
				break;	
			}
			line[strlen(line) - 1] = '\0';
		} else { // interactive mode
			printf("$ ");
			scanf("%[^\n]", line);
			getchar();
		}
		// printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);
   
       //do whatever you want with the commands, here we just print them

		///////////////////////////////////////////////Shell Implementation////////////////////////////////////////////////

		reap_children(bg_list);

		sigint_called = false;

		if(tokens[0]==NULL){ 
			// do nothing
		}else{

			if ( strcmp(tokens[0], "exit\0")==0){

				if(tokens[1]!=NULL){ // no arguments expected

					printf("Shell: Incorrect command\n");

				}else{

					// implement exit command

					for(int i=0; i<65; i++){ // kill background processes
					
						if (bg_list[i]!=-1){
							
							kill(bg_list[i], 15); // SIGTERM
							
							int res = waitpid(bg_list[i], NULL, 0);
							if(res!=0){
								// printf("%d %d %d",i, bg_list[i],res);
								printf("Shell: Background process finished\n");
								bg_list[i]=-1;
							}
						}
					}
       
					// Freeing the allocated memory	
					for(i=0;tokens[i]!=NULL;i++){
						free(tokens[i]);
					}
					free(tokens);

					break; // exit infinite loop

				}


			}else if ( strcmp(tokens[0], "cd\0") == 0 ) {

				if(tokens[1]==NULL || tokens[2]!=NULL){ // 0 argument to cd or more than 1 arguments
					
					printf("Shell: Incorrect command\n");

				} else {

					char* path = tokens[1];
					int ret = chdir(path);

					if (ret!=0) {

						printf("Shell: Incorrect command\n");

					}

				}

			} else {

				int num_tokens=0;
				bool back_ground=false;
				bool series = false;
				bool parallel = false;

				// find the number of tokens
				for(i=0; tokens[i]!=NULL;i++){
					if(strcmp(tokens[i],"&&\0")==0) series=true;
					if(strcmp(tokens[i],"&&&\0")==0) parallel=true;
					num_tokens++;
				}

				if(strcmp(tokens[num_tokens-1],"&\0")==0) {
					back_ground = true;
					tokens[num_tokens-1] = NULL;
					free(tokens[num_tokens]);
				}

				if(!series && !parallel){
				
					int fc = fork();

					if (fc < 0) { // fork failed
						fprintf(stderr, "%s\n", "Unable to create child process!!\n");
						// exit(1);
					}else if(fc==0){ // child code
						if(back_ground){
							setpgid(0,0);
						}
						execvp(tokens[0], tokens); // execute the command with arguments in the bash shell way
							// v --> vector arguments
						 	// p --> duplicate shell for search
						printf("Command execution failed!!\n"); // will reach here only if exec fails
						_exit(1); // prevent buffer flush by child process
					}else{

						if(!back_ground){
							int wc = waitpid(fc, NULL, 0);
						}else{
							// do not wait for background process
							for(int i=0; i<65; i++){
								if(bg_list[i]==-1){
									bg_list[i] = fc;
									break; 
								}
							}
							// int wc = waitpid(fc, NULL, WNOHANG);
						}

					}
				}

				if(series){

					char **temptokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));

					int tokenNo = 0, tempTokenNo;
					
					while(tokens[tokenNo]!=NULL){

						if(sigint_called) break;

						tempTokenNo = 0;

						while(tokens[tokenNo]!=NULL && strcmp(tokens[tokenNo],"&&\0")!=0){
							temptokens[tempTokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
							strcpy(temptokens[tempTokenNo++], tokens[tokenNo++]);
						}

						if(tokens[tokenNo]!=NULL){
							tokenNo++;
						}

						temptokens[tempTokenNo] = NULL;

						int fc = fork();

						if (fc < 0) { // fork failed
							fprintf(stderr, "%s\n", "Unable to create child process!!\n");
							// exit(1);
						}else if(fc==0){ // child code
							execvp(temptokens[0], temptokens); // execute the command with arguments in the bash shell way
								// v --> vector arguments
							 	// p --> duplicate shell for search
							printf("Command execution failed!!\n"); // will reach here only if exec fails
							_exit(1); // prevent buffer flush by child process
						}else{ // wait for child
							
							int wc = waitpid(fc, NULL, 0);

						}	

						// Freeing the allocated memory	
						for(i=0;temptokens[i]!=NULL;i++){
							free(temptokens[i]);
						}

					}

					free(temptokens);

				}

				if(parallel){

					char **temptokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));

					int process_list[65]; // array to store parallel processes pid
					for ( i = 0; i < 65; i++ ) {
						process_list[i] = -1; // no process initially
					}

					int tokenNo = 0, tempTokenNo;
					
					while(tokens[tokenNo]!=NULL){

						if(sigint_called) break;

						tempTokenNo = 0;

						while(tokens[tokenNo]!=NULL && strcmp(tokens[tokenNo],"&&&\0")!=0){
							temptokens[tempTokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
							strcpy(temptokens[tempTokenNo++], tokens[tokenNo++]);
						}

						if(tokens[tokenNo]!=NULL){
							tokenNo++;
						}

						temptokens[tempTokenNo] = NULL;

						int fc = fork();

						if (fc < 0) { // fork failed
							fprintf(stderr, "%s\n", "Unable to create child process!!\n");
							// exit(1);
						}else if(fc==0){ // child code
							execvp(temptokens[0], temptokens); // execute the command with arguments in the bash shell way
								// v --> vector arguments
							 	// p --> duplicate shell for search
							printf("Command execution failed!!\n"); // will reach here only if exec fails
							_exit(1); // prevent buffer flush by child process
						}else{ // wait for child
							
							for(int i=0; i<65; i++){
								if(process_list[i]==-1){
									process_list[i]=fc;
									break;
								}
							}
							// int wc = waitpid(fc, NULL, 0);

						}	

						// Freeing the allocated memory	
						for(i=0;temptokens[i]!=NULL;i++){
							free(temptokens[i]);
						}

					}

					for(int i=0; i<65; i++){
						if(process_list[i]!=-1){
							waitpid(process_list[i], NULL, 0);
						}
					}

					free(temptokens);

				}


			}

		}

		reap_children(bg_list);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// for(i=0;tokens[i]!=NULL;i++){
		// 	printf("found token %s (remove this debug output later)\n", tokens[i]);
		// }
       
		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);

	}
	return 0;
}
