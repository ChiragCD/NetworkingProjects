#include "msg.h"

static int mqid;

void siginthandler(int status) {
    sigset_t set;
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, NULL);
    msgctl(mqid, IPC_RMID, NULL);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
    return;
}

void client() {
    
    char cwd[200];
    getcwd(cwd, sizeof(cwd));
    key_t key = ftok(cwd, 42);
    mqid = msgget(key, S_IWUSR | S_IRUSR);
    msg send_buf,recv_buf;
    ssize_t temp, msgsize;
    for(;;){
            printf("\nEnter the type of operation you would like to perform:\nADD FILE: 1 <file path>\nADD CHUNK: 2 <file path> <machine file path> <chunk number>\nCOPY: 3 <source> <destination>\nMOVE: 4 <source> <destination>\nREMOVE: 5 <file path>\nCOMMAND: 6 <command> <chunk name>\n");
            int choice = -1;
            choice = getc(stdin);
            choice -= '0';
            char cmd[100],s[100],d[100],fp[100];
            int chunk_num;
            switch(choice){
                case 1:scanf("%s",s); // take in input the desired location in M.
                getchar();
                       send_buf.mtype=1;
                       send_buf.mbody.req = ADD_FILE;
                       send_buf.mbody.sender = getpid();
                       strcpy(send_buf.mbody.paths[0],s);
                       temp = msgsnd(mqid,&send_buf,MSGSIZE,0);
                       msgsize = msgrcv(mqid, &recv_buf, MSGSIZE, getpid(), 0);
                       if(recv_buf.mbody.status==-1)
                               printf("%s\n",recv_buf.mbody.error);
                       else 
                               printf("FILE ADDED SUCCESSFULLY\n");
                       break;
                case 2:scanf("%s",s); 
                       scanf("%s",fp); // get the local file path inside client file-directory
                       scanf("%d",&chunk_num);
                getchar();
                       send_buf.mtype=1;
                       send_buf.mbody.req = ADD_CHUNK;
                       send_buf.mbody.sender = getpid();
                       strcpy(send_buf.mbody.paths[0],s);
                       int fd;
                       if((fd = open(fp,O_RDONLY)) == -1){
                               printf("Could not find file with name %s.\n",fp);
                               break;
                       }
                       chunk c;
                       lseek(fd,MSGSIZE/2*(chunk_num-1),SEEK_SET);
                       int num_read;
                       if((num_read = read(fd,c.data,MSGSIZE/2)) == 0 )
                       {printf("Chunk number too large, file is not that big\n");close(fd);break;}
                       close(fd);
                       c.data[num_read] = '\0';

                       temp = msgsnd(mqid,&send_buf,MSGSIZE,0);
                       msgsize = msgrcv(mqid, &recv_buf, MSGSIZE, getpid(), 0);
                       
                       if(recv_buf.mbody.status==-1)
                       {printf("%d %s\n",recv_buf.mbody.status, recv_buf.mbody.error);close(fd);break;}\
                       for(int i=0;i<3;i++){
                               pid_t d_pid = recv_buf.mbody.addresses[i];
                               send_buf.mtype=d_pid;
                               send_buf.mbody.chunk=c;
                               send_buf.mbody.chunk.chunk_id = recv_buf.mbody.chunk.chunk_id;
                               send_buf.mbody.req=STORE_CHUNK;
                               printf("Sending message to server %d\n",d_pid);
                               temp = msgsnd(mqid,&send_buf,MSGSIZE,0);
                               msg tempmsg;
                               msgsize = msgrcv(mqid, &tempmsg, MSGSIZE, getpid(), 0);
                       }
                       break;
                case 3:scanf("%s",s);
                       scanf("%s",d);
                getchar();
                       send_buf.mtype=1;
                       send_buf.mbody.req = CP;
                       send_buf.mbody.sender = getpid();
                       strcpy(send_buf.mbody.paths[0],s);
                       strcpy(send_buf.mbody.paths[1],d);
                       temp = msgsnd(mqid,&send_buf,MSGSIZE,0);
                       msgsize = msgrcv(mqid, &recv_buf, MSGSIZE, getpid(), 0);
                       if(recv_buf.mbody.status==-1)
                               printf("%s\n",recv_buf.mbody.error);
                       else 
                               printf("FILE COPIED SUCCESSFULLY\n");
                       break;
                case 4:scanf("%s",s);
                       scanf("%s",d);
                getchar();
                       send_buf.mtype=1;
                       send_buf.mbody.req = MV;
                       send_buf.mbody.sender = getpid();
                       strcpy(send_buf.mbody.paths[0],s);
                       strcpy(send_buf.mbody.paths[1],d);
                       temp = msgsnd(mqid,&send_buf,MSGSIZE,0);
                       msgsize = msgrcv(mqid, &recv_buf, MSGSIZE, getpid(), 0);
                       if(recv_buf.mbody.status==-1)
                               printf("%s\n",recv_buf.mbody.error);
                       else 
                               printf("FILE MOVED SUCCESSFULLY\n");
                       break;
                case 5:scanf("%s",s); // take in input the desired location in M.
                getchar();
                       send_buf.mtype=1;
                       send_buf.mbody.req = RM;
                       send_buf.mbody.sender = getpid();
                       strcpy(send_buf.mbody.paths[0],s);
                       temp = msgsnd(mqid,&send_buf,MSGSIZE,0);
                       msgsize = msgrcv(mqid, &recv_buf, MSGSIZE, getpid(), 0);
                       if(recv_buf.mbody.status==-1)
                               printf("%s\n",recv_buf.mbody.error);
                       else 
                               printf("FILE REMOVED SUCCESSFULLY\n");
                       break;
                case 6:scanf("%s",cmd);
                       scanf("%s",s);
                getchar();
                       send_buf.mtype=1;
                       send_buf.mbody.req = COMMAND;
                       send_buf.mbody.sender = getpid();
                       strcpy(send_buf.mbody.paths[0],s);
                       temp = msgsnd(mqid,&send_buf,MSGSIZE,0);
                       msgsize = msgrcv(mqid, &recv_buf, MSGSIZE, getpid(), 0);
                       if(recv_buf.mbody.status==-1)
                               printf("%s\n",recv_buf.mbody.error);
                       else 
                               printf("FILE ADDED SUCCESSFULLY\n");
                       break;
                case -49:
                printf("njwefk\n");
                    int tempkroqnnw = getc(stdin);
                    printf("%d\n", tempkroqnnw);
                break;

            }
    }
}

int main(int argc, char ** argv) {
    client();
    return 0;
}
