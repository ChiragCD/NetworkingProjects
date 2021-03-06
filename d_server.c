#include "msg.h"

static int CHUNK_SIZE;
static int mqid;
static char dir_name[10];

void get_file_name(int chunk_id, char * buffer) {
    strcpy(buffer, dir_name);
    buffer += strlen(buffer);
    strcpy(buffer, "/chunk");
    sprintf(buffer+6, "%d", chunk_id);
    buffer += strlen(buffer);
    strcpy(buffer, ".txt");
}

int store_chunk (msg message);
int copy_chunk (msg message);
int remove_chunk (msg message);
int command (msg message);
int status_update (msg message);
int ls_data (msg message);

void d_server() {

    pid_t pid = getpid();
    printf("D server at %d\n", pid);
    
    char cwd[200];
    getcwd(cwd, sizeof(cwd));
    key_t key = ftok(cwd, 42);
    mqid = msgget(key, S_IWUSR | S_IRUSR);
    if(mqid == -1) {
        perror("Msq not obtained");
        return;
    }

    msg recv_buf;
    msg send_buf;

    send_buf.mbody.sender = getpid();

    send_buf.mtype = 1;
    send_buf.mbody.req = NOTIFY_EXISTENCE;
    msgsnd(mqid, &send_buf, MSGSIZE, 0);

    dir_name[0] = 'D';
    sprintf(dir_name, "%d", pid);
    mkdir(dir_name, 0777);

    for(;;) {
        ssize_t msgsize = msgrcv(mqid, &recv_buf, MSGSIZE, pid, 0);
        if(msgsize == -1) {
            perror("Recv ");
            raise(SIGINT);
            exit(1);
        }
        int status = 0;
        if(recv_buf.mbody.req == STORE_CHUNK) status = store_chunk(recv_buf);
        if(recv_buf.mbody.req == COPY_CHUNK) status = copy_chunk(recv_buf);
        if(recv_buf.mbody.req == REMOVE_CHUNK) status = remove_chunk(recv_buf);
        if(recv_buf.mbody.req == COMMAND) status = command(recv_buf);
        if(recv_buf.mbody.req == STATUS_UPDATE) status = status_update(recv_buf);
        if(recv_buf.mbody.req == LS_DATA) status = ls_data(recv_buf);

    }
}

int main(int argc, char ** argv) {
    if(argc < 2) {
        printf("Usage - ./exec <CHUNK_SIZE>\nCHUNK_SIZE must be less than %d (bytes)\n", MSGSIZE/2);
        return -1;
    }
    CHUNK_SIZE = atoi(argv[1]);
    if(CHUNK_SIZE > 512) {
        printf("Usage - ./exec <CHUNK_SIZE>\nCHUNK_SIZE must be less than %d (bytes)\n", MSGSIZE/2);
        return -1;
    }
    d_server();
    return 0;
}

int store_chunk (msg message) {

    printf("\nStarting store chunk\n");

    int chunk_id = message.mbody.chunk.chunk_id;
    msg send;
    send.mtype = message.mbody.sender;
    send.mbody.sender = getpid();
    send.mbody.req = STATUS_UPDATE;

    char buffer[100];
    get_file_name(chunk_id, buffer);
    int fd = open(buffer, O_CREAT|O_EXCL|O_RDWR, 0777);
    if(fd == -1) {
        printf("Store chunk error - Chunk %d already present\n", chunk_id);
        send.mbody.status = -1;
        strcpy(send.mbody.error, "Store error - Chunk already present\n");
        msgsnd(mqid, &send, MSGSIZE, 0);
        return -1;
    }

    write(fd, message.mbody.chunk.data, CHUNK_SIZE);
    close(fd);

    printf("Stored chunk %d\n", chunk_id);
    strcpy(send.mbody.error, "Store Success");
    send.mbody.status = 0;
    msgsnd(mqid, &send, MSGSIZE, 0);
    return 0;
}

