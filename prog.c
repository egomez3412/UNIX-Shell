#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */

//Function prototypes
void userCommands(char *args[], int *amp_flag, int *argv);
void freeArguments(char *args[],int argv);

// ==== main ====================================================================
/*
    This is the main function. It has a void return type.
*/
// ==============================================================================
int main(void) {
    char *args[MAX_LINE / 2 + 1]; /* command line arguments */
    int should_run = 1; /* flag to determine when to exit program */
    pid_t pid; /* Create parent id */
    int amp_flag = 0; /* Ampersand Flag */
    int argv = 0; /* Number of arguments on >osh */
    int Pipping = 0; /* if pipping exists */
    while (should_run)
    {
        // Reset Variables
        Pipping = 0;
        /////////////////
        printf("\nosh>");
        fflush(stdout);
        /**
        * After reading user input, the steps are:
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) parent will invoke wait() unless command included &
        */
        userCommands(args, &amp_flag, &argv);
        pid = fork();
        if (pid == 0)
        {
            if (argv == 0) continue;
            else
            {
                int FILE;
                int caseRedirection = 0;

                // Handles "<", ">", and "|".
                for (int i = 1; i <= argv - 1; i++)
                {
                    // next args will be file name.
                    if (strcmp(args[i], "<") == 0)
                    {
                        // case input from file
                        FILE = open(args[i + 1], O_RDONLY);

                        if (FILE == -1 || args[i+1]  == NULL)
                        {
                            printf("Invalid Command!\n");
                            exit(1);
                        }

                        dup2(FILE, STDIN_FILENO);
                        args[i] = NULL;
                        args[i + 1] = NULL;
                        caseRedirection = 1;
                        break;
                    }
                    else if (strcmp(args[i], ">") == 0)
                    {
                        //case output from file
                        FILE = open(args[i + 1], O_WRONLY | O_CREAT, 0644);
                        if (FILE == -1 || args[i+1]  == NULL)
                        {
                            printf("Invalid Command!\n");
                            exit(1);
                        }

                        dup2(FILE, STDOUT_FILENO);
                        args[i] = NULL;
                        args[i + 1] = NULL;
                        caseRedirection = 2;
                        break;
                    } // case pipe
                    else if (strcmp(args[i], "|") == 0) {
                        Pipping = 1;

                        int fd1[2];

                        if (pipe(fd1) == -1)
                        {
                            fprintf(stderr, "Pipe Failed\n");
                            return 1;
                        }

                        // The 2 command, enter a pipe and enter stdout, run command 1
                        // pipe into stdin, run command 2
                        char *firstCommand[i + 1];
                        char *secondCommand[argv - i - 1 + 1];
                        for (int j = 0; j < i; j++)
                            firstCommand[j] = args[j];

                        firstCommand[i] = NULL;

                        for (int j = 0; j < argv - i - 1; j++)
                            secondCommand[j] = args[j + i + 1];

                        secondCommand[argv - i - 1] = NULL;

                        // Fork pipe
                        int pid_pipe = fork();
                        if (pid_pipe > 0)
                        {
                            wait(NULL);
                            close(fd1[1]);
                            dup2(fd1[0], STDIN_FILENO);
                            close(fd1[0]);

                            if (execvp(secondCommand[0], secondCommand) == -1)
                            {
                                printf("Invalid Command!\n");
                                return 1;
                            }

                        }
                        else if (pid_pipe == 0)
                        {
                            close(fd1[0]);
                            dup2(fd1[1], STDOUT_FILENO);
                            close(fd1[1]);

                            if (execvp(firstCommand[0], firstCommand) == -1)
                            {
                                printf("Invalid Command!\n");
                                return 1;
                            }

                            exit(1);
                        }

                        close(fd1[0]);
                        close(fd1[1]);
                        break;
                    }
                }

                //case arguments dont have redirect
                if (Pipping == 0)
                {
                    if (execvp(args[0], args) == -1)
                    {
                        printf("Invalid Command!\n");
                        return 1;
                    }
                }

                if (caseRedirection == 1)
                    close(STDIN_FILENO);

                else if (caseRedirection == 2)
                    close(STDOUT_FILENO);

                close(FILE);
            }

            exit(1);

        }
        else if (pid > 0)
        {
            if (amp_flag == 0) wait(NULL);
        }
        else
            printf("Error fork!!");
    }

    return 0;
}  // end of "main"


