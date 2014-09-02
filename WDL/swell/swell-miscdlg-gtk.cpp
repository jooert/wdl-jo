/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux
   Copyright (C) 2006-2008, Cockos, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
  

    This file provides basic APIs for browsing for files, directories, and messageboxes.

    These APIs don't all match the Windows equivelents, but are close enough to make it not too much trouble.

 
    (GTK version)
  */


#ifndef SWELL_PROVIDED_BY_APP

#include <gtk/gtk.h>

#include "swell.h"
#include "swell-internal.h"

void BrowseFile_SetTemplate(const char* dlgid, DLGPROC dlgProc, struct SWELL_DialogResourceIndex *reshead)
{
}

// return true
bool BrowseForSaveFile(const char *text, const char *initialdir, const char *initialfile, const char *extlist,
                       char *fn, int fnsize)
{
  return false;
}

bool BrowseForDirectory(const char *text, const char *initialdir, char *fn, int fnsize)
{
  return false;
}



char *BrowseForFiles(const char *text, const char *initialdir, 
                     const char *initialfile, bool allowmul, const char *extlist)
{
  return NULL;
}

int MessageBox(HWND hwndParent, const char *text, const char *caption, int type)
{
  GtkWidget* dialog = gtk_message_dialog_new(NULL,//hwndParent->m_oswindow,
					     GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_MESSAGE_OTHER,
					     GTK_BUTTONS_NONE,
					     text);
  gtk_window_set_resizable(GTK_WINDOW(dialog), false);
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(dialog), caption);

  if (type == MB_OK)
  {
    gtk_dialog_add_button(GTK_DIALOG(dialog), "OK", IDOK);
  }	
  else if (type == MB_OKCANCEL)
  {
    gtk_dialog_add_buttons(GTK_DIALOG(dialog), "OK", IDOK, "Cancel", IDCANCEL, NULL);
  }
  else if (type == MB_YESNO)
  {
    gtk_dialog_add_buttons(GTK_DIALOG(dialog), "Yes", IDYES, "No", IDNO, NULL);
  }
  else if (type == MB_RETRYCANCEL)
  {
    gtk_dialog_add_buttons(GTK_DIALOG(dialog), "Retry", IDRETRY, "Cancel", IDCANCEL, NULL);
  }
  else if (type == MB_YESNOCANCEL)
  {
    gtk_dialog_add_buttons(GTK_DIALOG(dialog), "Yes", IDYES, "No", IDNO, "Cancel", IDCANCEL, NULL);
  }

  int result = gtk_dialog_run(GTK_DIALOG(dialog));
  if(result == GTK_RESPONSE_NONE)
  {
    result = 0;
  }
  gtk_widget_destroy(dialog);
  return result;
}

#endif
