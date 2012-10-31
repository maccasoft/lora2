/*
** RPN Engine for LoraBBS 2.3x
**
** Scritto da Riccardo De Agostini
**
** NOTA: Questo file e` stato scritto con un Tab size di 4 caratteri.
*/

/*
** HEADER UTILIZZATI
*/
#include <stdio.h>
#include <limits.h>

#include "lsetup.h"
#include "sched.h"
#include "msgapi.h"
#include "externs.h"
#include "prototyp.h"
#include "lrpn.h"

extern int ansi_attr;
extern char vx, vy;

/*
** COSTANTI
*/
#define _FALSE   0
#define _TRUE    1
#define MAX_STP  (RPN_STACKSIZE - 1)

static const long  flagval[32] =  {
	0x80000000L,	/* Flag 0 */
	0x40000000L,	/* Flag 1 */
	0x20000000L,	/* Flag 2 */
	0x10000000L,	/* Flag 3 */
	0x08000000L,	/* Flag 4 */
	0x04000000L,	/* Flag 5 */
	0x02000000L,	/* Flag 6 */
	0x01000000L,	/* Flag 7 */
	0x00800000L,	/* Flag 8 */
	0x00400000L,	/* Flag 9 */
	0x00200000L,	/* Flag A */
	0x00100000L,	/* Flag B */
	0x00080000L,	/* Flag C */
	0x00040000L,	/* Flag D */
	0x00020000L,	/* Flag E */
	0x00010000L,	/* Flag F */
	0x00008000L,	/* Flag G */
	0x00004000L,	/* Flag H */
	0x00002000L,	/* Flag I */
	0x00001000L,	/* Flag J */
	0x00000800L,	/* Flag K */
	0x00000400L,	/* Flag L */
	0x00000200L,	/* Flag M */
	0x00000100L,	/* Flag N */
	0x00000080L,	/* Flag O */
	0x00000040L,	/* Flag P */
	0x00000020L,	/* Flag Q */
	0x00000010L,	/* Flag R */
	0x00000008L,	/* Flag S */
	0x00000004L,	/* Flag T */
	0x00000002L,	/* Flag U */
	0x00000001L		/* Flag V */
};

/*
** DATI PRIVATI DEL MODULO
*/
static short        uvar[RPN_NVAR];			/* Variabili utente             */
static short        stack[RPN_STACKSIZE];	/* Stack                        */
static short        stp;					/* Stack pointer                */
static short        acc;					/* Accumulatore                 */
static bool         acc_reset;				/* TRUE = La prossima cifra     */
											/*  resettera` l'accumulatore   */
/*
** PROTOTIPI DELLE FUNZIONI PRIVATE DEL MODULO
*/
short get_usr(short);
void set_usr(short, short);

/*
** MACRO
*/
#define PUSH(x)    if (stp < MAX_STP) stp++; stack[stp] = (x)
#define POP()      (stp < 0 ? 0 : stack[stp--])
#define ABS(x)     ((x) < 0 ? -(x) : (x))

/*==========================================================================*/

/*
** Funzione  : rpnInit
** Scopo     : Inizializza RPN Engine
** Parametri : u - Il puntatore ad una struct _usr che definisce i dati
**                 dell'utente
** Ritorna   :
** Note      : - Questa funzione DEVE essere chiamata prima di utilizzare
**               una qualsiasi altra funzione di RPN Engine, nonche` ogni
**               volta si desideri, per qualsiasi motivo, reinizializzare
**               lo stack e le variabili utente.
*/
void rpnInit (void)
{
	register short  i;
	register short  *p;

	/*
	** Inizializza il puntatore allo stack
	*/
	stp = -1;
	/*
	** Inizializza le variabili utente
	*/
	for (i = 0, p = uvar; i < RPN_NVAR; i++, *(p++) = 0);
}

