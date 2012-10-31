/*
** RPN Engine for LoraBBS 2.3x
** Scritto da Riccardo De Agostini
**
** File : LPRN.H
**
** NOTA : Questo file e` stato scritto con un Tab size di 4 caratteri.
*/

/*
** HEADER UTILIZZATI
*/

/*
** COSTANTI
*/
#define RPN_NVAR        200             /* Numero variabili utente                  */
#define RPN_STACKSIZE    64             /* Dimensione stack                         */
/*
** Identificativi dei campi del record utente
**
** - Campi di tipo bit (da 1 a 99)
**
**   . Configurazione terminale (1..20)
*/
#define USR_IBMSET      1
#define USR_FORMFEED    2
#define USR_COLOR       3
#define USR_ANSI        4
#define USR_AVATAR      5
#define USR_TABS        6
#define USR_MORE        7
#define USR_HOTKEY      8
/*
**   . Configurazione aree messaggi / files (21..40)
*/
#define USR_SCANMAIL   21
#define USR_FULLREAD   22
#define USR_USELORE    23
#define USR_KLUDGE     24
/*
**   . Privilegi e accessi vari (41..60)
*/
#define USR_HIDDEN     41
#define USR_NOKILL     42
#define USR_NERD       43
#define USR_XFERPRIOR  44
/*
** Campi numerici (da 100 in poi)
**
**   . Configurazione terminale (101..120)
*/
#define USR_LANGUAGE  101
#define USR_LEN       102
#define USR_WIDTH     103
/*
**   . Configurazione aree messaggi / files (121..140)
*/
#define USR_PROTOCOL  121
#define USR_ARCHIVER  122
#define USR_MSGSIG    123
#define USR_FILESIG   124
/*
**   . Privilegi e accessi vari (141..160)
*/
#define USR_PRIV      141
#define USR_CREDIT    142

/*
** DEFINIZIONE DI TIPI CUSTOM
**
** Variabile booleana
*/
typedef enum {
	FALSE = 0,
	TRUE = 1
} bool;

/* Fine del file LRPN.H */
