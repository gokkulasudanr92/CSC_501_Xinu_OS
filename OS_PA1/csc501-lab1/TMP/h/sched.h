#define EXPDISTSCHED 1
#define LINUXSCHED 2

/* Definition to set the schedclass */
void setschedclass(int sched_class);

/* Definition to get the schedclass */
int getschedclass();

/* Definition to set the first epoch */
void set_first_epoch(int value);

/* Definition to get the first epoch */
int get_first_epoch();
