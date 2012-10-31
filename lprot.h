
extern struct parse_list levels[];

void start_update (void);
void stop_update (void);
char *get_flag_text (long);
char *get_flagA_text (long);
char *get_flagB_text (long);
char *get_flagC_text (long);
char *get_flagD_text (long);
int select_level (int, int, int);
long recover_flags (char *);
void parse_netnode (char *, int *, int *, int *, int *);
