#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 80
#define HISTORY_SIZE 10

struct HistoryQueue{
    char* history_lines[HISTORY_SIZE];
    size_t size;
    size_t start;
    size_t end;
    size_t offset;
} history;

size_t get_ordered_lines(char* ordered_lines[]){
    size_t i = history.start;
    for(size_t num_iters = 0; num_iters < history.size; num_iters++){
        ordered_lines[num_iters] = history.history_lines[i];
        i = (i + 1) % HISTORY_SIZE;
    }
    return history.size;
}

void add_line(char* line){
    if(history.size == HISTORY_SIZE){
        free(history.history_lines[history.start]);
        history.start = (history.start + 1) % HISTORY_SIZE;
        history.offset ++;
    }
    size_t line_size = strlen(line);
    history.history_lines[history.end] = malloc((line_size + 1) * sizeof(char));
    strncpy(history.history_lines[history.end], line, line_size);
    history.end = (history.end + 1) % HISTORY_SIZE;
    if(history.size < HISTORY_SIZE)
        history.size ++;
}

char* get_nth_history(int n){
    return history.history_lines[(history.start + n) % HISTORY_SIZE]; 
}

size_t split_by_space(char* line, size_t line_size, char* tokens[]){
    size_t token_idx = 0;
    size_t token_length = 0;
    char* last_token_start = NULL;
    for(size_t line_idx = 0; line_idx < line_size; line_idx ++){
        if(line[line_idx] != ' '){
            if(last_token_start == NULL){
                last_token_start = line + line_idx;
            }
            token_length ++;
        }
        else{
            if(last_token_start != NULL){
                tokens[token_idx] = malloc((token_length)* sizeof(char));
                strncpy(tokens[token_idx], last_token_start, token_length);
                tokens[token_idx][token_length] = '\0';
                token_idx ++;
                token_length = 0;
                last_token_start = NULL;
            }
        }
    }
    if(last_token_start != NULL){
        tokens[token_idx] = (char*) malloc((token_length) * sizeof(char));
        strncpy(tokens[token_idx], last_token_start, token_length);
        tokens[token_idx][token_length] = '\0';
        token_idx ++;
        token_length = 0;
        last_token_start = NULL;
    }
    tokens[token_idx] = NULL;
    return token_idx;
}
int execute_command(char* args[], size_t num_args){
    if(strcmp(args[0], "exit") == 0){
        return 0;
    }
    else if(strcmp(args[0], "history") == 0){
        char* ordered_history_lines[HISTORY_SIZE];
        size_t num_lines = get_ordered_lines(ordered_history_lines);
        for (size_t i = 0; i < num_lines; i ++){
            printf("%d %s", i + 1 + history.offset, ordered_history_lines[i]);
        }
        fflush(stdout);
        return -1;
    }
    else if(args[0][0] == '!'){
        size_t command_length = strlen(args[0]);
        if(command_length != 2 || command_length != 3){
            if(args[0][1] == '!'){
                if(command_length == 2){
                    if(history.size == 0){
                        return 3;
                    }
                    char* parsed_line[MAX_LINE / 2 + 1];
                    char* last_command = get_nth_history(0);
                    size_t last_command_length = strlen(last_command) - 1;
                    size_t last_num_args = split_by_space(last_command, last_command_length, parsed_line);
                    execute_command(parsed_line, last_num_args);
                    return -1;
                }
            }
            else{
                int command_number = atoi(args[0] + 1);
                if(command_number == 0){
                    return 2;
                }
                else{
                    int command_idx = command_number - history.offset - 1;
                    if(command_idx < history.size){
                        char* parsed_line[MAX_LINE / 2 + 1];
                        char* last_command = get_nth_history(command_idx);
                        size_t last_command_length = strlen(last_command) - 1;
                        size_t last_num_args = split_by_space(last_command, last_command_length, parsed_line);
                        execute_command(parsed_line, last_num_args);
                        return -1;
                    }
                    else{
                        return 4;
                    }
                }
            }
        }
        return 2;
    }
    else{
        int should_wait = strcmp(args[num_args - 1], "&") != 0;
        if(!should_wait){
            free(args[num_args - 1]);
            args[num_args - 1] = NULL;
        }
        pid_t pid = fork();
        if(pid == 0){
            if(execvp(args[0], args) == -1){
                return 2;
            }
            exit(0);
        }
        else{
            if(should_wait){
                wait(NULL);
            }
        }
        return 1;
    }
}
void free_args(char* args[], size_t num_args){
    for (size_t i = 0; i < num_args; i ++){
        free(args[i]);
        args[i] = NULL;
    }
}

int main(){
    char *args[MAX_LINE/2 + 1];
    int should_run = 1;
    while(should_run){
        printf("osh>");
        fflush(stdout);
        char* line;
        size_t buffer_size;
        size_t line_size = getline(&line, &buffer_size, stdin) - 1; // -1 to exclude the \include
        if(line_size){
            size_t num_args = split_by_space(line, line_size, args);
            int command_type = execute_command(args, num_args);
            if(command_type == 0){
                should_run = 0;
            }
            else if(command_type == 1){
                add_line(line);
            }
            else if(command_type == 2){
                puts("Invalid Command!");
            }
            else if(command_type == 3){
                puts("No commands in history.");
            }
            else if(command_type == 4){
                puts("No such command in history.");
            }
            free_args(args, num_args);
        }
    }
    
    return 0;
}