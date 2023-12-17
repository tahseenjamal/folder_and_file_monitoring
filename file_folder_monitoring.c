#include<stdio.h>
#include<string.h>
#include<time.h>
#include<sys/inotify.h>
#include<unistd.h>
#include<stdlib.h>
#include<signal.h>
#include<fcntl.h> 

#define MAX_EVENTS 1024  /* Maximum number of events to process */
#define LEN_NAME 512  /* Assuming filename would not be longer than exceed 512 bytes */
#define EVENT_SIZE  ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) 


int fd,wd;

FILE *fptr;

void walker(char* name) {

  DIR *d;

  struct dirent *dir;

  d = opendir(name);

  if (d) {

    while ((dir = readdir(d)) != NULL) {

        if(dir->d_type == 4 && strcmp(dir->d_name,".") != 0 && strcmp(dir->d_name, "..") != 0) {

            printf("%s/%s\n", name, dir->d_name);

            walker(dir->d_name);
        }
    }

    closedir(d);

  }
}


void sig_handler(int sig){

    /* Step 5. Remove the watch descriptor and close the inotify instance*/
    inotify_rm_watch( fd, wd );

    close( fd );

    exit( 0 );

}


int main(int argc, char **argv){

    fptr = fopen("watcher.log","a");

    char *path_to_be_watched;

    signal(SIGINT,sig_handler);


    /* Step 1. Initialize inotify */
    fd = inotify_init();


    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)  // error checking for fcntl
        exit(2);

    /* Step 2. Add Watch */
    for (int i =0; i < argc-1; i++)
    {       

        path_to_be_watched = argv[i+1];

        wd = inotify_add_watch(fd,path_to_be_watched,IN_MODIFY | IN_CREATE | IN_DELETE);

        if(wd==-1){

            printf("Could not watch : %s\n",path_to_be_watched);

        }
        else{

            printf("Watching : %s\n",path_to_be_watched);

        }

    }


    while(1){

        int i=0,length;

        char buffer[BUF_LEN];

        /* Step 3. Read buffer*/
        length = read(fd,buffer,BUF_LEN);

        /* Step 4. Process the events which has occurred */
        while(i<length){

            struct inotify_event *event = (struct inotify_event *) &buffer[i];

            time_t now;

            time(&now);

            char *current_time = ctime(&now);

            if (current_time[strlen(current_time)-1] == '\n') 
            {

                current_time[strlen(current_time)-1] = '\0';
            }

            if(event->len && strcmp(event->name, "watcher.log") != 0){ 

                if ( event->mask & IN_CREATE ) {

                    if ( event->mask & IN_ISDIR ) {

                        fprintf( fptr, "%s,FOLDER,%s,CREATED\n", current_time, event->name);

                    } else {

                        fprintf(fptr, "%s,FILE,%s,CREATED\n", current_time, event->name);
                    }
                }
                else if ( event->mask & IN_DELETE ) {

                    if ( event->mask & IN_ISDIR ) {

                        fprintf(fptr, "%s,FOLDER,%s,DELETED\n", current_time, event->name);

                    }
                    else {

                        fprintf(fptr, "%s,FILE,%s,DELETED\n", current_time, event->name);
                    }
                }
                else if ( event->mask & IN_MODIFY ) {

                    if ( event->mask & IN_ISDIR ) {

                        fprintf(fptr, "%s,FOLDER,%s,MODIFIED\n", current_time, event->name);

                    }
                    else {

                        fprintf(fptr, "%s,FILE,%s,MODIFIED\n", current_time, event->name);

                    }
                }
                fflush(fptr);
            }
            i += EVENT_SIZE + event->len;
        }
    }

    fclose(fptr);
}