/*
** Funzione  : rpnProcessChar
** Scopo     : Processa un singolo carattere
** Parametri : c - Il carattere in questione
** Ritorna   :
** Note      : - La funzione non ritorna alcun codice di errore
**             - I caratteri non riconosciuti vengono ignorati, cosi` come
**               le indirezioni illecite su variabili o campi del
**               record utente
**             - Anche se eventuali stack overflow o underflow sono gestiti
**               in modo da non incasinare il sistema, occorre fare
**               attenzione in fase di sviluppo delle schermate; ad esempio,
**               se si vogliono passare dei parametri tramite lo stack ad una
**               schermata e questa non e` presente su disco, lo stack
**               conterra` valori "in piu`" e alla lunga l'overflow sara`
**               assicurato
**
*/
void rpnProcessChar(int c)
{
   short tmp;
   long ltmp;

   /*
   ** Cifre da 0 a 9
   */
   if ((c >= '0') && (c <= '9')) {
      if (acc_reset) {
         PUSH(acc);
         acc = 0;
         acc_reset = FALSE;
      }
      /*
      ** Un modo come un altro per evitare l'overflow
      */
      ltmp = (long)acc * 10 + c - '0';
      acc =  ltmp > 32767L ? 32767 : ltmp & 0xFFFF;
      return;
   }
   switch (c) {
      /*
      ** Interazione diretta con lo stack
      */
      case ',':       /* Inserimento accumulatore nello stack                     */
		PUSH(acc);
		acc = 0;
		acc_reset = FALSE;
		break;
	case '@':	/* Estrazione valore dallo stack nell'accumulatore          */
		acc = POP();
		acc_reset = TRUE;
		break;
	case '\"':	/* Scambio tra l'accumulatore e lo stack                    */
		if (stp < 0) {
			acc = 0;
		} else {
			tmp = acc;
			acc = stack[stp];
			stack[stp] = tmp;
		}
		acc_reset = TRUE;
		break;
	/*
	** Operatori matematici binari
	** (qui per stack si intende, per brevita`, l'ultimo elemento inserito)
	*/
	case '+':	/* Addizione tra l'accumulatore e lo stack                  */
		acc += POP();
		acc_reset = TRUE;
		break;
	case '-':	/* Sottrazione dell'accumulatore dallo stack                */
		acc = POP() - acc;
		acc_reset = TRUE;
		break;
	case '*':	/* Moltiplicazione dello stack per l'accumulatore           */
		acc = ((long)acc * POP()) & 0xFFFF;
		acc_reset = TRUE;
	case '/':	/* Divisione tra lo stack e l'accumulatore                  */
		if (acc == 0) {
			acc = POP() < 0 ? SHRT_MIN : SHRT_MAX;
		} else {
			acc = POP() / acc;
		}
		acc_reset = TRUE;
		break;
	case '%':	/* Resto della divisione tra lo stack e l'accumulatore      */
		if (acc == 0) {
			(void)POP();
		} else {
			acc = POP() % acc;
		}
		acc_reset = TRUE;
		break;
	case 'L':	/* Shift a sinistra                                         */
		acc = POP() << ABS(acc);
		acc_reset = TRUE;
		break;
	case 'R':	/* Shift a destra                                           */
		tmp = ABS(acc);
		if (tmp) {
			acc = (POP() >> 1) & 0x7FFF;
			acc >>= --tmp;
		} else {
			(void)POP();
                }
                acc_reset = TRUE;
		break;
	/*
	** Operatori matematici unari
	*/
	case '_':	/* Cambio segno dell'accumulatore                           */
		acc = -acc;
		acc_reset = TRUE;
		break;
	case '\'':	/* Valore assoluto dell'accumulatore                        */
		acc = ABS(acc);
		acc_reset = TRUE;
		break;
	case '$':	/* Segno dell'accumulatore (-1 se < 0, 0 se 0, 1 se > 0)    */
		if (acc) {
			acc = (acc < 0) ? -1 : 1;
		}
		acc_reset = TRUE;
		break;
	/*
	** Operatori logici e bitwise binari
	*/
	case '&':	/* Bitwise AND                                              */
		acc &= POP();
		acc_reset = TRUE;
		break;
	case '|':	/* Bitwise OR                                               */
		acc |= POP();
		acc_reset = TRUE;
		break;
	case '^':	/* Bitwise XOR                                              */
		acc ^= POP();
		acc_reset = TRUE;
		break;
	/*
	** Operatori logici e bitwise unari
	*/
	case '#':	/* Converte l'accumulatore in un valore booleano            */
		acc = acc ? _TRUE : _FALSE;
	case '~':	/* Inversione dei bit dell'accumulatore                     */
		acc = ~acc;
		acc_reset = TRUE;
		break;
	case '!':	/* Negazione logica dell'accumulatore                       */
		acc = acc ? _TRUE : _FALSE;
		acc_reset = TRUE;
		break;
	/*
	** Operatori di confronto
	*/
	case '=':	/* 1 se stk == acc                                          */
		acc = (POP() == acc) ? _TRUE : _FALSE;
	case '<':	/* 1 se stk < acc                                           */
		acc = (POP() < acc) ? _TRUE : _FALSE;
	case '>':	/* 1 se stk > acc                                           */
		acc = (POP() > acc) ? _TRUE : _FALSE;
	/*
	** Operatori vari
	*/
	case 'W':	/* Costruzione word (stk -> byte alto, acc -> byte basso)   */
		acc = (POP() << 8) | (acc & 0x00FF);
		acc_reset = TRUE;
		break;
	case 'w':	/* Split word (byte alto -> stk, byte basso -> acc)         */
		PUSH((acc >> 8) & 0x00FF);
		acc &= 0x00FF;
		acc_reset = TRUE;
		break;
	/*
	** Interazione con le variabili
	*/
	case 'V':	/* Carica variabile : acc <-- V[acc]                        */
		if ((acc >= 0) && (acc < RPN_NVAR)) {
			acc = uvar[acc];
		} else {
			acc = 0;
		}
		acc_reset = TRUE;
		break;
	case 'v':	/* Memorizza variabile : stack --> V[acc]                   */
		tmp = POP();
		if ((acc >= 0) && (acc < RPN_NVAR)) {
			uvar[acc] = tmp;
		}
		acc_reset = TRUE;
		break;
	/*
	** Interazione con i flag dell'utente
	*/
	case 'F':	/* Carica flag : acc = flag[acc] ? _TRUE : _FALSE           */
		if ((acc >= 0) && (acc < 32)) {
                        acc = (usr.flags & flagval[acc]) ? _TRUE : _FALSE;
		} else {
			acc = 0;
		}
		acc_reset = TRUE;
		break;
	case 'f':	/* flag[acc] = stack ? 1 : 0                                */
		tmp = POP();
		if ((acc >= 0) && (acc < 32)) {
			if (tmp) {
                                usr.flags |= flagval[acc];
			} else {
                                usr.flags &= ~flagval[acc];
			}
		}
		acc_reset = TRUE;
		break;
	/*
	** Interazione con i contatori utente
	*/
	case 'C':	/* acc <-- C[acc]                                           */
		if ((acc >= 0) && (acc < MAXCOUNTER)) {
                        acc = usr.counter[--acc];
		} else {
			acc = 0;
		}
		acc_reset = TRUE;
		break;
      case 'c':       /* C[acc] <-- stack                                         */
         tmp = POP();
         if ((acc >= 0) && (acc < MAXCOUNTER)) {
            if (tmp < 0)
               usr.counter[--acc] = 0;
            else if (tmp > 255)
               usr.counter[--acc] = tmp;
            else
               usr.counter[--acc] = tmp;
         }
         acc_reset = TRUE;
         break;
	/*
	** Interazione con i dati del record utente
	*/
      case 'U':       /* Carica un campo del record utente : acc <-- U[acc]       */
         acc = get_usr(acc);
         acc_reset = TRUE;
         break;
      case 'u':       /* Memorizza in record utente : U[acc] <-- stack            */
         set_usr(acc, POP());
         acc_reset = TRUE;
         break;
      /*
      ** Posizione cursore
      */
      case 'P':
         acc = (vy << 8) | vx;
         acc_reset = TRUE;
         break;
      case 'p':
         cpos ((acc >> 8) & 0x00FF, acc & 0x00FF);
         acc_reset = TRUE;
         break;
      /*
      ** Attributo di colore
      */
      case 'A':
         acc = ansi_attr;
         acc_reset = TRUE;
         break;
      case 'a':
         change_attr (acc & 0x00FF);
         acc_reset = TRUE;
         break;
      /*
      ** I caratteri sconosciuti vengono ignorati
      */
      default:
         break;
   }
}