// ==== freeArguments ===========================================================
/*
    This is the freeArguments function. This function grabs the arguemnts,
    and argument count to traverse and free it from the stack.
*/
// ============================================================================
void freeArguments(char *args[],int argv)
{
    for(int i = 0; i < argv; i++)
    {
        if(args[i] != NULL)
        {
            free(args[i]);
            if(i == 80)
                break;
        }
    }
}  // end of "freeArguments"

// ==== userCommands =========================================================
/*
    This is the userCommands function. This function separates the arguments
    and remove any delimiters and special Linux commands.
*/
// ============================================================================
void userCommands(char *args[], int *hasAmp, int *argv)
{
    char buffer[MAX_LINE];
    int length = 0;
    char delimiter[] = " ";

    length = read(STDIN_FILENO, buffer, 80);
    if(buffer[length-1] == '\n')
        buffer[length-1] = '\0';

    if(strcmp(buffer, "!!") == 0)
    {
        if(*argv == 0)
            printf("No history...\n");
        return;
    }

    freeArguments(args, *argv);
    *argv = 0;
    *hasAmp = 0;

    char *ptr = strtok(buffer, delimiter);

    while (ptr != NULL) {
        if (ptr[0] == '&') {
            *hasAmp = 1;
            ptr = strtok(NULL, delimiter);
            continue;
        }
        *argv += 1;
        args[*argv - 1] = strdup(ptr);
        ptr = strtok(NULL, delimiter);
    }
    args[*argv] = NULL;
}  // end of "readCommandFromUser"