int copy_chunk (msg message) {

    int chunk_id = message.mbody.chunk.chunk_id;
    int new_chunk_id = message.mbody.status;
    pid_t new_server = message.mbody.addresses[0];
    printf("\nStarting copy chunk %d to %d\n", chunk_id, new_chunk_id);

    msg update;
    update.mtype = 1;
    update.mbody.sender = getpid();
    update.mbody.req = STATUS_UPDATE;

    char buffer[100];
    get_file_name(chunk_id, buffer);
    int fd = open(buffer, O_RDONLY, 0777);
    if(fd == -1) {
        printf("Not found\n");
        update.mbody.status = -1;
        strcpy(update.mbody.error, "Copy error - Not found");
        msgsnd(mqid, &update, MSGSIZE, 0);
        return -1;
    }

    msg send;
    send.mtype = new_server;
    send.mbody.sender = getpid();
    send.mbody.req = STORE_CHUNK;
    send.mbody.chunk.chunk_id = new_chunk_id;
    int num_read;
    num_read = read(fd,send.mbody.chunk.data,CHUNK_SIZE);
    close(fd);
    send.mbody.chunk.data[num_read] = '\0';
    msgsnd(mqid, &send, MSGSIZE, 0);
    printf("Copy complete\n");
    update.mbody.status = 0;
    strcpy(update.mbody.error, "Copy chunk success");
    msgsnd(mqid, &update, MSGSIZE, 0);
    return 0;
}

int remove_chunk (msg message) {

    printf("\nStarting remove chunk\n");

    int chunk_id = message.mbody.chunk.chunk_id;
    char buffer[100];
    get_file_name(chunk_id, buffer);
    int fd = open(buffer, O_RDONLY, 0777);
    if(fd == -1) {
        printf("Chunk not present\n");
        return -1;
    }
    close(fd);
    remove(buffer);
    printf("Successfully removed %d\n", chunk_id);
    return 0;
}

int ls_data (msg message) {
    msg send_buf;
    send_buf.mtype = message.mbody.sender;
    send_buf.mbody.sender = getpid();
    int arr[2];
    pipe(arr);
    int pid = getpid();
    if(fork()){// parent
       close(arr[1]); // close write end
       send_buf.mbody.req = OUTPUT;
       int n = read(arr[0],send_buf.mbody.chunk.data,MSGSIZE/2);
       send_buf.mbody.chunk.data[n] = '\0';
       close(arr[0]);
       printf("read %d bytes %s\n",n,send_buf.mbody.chunk.data);
       msgsnd(mqid,&send_buf,MSGSIZE,0);
       return 0;
    }
    dup2(arr[1],1); //duplicate write end for child
    close(arr[0]);  //close the read end for child
    char dirname[100];
    char tmp[100];
    sprintf(tmp,"%d",pid);
    strcpy(dirname,tmp);
    execlp("ls","ls",dirname,NULL);
    printf("SHOULDNT BE HERE\n");
}

int status_update (msg message) {
    printf("\nReceived update from %d - %s\n", message.mbody.sender, message.mbody.error);
    return 0;
}
int command (msg message) {
    msg send_buf;
    send_buf.mtype = message.mbody.sender;
    send_buf.mbody.sender = getpid();
    char fname[100];
    strcpy(fname,dir_name);
    strcat(fname,"/chunk");
    strcat(fname,message.mbody.paths[0]);
    strcat(fname,".txt");
    printf("Attempting to open %s\n",fname);
    int fd;
    if((fd = open(fname,O_RDONLY)) == -1){
        send_buf.mbody.status=-1;
        strcpy(send_buf.mbody.error,"COULDN'T OPEN FILE INSIDE D SERVER\n");
        msgsnd(mqid,&send_buf,MSGSIZE,0);
        printf("COULDN'T OPEN FILE %s\n",fname);
        return -1;
    }
    char actual_cmd[100];
    strcpy(actual_cmd,message.mbody.chunk.data);
    char* token = strtok(actual_cmd," ");
    char cmd[100];
    strcpy(cmd,  token);
    char* args[20];
    int m=1;
    args[0]=cmd;
    while(token!=NULL){
        token=strtok(NULL," ");
        args[m++] = token;
        //printf("args[%d] is %s\n",m-1,token);
    }
    args[m-3] = NULL;// to remove the extra stuff like d pid and chunk id
    int arr[2];
//     printf("LISTING ALL ARGS TO CMD: %s\n",cmd);
//     for(int i=0;args[i];i++)
//             printf("%s\n",args[i]);
    pipe(arr);
    printf("About to execute %s on %s\n",cmd,fname);
    if(fork()){// parent
       close(arr[1]); // close write end
       send_buf.mbody.req = OUTPUT;
       int n = read(arr[0],send_buf.mbody.chunk.data,MSGSIZE/2);
       send_buf.mbody.chunk.data[n] = '\0';
       msgsnd(mqid,&send_buf,MSGSIZE,0);
    return 0;
    }
    dup2(arr[1],1); //duplicate write end for child
    close(arr[0]);  //close the read end for child
    dup2(fd,0);
    execvp(cmd,args);
}