char *rpnProcessString (char *p)
{
   while (*p && *p >= 32 && *p != 0x0A && *p != 0x0D)
      rpnProcessChar (*p++);
   p--;
   return (p);
}

/*
** Funzione  : get_usr
** Scopo     : Legge un campo del record utente
** Parametri : field - Identificativo campo
** Ritorna   : Il valore del campo richiesto o 0 in caso di errore
** Note      :
*/
short get_usr(short field)
{
	register short  val;

	switch (field) {
	case USR_IBMSET:
                val = (usr.ibmset) ? _TRUE : _FALSE;
		break;
	case USR_FORMFEED:
                val = (usr.formfeed) ? _TRUE : _FALSE;
		break;
	case USR_COLOR:
                val = (usr.color) ? _TRUE : _FALSE;
		break;
	case USR_ANSI:
                val = (usr.ansi) ? _TRUE : _FALSE;
		break;
	case USR_AVATAR:
                val = (usr.avatar) ? _TRUE : _FALSE;
		break;
	case USR_TABS:
                val = (usr.tabs) ? _TRUE : _FALSE;
		break;
	case USR_MORE:
                val = (usr.more) ? _TRUE : _FALSE;
		break;
	case USR_HOTKEY:
                val = (usr.hotkey) ? _TRUE : _FALSE;
		break;
	case USR_SCANMAIL:
                val = (usr.scanmail) ? _TRUE : _FALSE;
		break;
	case USR_FULLREAD:
                val = (usr.full_read) ? _TRUE : _FALSE;
		break;
	case USR_USELORE:
                val = (usr.use_lore) ? _TRUE : _FALSE;
		break;
	case USR_KLUDGE:
                val = (usr.kludge) ? _TRUE : _FALSE;
		break;
	case USR_HIDDEN:
                val = (usr.usrhidden) ? _TRUE : _FALSE;
		break;
	case USR_NOKILL:
                val = (usr.nokill) ? _TRUE : _FALSE;
		break;
	case USR_NERD:
                val = (usr.nerd) ? _TRUE : _FALSE;
		break;
	case USR_XFERPRIOR:
                val = (usr.xfer_prior) ? _TRUE : _FALSE;
		break;
	case USR_LANGUAGE:
                val = usr.language;
		break;
	case USR_LEN:
                val = usr.len;
		break;
	case USR_WIDTH:
                val = usr.width;
		break;
	case USR_PROTOCOL:
                val = usr.protocol;
		break;
	case USR_ARCHIVER:
                val = usr.archiver;
		break;
	case USR_MSGSIG:
                val = usr.msg_sig;
		break;
	case USR_FILESIG:
                val = usr.file_sig;
		break;
	case USR_PRIV:
                val = usr.priv >> 4;
		break;
	case USR_CREDIT:
                val = usr.credit;
		break;
	default:
		val = 0;
		break;
	}
	return val;
}

