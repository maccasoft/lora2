#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <cxl\cxlvid.h>
#include <cxl\cxlstr.h>

#define CLEAN_MODULE
#include "defines.h"
#include "lora.h"
#include "externs.h"
#include "prototyp.h"

void user_configuration()
{
        cls();
        del_line();
        m_print(bbstxt[B_CONFIG_HEADER]);

        m_print(bbstxt[B_CONFIG_NAME],usr.name);

        m_print(bbstxt[B_CONFIG_ALIAS], usr.handle, usr.voicephone);

        m_print(bbstxt[B_CONFIG_CITY], usr.city, usr.dataphone);

        m_print(bbstxt[B_CONFIG_LENGTH],
                usr.len,
                usr.formfeed ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.nulls);

        m_print(bbstxt[B_CONFIG_MORE],
                usr.more ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.scanmail ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.tabs ? bbstxt[B_YES] : bbstxt[B_NO]);

        m_print(bbstxt[B_CONFIG_ANSI],
                usr.ansi ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.use_lore ? bbstxt[B_NO] : bbstxt[B_YES],
                usr.color ? bbstxt[B_YES] : bbstxt[B_NO]);

        m_print(bbstxt[B_CONFIG_AVATAR],
                usr.avatar ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.full_read ? bbstxt[B_YES] : bbstxt[B_NO],
                usr.hotkey ? bbstxt[B_YES] : bbstxt[B_NO]);

        m_print(bbstxt[B_CONFIG_LANGUAGE], lang_descr[usr.language]);
        m_print(bbstxt[B_CONFIG_SIGN],usr.signature);
}

void more_change()
{
        usr.more^=1;
}

void hotkey_change()
{
        usr.hotkey^=1;
}

void color_change()
{
        if(rate < speed_graphics || (!usr.ansi && !usr.avatar))
		return;
        if(usr.color)
                change_attr(LGREY|_BLACK);
        usr.color^=1;
        if (function_active == 1)
                f1_status();

        if (usr.color)
                m_print(bbstxt[B_COLOR_USED]);
        else
                m_print(bbstxt[B_COLOR_NOT_USED]);
        press_enter ();
}

void ansi_change()
{
        if(rate < speed_graphics)
		return;
        if(usr.ansi)
                change_attr(LGREY|_BLACK);

        usr.ansi^=1;

        if(!usr.ansi)
        {
                usr.use_lore = 1;
                usr.color = 0;
	}
        else {
                usr.avatar = 0;
                usr.color = 1;
        }

        if (function_active == 1)
                f1_status();

        if (usr.ansi)
                m_print(bbstxt[B_ANSI_USED]);
        else
        {
                m_print(bbstxt[B_ANSI_NOT_USED]);
                if (usr.use_lore)
                        m_print(bbstxt[B_FULL_NOT_USED2]);
        }
        m_print(bbstxt[B_TWO_CR]);
        press_enter ();
}

void avatar_change()
{
        if(usr.avatar)
                change_attr(LGREY|_BLACK);
        usr.avatar^=1;

        if(!usr.avatar) {
                usr.use_lore = 1;
                usr.color = 0;
	}
        else {
                usr.ansi = 0;
                usr.color = 1;
        }

        if (function_active == 1)
                f1_status();

        if (usr.avatar)
                m_print(bbstxt[B_AVATAR_USED]);
        else
        {
                m_print(bbstxt[B_AVATAR_NOT_USED]);
                if (usr.use_lore)
                        m_print(bbstxt[B_FULL_NOT_USED2]);
        }
        m_print(bbstxt[B_TWO_CR]);
        press_enter ();
}

void formfeed_change()
{
   usr.formfeed^=1;
}

void scanmail_change()
{
   usr.scanmail^=1;
}

void fullscreen_change()
{
        if(!usr.ansi && !usr.avatar)
		return;

        usr.use_lore^=1;

        if (usr.use_lore)
                m_print(bbstxt[B_FULL_NOT_USED]);
        else
                m_print(bbstxt[B_FULL_USED]);
        press_enter ();
}

void tabs_change()
{
   usr.tabs^=1;
}

void fullread_change()
{
        usr.full_read^=1;

        if (usr.full_read)
                m_print(bbstxt[B_FULLREAD_USED]);
        else
                m_print(bbstxt[B_FULLREAD_NOT_USED]);
        press_enter ();
}

