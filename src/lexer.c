#include "../include/main.h"

Token* lexer(char* input, int* count){
    Token* tokens = malloc(100* sizeof(Token));
    int i = 0;
    *count = 0;
    while(input[i]!='\0'){
        while(isspace(input[i])) i++;
        if(input[i]=='\0') break;
        if(input[i]=='|'){
            tokens[*count].type = OP_PIPE;
            tokens[*count].value = strdup("|");
            (*count)++, i++;
        }
        else if(input[i]=='<'){
            tokens[*count].type = OP_LT;
            tokens[*count].value = strdup("<");
            (*count)++, i++;
        }
        else if(input[i]=='>'){
            if(input[i+1]=='>'){
                tokens[*count].type = OP_GTGT;
                tokens[*count].value = strdup(">>");
                (*count)++, i+=2;
            }
            else{
                tokens[*count].type = OP_GT;
                tokens[*count].value = strdup(">");
                (*count)++, i++;
            }
        }
        else if(input[i]=='&'){
            tokens[*count].type = OP_AMP;
            tokens[*count].value = strdup("&");
            (*count)++, i++;
        }
        else if(input[i]==';'){
            tokens[*count].type = OP_SEMI;
            tokens[*count].value = strdup(";");
            (*count)++, i++;
        }
        else{
            int j = 0;
            char word[1024];
            char quote = '\0';
            while(input[i]!='\0'){
                if(input[i]=='\\'){
                    if(input[i+1]=='\0' || input[i+1]=='\n'){
                    printf("Shell : invalid syntax\n");
                    return NULL;
                    }
                    if(quote=='"'){
                        char next = input[i+1];
                        if(next=='"' || next=='\\' || next=='$' || next=='`'){
                            word[j++] = next;
                            i+=2;
                        }
                        else{
                            word[j++] = input[i];
                            word[j++] = input[i+1];
                            i+=2;
                        }
                    }
                    else if(quote=='\''){
                        word[j++] = input[i];
                        i++;
                    }
                    else{
                        word[j++] = input[i+1];
                        i+=2;
                    }
                    continue;
                }
                else if(input[i]=='\'' || input[i]=='"'){
                    if(quote=='\0'){
                        quote = input[i];
                        i++;
                        continue;
                    }
                    if(quote==input[i]){
                        quote = '\0';
                        i++;
                        continue;
                    }
                }
                if(quote=='\0' && (isspace(input[i]) || input[i]=='<' || input[i]=='>'
                || input[i]=='&' || input[i]=='|' || input[i]==';')) break;
                word[j++] = input[i++];
            }
            if(quote!='\0'){
                printf("Shell : invalid syntax\n");
                free(tokens);
                return NULL;
            }
            word[j] = '\0';
            tokens[*count].type = WORD;
            tokens[*count].value = strdup(word);
            (*count)++;
        }
    }
    return tokens;
}