//==========================================================
// OUTPUT from prog.c
//==========================================================
// osh>ps -ael
// F S   UID   PID  PPID  C PRI  NI ADDR SZ WCHAN  TTY          TIME CMD
// 4 S     0     1     0  0  80   0 - 29947 -      ?        00:00:01 systemd
// 1 S     0     2     0  0  80   0 -     0 -      ?        00:00:00 kthreadd
// 1 S     0     3     2  0  80   0 -     0 -      ?        00:00:00 ksoftirqd/0
// 1 S     0     5     2  0  60 -20 -     0 -      ?        00:00:00 kworker/0:0H
// 1 S     0     7     2  0  80   0 -     0 -      ?        00:00:01 rcu_sched
// 1 S     0     8     2  0  80   0 -     0 -      ?        00:00:00 rcu_bh
// 1 S     0     9     2  0 -40   - -     0 -      ?        00:00:00 migration/0
// 5 S     0    10     2  0 -40   - -     0 -      ?        00:00:00 watchdog/0
// 5 S     0    11     2  0 -40   - -     0 -      ?        00:00:00 watchdog/1
// 1 S     0    12     2  0 -40   - -     0 -      ?        00:00:00 migration/1
// 1 S     0    13     2  0  80   0 -     0 -      ?        00:00:00 ksoftirqd/1
// 1 S     0    15     2  0  60 -20 -     0 -      ?        00:00:00 kworker/1:0H
// 5 S     0    16     2  0 -40   - -     0 -      ?        00:00:00 watchdog/2
// 1 S     0    17     2  0 -40   - -     0 -      ?        00:00:00 migration/2
// 1 S     0    18     2  0  80   0 -     0 -      ?        00:00:00 ksoftirqd/2
// 1 S     0    20     2  0  60 -20 -     0 -      ?        00:00:00 kworker/2:0H
// 5 S     0    21     2  0 -40   - -     0 -      ?        00:00:00 watchdog/3
// 1 S     0    22     2  0 -40   - -     0 -      ?        00:00:00 migration/3
// 1 S     0    23     2  0  80   0 -     0 -      ?        00:00:00 ksoftirqd/3
// 1 S     0    25     2  0  60 -20 -     0 -      ?        00:00:00 kworker/3:0H
// 5 S     0    26     2  0  80   0 -     0 -      ?        00:00:00 kdevtmpfs
// 1 S     0    27     2  0  60 -20 -     0 -      ?        00:00:00 netns
// 1 S     0    28     2  0  60 -20 -     0 -      ?        00:00:00 perf
// 1 S     0    29     2  0  80   0 -     0 -      ?        00:00:00 khungtaskd
// 1 S     0    30     2  0  60 -20 -     0 -      ?        00:00:00 writeback
// 1 S     0    31     2  0  85   5 -     0 -      ?        00:00:00 ksmd
// 1 S     0    32     2  0  99  19 -     0 -      ?        00:00:00 khugepaged
// 1 S     0    33     2  0  60 -20 -     0 -      ?        00:00:00 crypto
// 1 S     0    34     2  0  60 -20 -     0 -      ?        00:00:00 kintegrityd
// 1 S     0    35     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    36     2  0  60 -20 -     0 -      ?        00:00:00 kblockd
// 1 S     0    37     2  0  60 -20 -     0 -      ?        00:00:00 ata_sff
// 1 S     0    38     2  0  60 -20 -     0 -      ?        00:00:00 md
// 1 S     0    39     2  0  60 -20 -     0 -      ?        00:00:00 devfreq_wq
// 1 S     0    43     2  0  80   0 -     0 -      ?        00:00:00 kswapd0
// 1 S     0    44     2  0  60 -20 -     0 -      ?        00:00:00 vmstat
// 1 S     0    45     2  0  80   0 -     0 -      ?        00:00:00 fsnotify_mark
// 1 S     0    46     2  0  80   0 -     0 -      ?        00:00:00 ecryptfs-kthrea
// 1 S     0    62     2  0  60 -20 -     0 -      ?        00:00:00 kthrotld
// 1 S     0    63     2  0  60 -20 -     0 -      ?        00:00:00 acpi_thermal_pm
// 1 S     0    64     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    65     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    66     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    67     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    68     2  0  80   0 -     0 -      ?        00:00:00 kworker/0:1
// 1 S     0    69     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    70     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    71     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    72     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0    73     2  0  80   0 -     0 -      ?        00:00:00 scsi_eh_0
// 1 S     0    74     2  0  60 -20 -     0 -      ?        00:00:00 scsi_tmf_0
// 1 S     0    75     2  0  80   0 -     0 -      ?        00:00:00 scsi_eh_1
// 1 S     0    76     2  0  60 -20 -     0 -      ?        00:00:00 scsi_tmf_1
// 1 S     0    79     2  0  80   0 -     0 -      ?        00:00:00 kworker/1:1
// 1 S     0    83     2  0  60 -20 -     0 -      ?        00:00:00 ipv6_addrconf
// 1 S     0    96     2  0  60 -20 -     0 -      ?        00:00:00 deferwq
// 1 S     0    97     2  0  60 -20 -     0 -      ?        00:00:00 charger_manager
// 1 S     0    98     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0   150     2  0  60 -20 -     0 -      ?        00:00:00 kpsmoused
// 1 S     0   151     2  0  80   0 -     0 -      ?        00:00:00 kworker/1:2
// 1 S     0   181     2  0  60 -20 -     0 -      ?        00:00:00 kworker/1:1H
// 1 S     0   183     2  0  80   0 -     0 -      ?        00:00:00 scsi_eh_2
// 1 S     0   184     2  0  60 -20 -     0 -      ?        00:00:00 scsi_tmf_2
// 1 S     0   185     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0   259     2  0  60 -20 -     0 -      ?        00:00:00 raid5wq
// 1 S     0   285     2  0  60 -20 -     0 -      ?        00:00:00 bioset
// 1 S     0   318     2  0  60 -20 -     0 -      ?        00:00:00 kworker/3:1H
// 1 S     0   320     2  0  80   0 -     0 -      ?        00:00:00 jbd2/sda1-8
// 1 S     0   321     2  0  60 -20 -     0 -      ?        00:00:00 ext4-rsv-conver
// 1 S     0   349     2  0  60 -20 -     0 -      ?        00:00:00 kworker/0:1H
// 1 S     0   372     2  0  80   0 -     0 -      ?        00:00:00 kworker/3:2
// 1 S     0   383     2  0  60 -20 -     0 -      ?        00:00:00 iscsi_eh
// 4 S     0   388     1  0  80   0 -  8817 -      ?        00:00:00 systemd-journal
// 1 S     0   390     2  0  60 -20 -     0 -      ?        00:00:00 ib_addr
// 1 S     0   396     2  0  80   0 -     0 -      ?        00:00:00 kauditd
// 1 S     0   398     2  0  60 -20 -     0 -      ?        00:00:00 ib_mcast
// 1 S     0   401     2  0  60 -20 -     0 -      ?        00:00:00 ib_nl_sa_wq
// 1 S     0   403     2  0  60 -20 -     0 -      ?        00:00:00 ib_cm
// 1 S     0   406     2  0  80   0 -     0 -      ?        00:00:00 kworker/3:3
// 1 S     0   412     2  0  60 -20 -     0 -      ?        00:00:00 iw_cm_wq
// 1 S     0   414     2  0  60 -20 -     0 -      ?        00:00:00 rdma_cm
// 4 S     0   433     1  0  80   0 - 23692 -      ?        00:00:00 lvmetad
// 4 S     0   441     1  0  80   0 - 11136 -      ?        00:00:00 systemd-udevd
// 1 S     0   477     2  0  80   0 -     0 -      ?        00:00:00 kworker/0:2
// 1 S     0   485     2  0  60 -20 -     0 -      ?        00:00:00 iprt-VBoxWQueue
// 1 S     0   625     2  0  60 -20 -     0 -      ?        00:00:00 ttm_swap
// 4 S     0   772     1  0  80   0 - 23841 -      ?        00:00:00 lxcfs
// 4 S     0   777     1  0  80   0 - 68967 -      ?        00:00:00 accounts-daemon
// 0 S     0   780     1  0  80   0 -  1098 -      ?        00:00:00 acpid
// 4 S     0   783     1  0  80   0 -  7148 -      ?        00:00:00 systemd-logind
// 4 S     0   786     1  0  80   0 -  7251 -      ?        00:00:00 cron
// 4 S   104   788     1  0  80   0 - 64097 -      ?        00:00:00 rsyslogd
// 4 S     1   790     1  0  80   0 -  6510 -      ?        00:00:00 atd
// 4 S     0   795     1  0  80   0 - 93293 -      ?        00:00:00 snapd
// 4 S   107   796     1  0  80   0 - 10724 -      ?        00:00:00 dbus-daemon
// 1 S     0   803     2  0  80   0 -     0 -      ?        00:00:00 kworker/2:3
// 1 S     0   824     1  0  80   0 -  3342 -      ?        00:00:00 mdadm
// 4 S     0   827     1  0  80   0 - 69277 -      ?        00:00:00 polkitd
// 4 S   100   889     1  0  80   0 - 24022 -      ?        00:00:00 systemd-timesyn
// 1 S     0   892     2  0  60 -20 -     0 -      ?        00:00:00 kworker/2:1H
// 1 S     0   893     1  0  80   0 -  4029 -      ?        00:00:00 dhclient
// 4 S     0  1018     1  0  80   0 - 16376 -      ?        00:00:00 sshd
// 1 S     0  1034     1  0  80   0 -  1304 -      ?        00:00:00 iscsid
// 5 S     0  1035     1  0  70 -10 -  1429 -      ?        00:00:00 iscsid
// 5 S     0  1103     1  0  80   0 -  4867 -      ?        00:00:00 irqbalance
// 4 S     0  1108     1  0  80   0 - 16980 -      tty1     00:00:00 login
// 4 S  1000  1510     1  0  80   0 - 11318 ep_pol ?        00:00:00 systemd
// 5 S  1000  1513  1510  0  80   0 - 36333 -      ?        00:00:00 (sd-pam)
// 4 S  1000  1516  1108  0  80   0 -  5648 wait_w tty1     00:00:00 bash
// 4 S     0  1537  1018  0  80   0 - 23730 -      ?        00:00:00 sshd
// 5 S  1000  1568  1537  0  80   0 - 23730 -      ?        00:00:00 sshd
// 0 S  1000  1569  1568  0  80   0 -  3174 wait   ?        00:00:00 bash
// 0 S  1000  1646  1569  0  80   0 -  1125 wait   ?        00:00:00 sh
// 0 R  1000  1653  1646  0  80   0 - 276309 -     ?        00:00:11 node
// 0 S  1000  1729     1  0  80   0 -  1125 wait   ?        00:00:00 sh
// 0 S  1000  1730  1729  0  80   0 - 918943 futex_ ?       00:00:01 vsls-agent
// 4 S     0  1790  1018  0  80   0 - 23730 -      ?        00:00:00 sshd
// 5 S  1000  1829  1790  0  80   0 - 23768 -      ?        00:00:04 sshd
// 0 S  1000  1830  1829  0  80   0 -  3174 wait   ?        00:00:00 bash
// 0 R  1000  1882  1653  3  80   0 - 339360 -     ?        00:03:06 node
// 0 S  1000  1893  1653  0  80   0 - 217191 ep_pol ?       00:00:01 node
// 0 S  1000  1948  1882  0  80   0 - 348129 unix_s ?       00:00:14 cpptools
// 0 S  1000  2022  1653  0  80   0 -  5621 wait   pts/0    00:00:00 bash
// 0 S  1000  2066  1948  0  80   0 - 1241714 futex_ ?      00:00:08 cpptools-srv
// 1 S     0  4375     2  0  80   0 -     0 -      ?        00:00:00 kworker/2:1
// 0 T  1000  8583  2022  0  80   0 -  1630 signal pts/0    00:00:00 prog
// 0 T  1000  8601  2022  0  80   0 -  1630 signal pts/0    00:00:00 prog
// 0 T  1000  8660  8601  0  80   0 -  2413 signal pts/0    00:00:00 less
// 0 T  1000  8677  2022  0  80   0 -  1630 signal pts/0    00:00:00 prog
// 0 T  1000  8686  8677  0  80   0 -  2413 signal pts/0    00:00:00 less
// 1 S     0 13638     2  0  80   0 -     0 -      ?        00:00:00 kworker/u8:2
// 0 S  1000 14325  1948  0  80   0 - 1241747 futex_ ?      00:00:00 cpptools-srv
// 1 R     0 14473     2  0  80   0 -     0 -      ?        00:00:00 kworker/u8:1
// 0 T  1000 14524  2022  0  80   0 -  1630 signal pts/0    00:00:00 prog
// 1 T  1000 14567 14524  0  80   0 -  1630 signal pts/0    00:00:00 prog
// 0 T  1000 14645 14524  0  80   0 -  2413 signal pts/0    00:00:00 less
// 1 T  1000 14647 14567  0  80   0 -  1630 signal pts/0    00:00:00 prog
// 0 T  1000 14673 14645  0  80   0 -  1125 signal pts/0    00:00:00 sh
// 0 Z  1000 14674 14673  0  80   0 -     0 exit   pts/0    00:00:00 bash <defunct>
// 1 S     0 15177     2  0  80   0 -     0 -      ?        00:00:00 kworker/u8:0
// 0 S  1000 15206  1830  0  80   0 -  1821 hrtime ?        00:00:00 sleep
// 0 S  1000 15690  1569  0  80   0 -  1821 hrtime ?        00:00:00 sleep
// 0 S  1000 15715  2022  0  80   0 -  1630 wait   pts/0    00:00:00 prog
// 0 R  1000 15739 15715  0  80   0 -  7228 -      pts/0    00:00:00 ps

// osh>ls > out.txt

// osh>sort < out.txt
// out.txt
// prog
// prog.c
// shell_skeleton.c

// osh>cat out.txt
// out.txt
// prog
// prog.c
// shell_skeleton.c

// osh>!!
// out.txt
// prog
// prog.c
// shell_skeleton.c

// osh>ls -l
// total 32
// -rw-r--r-- 1 osc osc    37 Mar 17 23:01 out.txt
// -rwxrwxr-x 1 osc osc 13736 Mar 17 23:01 prog
// -rw-rw-r-- 1 osc osc  7743 Mar 17 23:01 prog.c
// -rw-rw-r-- 1 osc osc  1547 Mar 17 21:32 shell_skeleton.c

// osh>ls -l | less
// total 32
// -rw-r--r-- 1 osc osc    37 Mar 17 23:01 out.txt
// -rwxrwxr-x 1 osc osc 13736 Mar 17 23:01 prog
// -rw-rw-r-- 1 osc osc  7743 Mar 17 23:01 prog.c
// -rw-rw-r-- 1 osc osc  1547 Mar 17 21:32 shell_skeleton.c
//==========================================================
// end of OUTPUT from prog.c
//==========================================================
