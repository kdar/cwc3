#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

int verbose = 0;

void usage(argv0)
char *argv0;
{
	fprintf(stderr,"usage: %s time command\n",argv0);
	exit(1);
}

int main(argc,argv)
int argc;
char *argv[];
{
	int seconds;
	int status;
	int natural_causes;
	pid_t pid;
	pid_t wpv;
	if (argc < 3) {
		usage(argv[0]);
	}
	if (sscanf(argv[1],"%d",&seconds) != 1) {
		usage(argv[0]);
	}
	pid = fork();
	if (pid==0) {
		/* child process */
		execvp(argv[2],&argv[2]);
		exit(253);
	} else {
		/* parent process */
		int second;
		natural_causes = 0;
		for (second=0; second<seconds; second++) {
			if (verbose) {
				printf("watching for child death\n");
				fflush(stdout);
			}
			sleep(1);
			/* printf("%d\n",second); */
			wpv = waitpid(pid,&status,WNOHANG);
			if (wpv == pid) {
				/* printf("natural causes %d\n",status); */
				if (verbose) {
					printf("child died naturally %d\n",status);
					fflush(stdout);
				}
				natural_causes = 1;
				break;
			}
		}
		if (natural_causes == 0) {
			if (verbose) {
				printf("attempting to kill child - 1\n");
				fflush(stdout);
			}
			kill(pid,SIGKILL);
			wpv=waitpid(pid,&status,WNOHANG);
			if (wpv == -1) {
				perror("waitpid problem");
				exit(252);
			}
			if (wpv!=pid) {
				sleep(1);
				if (verbose) {
					printf("attempting to kill child - 2\n");
					fflush(stdout);
				}
				kill(pid,SIGKILL);
				wpv=waitpid(pid,&status,WNOHANG);
				if (wpv == -1) {
					perror("waitpid problem");
					exit(252);
				}
				if (wpv!=pid) {
					int pid2;
					/* process appears to be unkillable, even with SIGKILL -
					** at least at this time.  fork off another process to keep
					** retrying the kill, and exit to the calling process
					*/
					if (verbose) {
						printf("forking process-cidal process\n");
						fflush(stdout);
					}
					pid2 = fork();
					if (pid2==0) {
						/* child process */
						for (;;) {
							sleep(60);
							if (verbose) {
								printf("process-cidal process sending kill\n");
								fflush(stdout);
							}
							kill(pid,SIGKILL);
							wpv=waitpid(pid,&status,WNOHANG);
							if (wpv == -1) {
								perror("waitpid problem");
								exit(252);
							}
							if (waitpid(pid,&status,WNOHANG)==pid) {
								/* status passed back to... honorable ancestors */
								exit(status);
							}
						}
					} else {
						/* parent process - status passed back to shell */
						exit(254);
					}

				}
			}
		}
	}
	if (natural_causes) {
		exit(status/256);
	} else {
		exit(254);
	}
}
