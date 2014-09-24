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
#include "../wdlcstring.h"

static void swell_parseExtlist(GtkFileChooser *chooser, const char *extlist)
{
  if(extlist)
  {
     // Iterate over list of extensions
    int it = 0;
    while(!(extlist[it] == '\0' && extlist[++it] == '\0'))
    {
      GtkFileFilter *filter = gtk_file_filter_new();
      // First string contains name of filter
      gtk_file_filter_set_name(filter, &extlist[it]);
      it += strlen(&extlist[it]) + 1;

      // Second string contains patterns, semicolon-separated
      // Make patterns case insensitive: Replace *.xyz with *.[xX][yY][zZ]
      char buf[128] = {0};
      int bufi = 0;
      for(int i = it, len = strlen(&extlist[it]) + it + 1; i < len; ++i)
      {
	if(extlist[i] == ';' || i + 1 == len)
	{
	  buf[bufi] = '\0';
	  gtk_file_filter_add_pattern(filter, buf);
	  bufi = 0;
	}
	else if(isalpha(extlist[i]))
	{
	  buf[bufi++] = '[';
	  buf[bufi++] = tolower(extlist[i]);
	  buf[bufi++] = toupper(extlist[i]);
	  buf[bufi++] = ']';
	}
	else
	{
	  buf[bufi++] = extlist[i];
	}
      }
      gtk_file_chooser_add_filter(chooser, filter);
      it += strlen(&extlist[it]);
    }
  }
}

void BrowseFile_SetTemplate(const char* dlgid, DLGPROC dlgProc, struct SWELL_DialogResourceIndex *reshead)
{
}

// return true
bool BrowseForSaveFile(const char *text, const char *initialdir, const char *initialfile, const char *extlist,
                       char *fn, int fnsize)
{
  GtkWidget* dialog = gtk_file_chooser_dialog_new(text, NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
				       "_Cancel", GTK_RESPONSE_CANCEL,
				       "_Save", GTK_RESPONSE_ACCEPT,
				       NULL);
  GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
  gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

  // Set initialfile or initialdir if given
  if(initialfile && initialfile[0] != '\0')
  {
    gtk_file_chooser_set_filename(chooser, initialfile);
  }
  else if(initialdir && initialdir[0] != '\0')
  {
    gtk_file_chooser_set_current_folder(chooser, initialdir);
  }

  swell_parseExtlist(chooser, extlist);

  gint res = gtk_dialog_run(GTK_DIALOG(dialog));
  bool ret = false;
  if(res == GTK_RESPONSE_ACCEPT)
  {
    char* buf = gtk_file_chooser_get_filename(chooser);
    lstrcpyn_safe(fn, buf, fnsize);
    g_free(buf);
    ret = true;
  }
  gtk_widget_destroy(dialog);

  return ret;
}

bool BrowseForDirectory(const char *text, const char *initialdir, char *fn, int fnsize)
{
  GtkWidget* dialog = gtk_file_chooser_dialog_new(text, NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				       "_Cancel", GTK_RESPONSE_CANCEL,
				       "_Open", GTK_RESPONSE_ACCEPT,
				       NULL);
  GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);

  // Set initialdir if given
  if(initialdir && initialdir[0] != '\0')
  {
    gtk_file_chooser_set_current_folder(chooser, initialdir);
  }

  gint res = gtk_dialog_run(GTK_DIALOG(dialog));
  bool ret = false;
  if(res == GTK_RESPONSE_ACCEPT)
  {
    char* buf = gtk_file_chooser_get_filename(chooser);
    lstrcpyn_safe(fn, buf, fnsize);
    g_free(buf);
    ret = true;
  }
  gtk_widget_destroy(dialog);

  return ret;
}



char *BrowseForFiles(const char *text, const char *initialdir, 
                     const char *initialfile, bool allowmul, const char *extlist)
{
  GtkWidget* dialog = gtk_file_chooser_dialog_new(text, NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
				       "_Cancel", GTK_RESPONSE_CANCEL,
				       "_Open", GTK_RESPONSE_ACCEPT,
				       NULL);
  GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
  gtk_file_chooser_set_select_multiple(chooser, allowmul);

  // Set initialfile or initialdir if given
  if(initialfile && initialfile[0] != '\0')
  {
    gtk_file_chooser_set_filename(chooser, initialfile);
  }
  else if(initialdir && initialdir[0] != '\0')
  {
    gtk_file_chooser_set_current_folder(chooser, initialdir);
  }

  swell_parseExtlist(chooser, extlist);

  gint res = gtk_dialog_run(GTK_DIALOG(dialog));
  char* ret = NULL;
  if(res == GTK_RESPONSE_ACCEPT)
  {
    if(!allowmul)
    {
      char* buf = gtk_file_chooser_get_filename(chooser);
      size_t length = strlen(buf) + 1;
      ret = (char*)malloc(length);
      lstrcpyn_safe(ret, buf, length);
      g_free(buf);
    }
    else
    {
      char* buf = gtk_file_chooser_get_current_folder(chooser);
      size_t length = strlen(buf) + 1;
      ret = (char*)malloc(length);
      lstrcpyn_safe(ret, buf, length);
      g_free(buf);

      // Iterate over filenames
      GSList* fns = gtk_file_chooser_get_filenames(chooser);
      while(fns)
      {
	char* fn = (char*)fns->data;
	size_t len = strlen(fn) + 1, i = len;
	// Retrieve only filename from path
	while(fn[i-1] != '/')
	{
	  --i;
	}
	ret = (char*)realloc(ret, length + len - i);
	lstrcpyn_safe(&ret[length], &fn[i], len - i);
	length += len - i;
	g_free(fn);
	fns = fns->next;
      }
      g_slist_free(fns);
      ret = (char*)realloc(ret, length + 1);
      ret[length] = '\0';
    }
  }
  gtk_widget_destroy(dialog);

  return ret;
}

int MessageBox(HWND hwndParent, const char *text, const char *caption, int type)
{
  GtkWidget* dialog = gtk_message_dialog_new((hwndParent && hwndParent->m_oswindow) ? GTK_WINDOW(hwndParent->m_oswindow) : NULL,
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