void screen_change()
{
   int col;
   char stringa[6];

   do {
      m_print(bbstxt[B_LINE_CHANGE]);
      sprintf (stringa, "%d", usr.len);
      chars_input (stringa, 3, INPUT_UPDATE|INPUT_FIELD);
      if (!CARRIER)
         col = usr.len;
      else
         col = atoi(stringa);
   } while ((col < 10 || col > 66) && CARRIER);

   usr.len = col;
}

void nulls_change()
{
   int col;
   char stringa[6];

   m_print (bbstxt[B_NULLS_CHANGE]);
   input (stringa,80);

   col = atoi(stringa);

   if (col < 0 || col > 255)
      return;

   usr.nulls = col;
}

void handle_change()
{
   char stringa[40];

   strcpy (stringa, usr.handle);
   m_print(bbstxt[B_ALIAS_CHANGE]);
   chars_input (stringa, 35, INPUT_FANCY|INPUT_UPDATE|INPUT_FIELD);

   strcpy(usr.handle,fancy_str(stringa));
}

void voice_phone_change()
{
   char stringa[20];

   for (;;) {
      m_print(bbstxt[B_VOICE_PHONE]);

      m_print(bbstxt[B_ASK_NUMBER]);
      input(stringa, 19);

      if (!strlen (stringa) || !CARRIER)
         return;

      m_print(bbstxt[B_PHONE_IS], stringa);
      m_print(bbstxt[B_PHONE_OK]);
      if (yesno_question (DEF_YES) == DEF_YES)
         break;
   }

   strcpy(usr.voicephone,stringa);
}

void data_phone_change()
{
   char stringa[20];

   for (;;) {
      m_print(bbstxt[B_DATA_PHONE]);

      m_print(bbstxt[B_ASK_NUMBER]);
      input(stringa, 19);

      if (!strlen (stringa) || !CARRIER)
         return;

      m_print(bbstxt[B_PHONE_IS], stringa);
      m_print(bbstxt[B_PHONE_OK]);
      if (yesno_question (DEF_YES) == DEF_YES)
         break;
   }

   strcpy(usr.dataphone,stringa);
}

void city_change()
{
   char stringa[40];

   m_print(bbstxt[B_CITY_STATE]);
   input(stringa,35);

   if (!strlen (stringa))
      return;

    strcpy(usr.city,stringa);

    if (function_active == 1)
       f1_status();
}

void password_change()
{
   char stringa[20], parola[20];

   read_system_file("CHGPWD");

   m_print(bbstxt[B_ONE_CR]);

   for (;;) {
      m_print(bbstxt[B_SELECT_PASSWORD]);
      inpwd(stringa,15);

      if (!strlen(stringa))
         return;

      strcpy(parola,stringa);

      m_print(bbstxt[B_VERIFY_PASSWORD]);

      m_print(bbstxt[B_PASSWORD]);
      inpwd(stringa,15);

      if (!strlen(stringa) || !CARRIER)
         return;

      if(strcmp(stringa,parola) == 0)
         break;

      m_print(bbstxt[B_WRONG_PWD1],strupr(parola));
      m_print(bbstxt[B_WRONG_PWD2],strupr(stringa));
   }

   strcpy(usr.pwd,strcode(strupr(stringa),usr.name));
}

void signature_change()
{
   char stringa[60];

   m_print(bbstxt[B_SET_SIGN]);
   m_print(bbstxt[B_ASK_SIGN]);
   input(stringa,58);

   strcpy(usr.signature,stringa);
}

int select_language ()
{
   int i;
   char c, stringa[4], strcom[25];

   if (lang_name[1] == NULL)
      return (0);

   cls();

   i = 0;
   strcom[0] = '\0';

   m_print(bbstxt[B_LANGUAGE_AVAIL]);

   while (lang_name[i] != NULL)
   {
      m_print(bbstxt[B_PROTOCOL_FORMAT], lang_keys[i], lang_descr[i]);
      strcom[i] = toupper(lang_keys[i]);
      i++;
   }

   strcom[i] = '\0';

   m_print(bbstxt[B_SELECT_LANGUAGE]);
   input(stringa,1);
   c = toupper(stringa[0]);
   if(c == '\0' || c == '\r' || !CARRIER || !strchr(strcom,c))
      return(-1 );

   i = 0;

   while (lang_name[i] != NULL)
   {
      if (lang_keys[i] == c)
         break;
      i++;
   }

   return (i);
}

