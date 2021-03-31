#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h> 
#include <semaphore.h>
#include <fcntl.h>     

#define NUM_OF_PROCESSES 5
#define NUM_OF_RESOURCES 3
#define WRITE_COUNT 5

void createFiles(){
    /* ********************************************************************** */
    /* Function that will create files if does not exist, else truncate to 0 */
    /* ********************************************************************** */
    FILE *temp;
    int initial_cookie_count = 0;

    // Ensure files are recreated from scratch
    for (int i = 0; i < NUM_OF_RESOURCES; i++){
        char filename[10];
        // Get name of file to create
        snprintf(filename, 10, "jar%d.txt", i+1);
        // Open file for writing (if exist, overwrite, if does not exist, create and write)
        temp = fopen(filename, "w+");
        // Write the cookie count (default initial value == 0) into the file
        fprintf(temp, "%d", initial_cookie_count);
        // Close the file stream
        fclose(temp);
    }
}

void openFileAndModify(int jar_count){
    /* ********************************************************* */
    /* Function that increment file contents 'WRITE_COUNT' times */
    /* ********************************************************* */

    // Declare temp file stream variable
    FILE *jar;
    
    // Init var to store number of cookies in each file
    int cookie_count = 0;
    
    // Get name of file to open on this iteration
    char file_to_open[10];
    snprintf(file_to_open, 10, "jar%d.txt", jar_count);

    // Then, write n times, where n = WRITE_COUNT constant
    for (int i = 0; i < WRITE_COUNT; i++){
        // First open the file for reading
        jar = fopen(file_to_open, "r");
        // Then get the current value of cookies in the file
        fscanf(jar, "%d", &cookie_count);
        // After reading value, close the file stream
        fclose(jar);
        // Then, open the file for writing
        jar = fopen(file_to_open, "w+");
        // Write incremented value of current cookie count into file
        cookie_count += 1;
        fprintf(jar, "%d", cookie_count);
        // After writing the new value, close the file stream
        fclose(jar);
    }
}
void* race_c2(){
    /* ************************************************************************* */
    /* Worker thread function that calls 'openFileAndModify()' for all resources */
    /* ************************************************************************* */
    pid_t x = syscall(SYS_gettid);
    // Process will access each file. 
    for (int i = 0; i < NUM_OF_RESOURCES; i++){
        // Simulate acquire lock for this jar 1
        printf("\n[+] Bear PID: [%d] acquired lock for jar %d", x, i+1);
        /********************/
        /* Critical Section */
        /********************/
        openFileAndModify(i+1);
        // Simulate elease the lock for this jar (1 since we have written to it WRITE_COUNT times
        printf("\n[-] Bear PID: [%d] released lock for jar %d", x, i+1);
    }
}

int main()
{
    /* *********************************************************** */
    int jarcount;           // store jar count    
    int bear_id;            // store bear id        
    sem_t *sem;             // store shared semaphore
    pid_t bear;             // Process to be forked
    /* *********************************************************** */

    // Create shared semaphore lock for process. 
    sem = sem_open ("jarlock", O_CREAT | O_EXCL, 0644, 1); 
    printf("\nJarlock Semaphore Initialized.\n\n");

    // Ensure NUM_OF_RESOURCES * jar files are created
    createFiles();
    
    // Create NUM_OF_PROCESSES processes
    for(bear_id = 0; bear_id < NUM_OF_PROCESSES; bear_id++){
        bear = fork();
        // Check if error occured in fork process
        if(bear < 0){
            sem_unlink("jarlock");     
            sem_close(sem);           
            printf("\nFork error.");
        }
        else if(bear == 0){
            // If process is child, do not fork.
            break;
        }
    }
    // Parent Bear Process 
    if(bear != 0){
        //wait for children to exit 
        while(bear = waitpid(-1, NULL, 0)){
            if(errno == ECHILD){
                break;
            }
        }
        printf("\nParent Bear: All child Bear have exited.");
        sem_unlink("jarlock");
        sem_close(sem);
    }
    // Child Bear Process
    else{
        sem_wait(sem);
        printf ("\nBear[%d] is in critical section.", bear_id+1);
        race_c2();
        sem_post(sem);  
    }
    return 0;
}