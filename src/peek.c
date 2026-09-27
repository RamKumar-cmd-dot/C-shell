#include "../include/main.h"

#define PEEK_CHUNK_SIZE 4096

int open_file(char* filename){
    struct stat st;
    if(stat(filename, &st)!=0) return 0;
    if(S_ISDIR(st.st_mode)) return -1;
    return 1;
}

void print_stream(){
    char line[1024];
    while(fgets(line, sizeof(line), stdin)!=NULL) printf("%s", line);
    return ;
}

int number_stdin(){
    char line[1024];
    int line_number = 1;
    while(fgets(line, sizeof(line), stdin)!=NULL){
        if(line[0]=='\n' || line[0]=='\r'){
            printf("%s", line);
            continue;
        }
        printf("%d %s", line_number, line);
        line_number++;
    }
    return 1;
}

void reverse_stdin_helper(){
    char line[1024];
    if(fgets(line, sizeof(line), stdin)==NULL) return;
    reverse_stdin_helper();
    printf("%s", line);
}

void reverse_number_stdin_helper(int *line_number){
    char line[1024];
    if(fgets(line, sizeof(line), stdin)==NULL) return;
    int current_number = *line_number;
    if(line[0]!='\n' && line[0]!='\r') (*line_number)++;
    reverse_number_stdin_helper(line_number);
    if(line[0]=='\n' || line[0]=='\r') printf("%s", line);
    else printf("%d %s", current_number, line);
}

int reverse_number_stdin(){
    int line_number = 1;
    reverse_number_stdin_helper(&line_number);
    return 1;
}

int reverse_stdin(){
    reverse_stdin_helper();
    return 1;
}

int print_file(char* filename){
    FILE* file = fopen(filename, "r");
    if(file==NULL) return 0;
    int ch;
    while((ch=fgetc(file))!=EOF) putchar(ch);
    fclose(file);
    return 1;
}

void reverse_helper(FILE* file){
    char line[1024];
    if(fgets(line, sizeof(line), file)==NULL) return;
    reverse_helper(file);
    printf("%s", line);
}

int count_nonempty_lines(int fd){
    if(lseek(fd, 0, SEEK_SET) == -1) return -1;
    char chunk[PEEK_CHUNK_SIZE];
    ssize_t n;
    int count = 0;
    int at_line_start = 1;
    while((n = read(fd, chunk, PEEK_CHUNK_SIZE)) > 0){
        for(ssize_t i = 0; i < n; i++){
            if(at_line_start){
                if(chunk[i] != '\n' && chunk[i] != '\r') count++;
                at_line_start = 0;
            }
            if(chunk[i] == '\n') at_line_start = 1;
        }
    }
    if(n < 0) return -1;
    return count;
}

void print_segment(const char *data, long len, int numbering, int *current_number){
    int is_blank = (len > 0) && (data[0] == '\n' || data[0] == '\r');
    if(numbering && !is_blank){
        printf("%d ", *current_number);
        (*current_number)--;
    }
    if(len > 0) fwrite(data, 1, (size_t)len, stdout);
}

int reverse_seek_output(int fd, int numbering){
    long file_size = lseek(fd, 0, SEEK_END);
    if(file_size < 0) return 0;
    if(file_size == 0) return 1;
    int current_number = 0;
    if(numbering){
        int total = count_nonempty_lines(fd);
        if(total<0) return 0;
        current_number = total;
    }
    char chunk[PEEK_CHUNK_SIZE];
    char *buf = NULL;
    long buf_len = 0;
    long pos = file_size;
    while(pos>0 || buf_len>0){
        if(pos>0){
            long read_size = (pos>=PEEK_CHUNK_SIZE)?PEEK_CHUNK_SIZE:pos;
            pos -= read_size;
            if(lseek(fd, pos, SEEK_SET)==-1){
                free(buf);
                return 0;
            }
            ssize_t got = read(fd, chunk, (size_t)read_size);
            if(got!=read_size){
                free(buf);
                return 0;
            }
            char *new_buf = malloc(read_size + buf_len);
            if(new_buf == NULL){
                free(buf);
                return 0;
            }
            memcpy(new_buf, chunk, read_size);
            if(buf_len>0) memcpy(new_buf + read_size, buf, buf_len);
            free(buf);
            buf = new_buf;
            buf_len += read_size;
        }
        while(buf_len>0){
            long j;
            int found = 0;
            long search_from = (buf[buf_len - 1]=='\n')?buf_len-2:buf_len-1;
            for(j=search_from;j>=0;j--){
                if(buf[j] == '\n'){
                    found = 1;
                    break;
                }
            }
            if(found){
                long seg_start = j + 1;
                long seg_len = buf_len - seg_start;
                print_segment(buf+seg_start, seg_len, numbering, &current_number);
                buf_len = j + 1;
            } 
            else if(pos==0){
                print_segment(buf, buf_len, numbering, &current_number);
                buf_len = 0;
            } 
            else break;
        }
    }
    free(buf);
    return 1;
}