/*
** Funzione  : set_usr
** Scopo     : Modifica un campo del record utente
** Parametri : field - Identificativo campo
**             val   - Valore da assegnare al campo
** Ritorna   :
** Note      : Per i campi numerici manca ancora il controllo di validita`
**             del valore da assegnare
*/
void set_usr(short field, short val)
{
	switch (field) {
	case USR_IBMSET:
                usr.ibmset = val ? 1 : 0;
		break;
	case USR_FORMFEED:
                usr.formfeed = val ? 1 : 0;
		break;
	case USR_COLOR:
                usr.color = val ? 1 : 0;
		break;
	case USR_ANSI:
                usr.ansi = val ? 1 : 0;
		break;
	case USR_AVATAR:
                usr.avatar = val ? 1 : 0;
		break;
	case USR_TABS:
                usr.tabs = val ? 1 : 0;
		break;
	case USR_MORE:
                usr.more = val ? 1 : 0;
		break;
	case USR_HOTKEY:
                usr.hotkey = val ? 1 : 0;
		break;
	case USR_SCANMAIL:
                usr.scanmail = val ? 1 : 0;
		break;
	case USR_FULLREAD:
                usr.full_read = val ? 1 : 0;
		break;
	case USR_USELORE:
                usr.use_lore = val ? 1 : 0;
		break;
	case USR_KLUDGE:
                usr.kludge = val ? 1 : 0;
		break;
	case USR_HIDDEN:
                usr.usrhidden = val ? 1 : 0;
		break;
	case USR_NOKILL:
                usr.nokill = val ? 1 : 0;
		break;
	case USR_NERD:
                usr.nerd = val ? 1 : 0;
		break;
	case USR_XFERPRIOR:
                usr.xfer_prior = val ? 1 : 0;
		break;
	case USR_LANGUAGE:
                usr.language = val;
		break;
	case USR_LEN:
                usr.len = val;
		break;
	case USR_WIDTH:
                usr.width = val;
		break;
	case USR_PROTOCOL:
                usr.protocol = val;
		break;
	case USR_ARCHIVER:
                usr.archiver = val;
		break;
	case USR_MSGSIG:
                usr.msg_sig = val;
		break;
	case USR_FILESIG:
                usr.file_sig = val;
		break;
	case USR_PRIV:
                usr.priv = val << 4;
		break;
	case USR_CREDIT:
                usr.credit = val;
		break;
	default:
		break;
	}
}

/* Fine del file LRPN.C */