int print_reverse(char* filename){
    FILE* file = fopen(filename, "r");
    if(file==NULL) return 0;
    int fd = fileno(file);
    if(lseek(fd, 0, SEEK_CUR)==-1){
        reverse_helper(file);
        fclose(file);
        return 1;
    }
    int result = reverse_seek_output(fd, 0);
    fclose(file);
    return result;
}

void reverse_numbering_helper(FILE* file, int* line_number){
    char line[1024];
    if(fgets(line, sizeof(line), file)==NULL) return;
    int current = *line_number;
    if(line[0]!='\n' && line[0]!='\r') (*line_number)++;
    reverse_numbering_helper(file, line_number);
    if(line[0]=='\n' || line[0]=='\r') printf("%s", line);
    else printf("%d %s", current, line);
}

int reverse_numbering(char* filename){
    FILE* file = fopen(filename, "r");
    if(file==NULL) return 0;
    int fd = fileno(file);
    if(lseek(fd, 0, SEEK_CUR) == -1){
        int line_number = 1;
        reverse_numbering_helper(file, &line_number);
        fclose(file);
        return 1;
    }
    int result = reverse_seek_output(fd, 1);
    fclose(file);
    return result;
}

int number_line(char* filename){
    FILE* file = fopen(filename, "r");
    if(file==NULL) return 0;
    int line_number = 1;
    char line[1024];
    while(fgets(line, sizeof(line), file)!=NULL){
        if(line[0]=='\n' || line[0]=='\r'){
            printf("%s", line);
            continue;
        }
        printf("%d %s", line_number, line);
        line_number++;
    }
    fclose(file);
    return 1;
}

int peek(char** args, int count){
    char* filenames[count + 1];
    int file_count = 0;
    int numbering = 0;
    int reverse = 0;
    for(int i=0;i<count;i++){
        if(args[i][0]=='-' && args[i][1]!='\0'){
            for(int j=1;args[i][j]!='\0';j++){
                if(args[i][j]=='n') numbering = 1;
                else if(args[i][j]=='r') reverse = 1;
                else return 0;
            }
        }
        else{
            filenames[file_count] = args[i];
            file_count++;
        }
    }
    if(file_count==0){
        filenames[0] = "-";
        file_count = 1;
    }
    int overall_status = 1;

    for(int i=0;i<file_count;i++){
        if(strcmp(filenames[i], "-")==0){
        if(reverse && numbering) reverse_number_stdin();
        else if(reverse) reverse_stdin();
        else if(numbering) number_stdin();
        else print_stream();
        continue;
        }
        int status = open_file(filenames[i]);
        if(status==0){
            printf("peek: no such file or directory\n");
            overall_status = 0;
            continue;
        }
        else if(status==-1){
            printf("peek: is a directory\n");
            overall_status = 0;
            continue;
        }
        int result;
        if(reverse && numbering) result = reverse_numbering(filenames[i]);
        else if(reverse) result = print_reverse(filenames[i]);
        else if(numbering) result = number_line(filenames[i]);
        else result = print_file(filenames[i]);
        if(!result) overall_status = 0;
    }
    return overall_status;
}