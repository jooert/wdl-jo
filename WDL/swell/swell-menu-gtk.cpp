/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Linux)
   Copyright (C) 2006-2007, Cockos, Inc.

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
  

    This file provides basic windows menu API

  */

#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-menugen.h"

#include "swell-internal.h"

#include "../ptrlist.h"
#include "../wdlcstring.h"

HMENU__ *HMENU__::Duplicate()
{
  HMENU__ *p = new HMENU__;
  int x;
  for (x = 0; x < items.GetSize(); x ++)
  {
    MENUITEMINFO *s = items.Get(x);
    MENUITEMINFO *inf = (MENUITEMINFO*)calloc(sizeof(MENUITEMINFO),1);

    *inf = *s;
    if (inf->dwTypeData)
    {
      // todo handle bitmap types
      inf->dwTypeData=strdup(inf->dwTypeData);
    }
    if (inf->hSubMenu) inf->hSubMenu = inf->hSubMenu->Duplicate();

    p->items.Add(inf);
  }
  return p;
}

void HMENU__::freeMenuItem(void *p)
{
  MENUITEMINFO *inf = (MENUITEMINFO *)p;
  if (!inf) return;
  delete inf->hSubMenu;
  free(inf->dwTypeData); // todo handle bitmap types
  free(inf);
}

static MENUITEMINFO *GetMenuItemByID(HMENU menu, int id, bool searchChildren=true)
{
  if (!menu) return 0;
  int x;
  for (x = 0; x < menu->items.GetSize(); x ++)
    if (menu->items.Get(x)->wID == id) return menu->items.Get(x);

  if (searchChildren) for (x = 0; x < menu->items.GetSize(); x ++)
  {
    if (menu->items.Get(x)->hSubMenu)
    {
      MENUITEMINFO *ret = GetMenuItemByID(menu->items.Get(x)->hSubMenu,id,true);
      if (ret) return ret;
    }
  }

  return 0;
}

static void swell_fillGtkMenu(HMENU menu, GtkWidget *gtk_menu, GCallback callback, gpointer user_data)
{
  // TODO: Implement bitmaps and item state

  // Update GTK menu from menu's WDL_PtrList of MENUITEMINFO elements
  for (int i = 0; i < menu->items.GetSize(); ++i)
  {
    MENUITEMINFO *mi = menu->items.Get(i);

    GtkWidget *gi = NULL;
    switch (mi->fType)
    {
    case MFT_SEPARATOR:
      gi = gtk_separator_menu_item_new();
      break;
    case MFT_STRING:
      if (mi->dwTypeData)
      {
	// Replace & for mnemonics with _
	size_t length = strlen(mi->dwTypeData);
	char *newstring = (char*)malloc(length + 1);
	for (size_t i = 0; i < length; ++i)
	  {
	    newstring[i] = (mi->dwTypeData[i] == '&') ? '_' : mi->dwTypeData[i];
	  }
	newstring[length] = '\0';

	if (mi->fState & MFS_CHECKED)
	{
	  gi = gtk_check_menu_item_new_with_mnemonic(newstring);
	  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gi), TRUE);
	}
	else
	  gi = gtk_menu_item_new_with_mnemonic(newstring);

	free(newstring);
      }
      else
      {
	gi = gtk_menu_item_new_with_label("");
      }
      break;
    }

    if (gi)
    {
      gtk_menu_shell_append(GTK_MENU_SHELL(gtk_menu), gi);

      if (mi->fState & MFS_DISABLED)
	gtk_widget_set_sensitive(gi, FALSE);

      if (mi->wID)
      {
	g_object_set_data(G_OBJECT(gi), "wID", GUINT_TO_POINTER(mi->wID));
	g_signal_connect(gi, "activate", callback, user_data);
      }

      if (mi->hSubMenu)
      {
	GtkWidget *submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(gi), submenu);
	swell_fillGtkMenu(mi->hSubMenu, submenu, callback, user_data);
      }
    }
  }
}

bool SetMenuItemText(HMENU hMenu, int idx, int flag, const char *text)
{
  MENUITEMINFO *item = hMenu ? ((flag & MF_BYPOSITION) ? hMenu->items.Get(idx) : GetMenuItemByID(hMenu,idx)) : NULL;
  if (!item) return false;

  item->fType = MFT_STRING;
  free(item->dwTypeData); // todo handle bitmap types
  item->dwTypeData=strdup(text?text:"");

  return true;
}

bool EnableMenuItem(HMENU hMenu, int idx, int en)
{
  MENUITEMINFO *item = hMenu ? ((en & MF_BYPOSITION) ? hMenu->items.Get(idx) : GetMenuItemByID(hMenu,idx)) : NULL;
  if (!item) return false;

  int mask = MF_ENABLED|MF_DISABLED|MF_GRAYED;
  item->fState &= ~mask;
  item->fState |= en&mask;

  return true;
}

bool CheckMenuItem(HMENU hMenu, int idx, int chk)
{
  MENUITEMINFO *item = hMenu ? ((chk & MF_BYPOSITION) ? hMenu->items.Get(idx) : GetMenuItemByID(hMenu,idx)) : NULL;
  if (!item) return false;

  int mask = MF_CHECKED;
  item->fState &= ~mask;
  item->fState |= chk&mask;

  return true;
}


HMENU SWELL_GetCurrentMenu()
{
  return NULL; // not osx
}
void SWELL_SetCurrentMenu(HMENU hmenu)
{
}


HMENU GetSubMenu(HMENU hMenu, int pos)
{
  MENUITEMINFO *item = hMenu ? hMenu->items.Get(pos) : NULL;
  if (item) return item->hSubMenu;
  return 0;
}

int GetMenuItemCount(HMENU hMenu)
{
  if (hMenu) return hMenu->items.GetSize();
  return 0;
}

int GetMenuItemID(HMENU hMenu, int pos)
{
  MENUITEMINFO *item = hMenu ? hMenu->items.Get(pos) : NULL;
  if (!item || item->hSubMenu) return -1;
  return item->wID;
}

bool SetMenuItemModifier(HMENU hMenu, int idx, int flag, int code, unsigned int mask)
{
  return false;
}

HMENU CreatePopupMenu()
{
  return new HMENU__;
}
HMENU CreatePopupMenuEx(const char *title)
{
  return CreatePopupMenu();
}

void DestroyMenu(HMENU hMenu)
{
  delete hMenu;
}

int AddMenuItem(HMENU hMenu, int pos, const char *name, int tagid)
{
  if (!hMenu) return -1;
  MENUITEMINFO *inf = (MENUITEMINFO*)calloc(1,sizeof(MENUITEMINFO));
  inf->wID = tagid;
  inf->dwTypeData = strdup(name?name:"");
  hMenu->items.Insert(pos,inf);
  return 0;
}

bool DeleteMenu(HMENU hMenu, int idx, int flag)
{
  if (!hMenu) return false;
  if (flag&MF_BYPOSITION)
  {
    if (hMenu->items.Get(idx))
    {
      hMenu->items.Delete(idx,true,HMENU__::freeMenuItem);
      return true;
    }
    return false;
  }
  else
  {
    int x;
    int cnt=0;
    for (x=0;x<hMenu->items.GetSize(); x ++)
    {
      if (!hMenu->items.Get(x)->hSubMenu && hMenu->items.Get(x)->wID == idx)
      {
        hMenu->items.Delete(x--,true,HMENU__::freeMenuItem);
        cnt++;
      }
    }
    if (!cnt)
    {
      for (x=0;x<hMenu->items.GetSize(); x ++)
      {
        if (hMenu->items.Get(x)->hSubMenu) cnt += DeleteMenu(hMenu->items.Get(x)->hSubMenu,idx,flag)?1:0;
      }
    }
    return !!cnt;
  }
}


BOOL SetMenuItemInfo(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return 0;
  MENUITEMINFO *item = byPos ? hMenu->items.Get(pos) : GetMenuItemByID(hMenu, pos, true);
  if (!item) return 0;

  if ((mi->fMask & MIIM_SUBMENU) && mi->hSubMenu != item->hSubMenu)
  {
    delete item->hSubMenu;
    item->hSubMenu = mi->hSubMenu;
  }
  if (mi->fMask & MIIM_TYPE)
  {
    free(item->dwTypeData); // todo handle bitmap types
    item->dwTypeData=0;
    if (mi->fType == MFT_STRING && mi->dwTypeData)
    {
      item->dwTypeData = strdup( mi->dwTypeData );
    }
    item->fType = mi->fType;
  }

  if (mi->fMask & MIIM_STATE) item->fState = mi->fState;
  if (mi->fMask & MIIM_ID) item->wID = mi->wID;
  if (mi->fMask & MIIM_DATA) item->dwItemData = mi->dwItemData;

  return true;
}

BOOL GetMenuItemInfo(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return 0;
  MENUITEMINFO *item = byPos ? hMenu->items.Get(pos) : GetMenuItemByID(hMenu, pos, true);
  if (!item) return 0;

  if (mi->fMask & MIIM_TYPE)
  {
    mi->fType = item->fType;
    if (item->fType == MFT_STRING && mi->dwTypeData && mi->cch)
    {
      lstrcpyn_safe(mi->dwTypeData,item->dwTypeData?item->dwTypeData:"",mi->cch);
    }
  }

  if (mi->fMask & MIIM_DATA) mi->dwItemData = item->dwItemData;
  if (mi->fMask & MIIM_STATE) mi->fState = item->fState;
  if (mi->fMask & MIIM_ID) mi->wID = item->wID;
  if (mi->fMask & MIIM_SUBMENU) mi->hSubMenu = item->hSubMenu;

  return 1;
}

void SWELL_InsertMenu(HMENU menu, int pos, int flag, int idx, const char *str)
{
  MENUITEMINFO mi={sizeof(mi),MIIM_ID|MIIM_STATE|MIIM_TYPE,MFT_STRING,
    (flag & ~MF_BYPOSITION),idx,NULL,NULL,NULL,0,(char *)str};
  InsertMenuItem(menu,pos,(flag&MF_BYPOSITION) ?  TRUE : FALSE, &mi);
}

void SWELL_InsertMenu(HMENU menu, int pos, unsigned int flag, UINT_PTR idx, const char *str)
{
  MENUITEMINFO mi={sizeof(mi),MIIM_ID|MIIM_STATE|MIIM_TYPE,MFT_STRING,
    (flag & ~MF_BYPOSITION),(flag&MF_POPUP) ? 0 : (int)idx,NULL,NULL,NULL,0,(char *)str};
  
  if (flag&MF_POPUP) 
  {
    mi.hSubMenu = (HMENU)idx;
    mi.fMask |= MIIM_SUBMENU;
    mi.fState &= ~MF_POPUP;
  }
  
  if (flag&MF_SEPARATOR)
  {
    mi.fMask=MIIM_TYPE;
    mi.fType=MFT_SEPARATOR;
    mi.fState &= ~MF_SEPARATOR;
  }
    
  InsertMenuItem(menu,pos,(flag&MF_BYPOSITION) ?  TRUE : FALSE, &mi);
}

void InsertMenuItem(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return;
  int ni=hMenu->items.GetSize();

  if (!byPos)
  {
    int x;
    for (x=0;x<ni && hMenu->items.Get(x)->wID != pos; x++);
    pos = x;
  }
  if (pos < 0 || pos > ni) pos=ni;

  MENUITEMINFO *inf = (MENUITEMINFO*)calloc(sizeof(MENUITEMINFO),1);
  inf->fType = mi->fType;
  if (mi->fType == MFT_STRING)
  {
    inf->dwTypeData = strdup(mi->dwTypeData?mi->dwTypeData:"");
  }
  else if (mi->fType == MFT_BITMAP)
  { // todo handle bitmap types
  }
  else if (mi->fType == MFT_SEPARATOR)
  {
  }
  if (mi->fMask&MIIM_SUBMENU) inf->hSubMenu = mi->hSubMenu;
  if (mi->fMask & MIIM_STATE) inf->fState = mi->fState;
  if (mi->fMask & MIIM_DATA) inf->dwItemData = mi->dwItemData;
  if (mi->fMask & MIIM_ID) inf->wID = mi->wID;

  hMenu->items.Insert(pos,inf);
}


void SWELL_SetMenuDestination(HMENU menu, HWND hwnd)
{
  // only needed for Cocoa
}

static void swell_gtkPopupMenuItemActivate(GtkMenuItem *menuitem, gpointer data)
{
  *((UINT*)data) = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "wID"));
}

static void swell_gtkPopupMenuClose(GtkWidget *menu, gpointer data)
{
  *((bool*)data) = false;
}

int TrackPopupMenu(HMENU hMenu, int flags, int xpos, int ypos, int resvd, HWND hwnd, const RECT *r)
{
  if (!hMenu) return 0;

  if (!hMenu->gtk_menu)
  {
    // Create GtkMenu for the first time
    hMenu->gtk_menu = gtk_menu_new();
  }

  GtkWidget *gtk_menu = hMenu->gtk_menu;
  // Remove all elements from GTK menu
  GList *children = gtk_container_get_children(GTK_CONTAINER(gtk_menu));
  for(GList *iter = children; iter != NULL; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  g_list_free(children);

  bool running = true;
  UINT selected_wID = 0;

  // Refill menu with items from WDL_PtrList
  swell_fillGtkMenu(hMenu, gtk_menu, G_CALLBACK(swell_gtkPopupMenuItemActivate), &selected_wID);
  gtk_widget_show_all(gtk_menu);

  g_signal_connect(gtk_menu, "hide", G_CALLBACK(swell_gtkPopupMenuClose), &running);

  // Show popup menu
  gtk_menu_popup(GTK_MENU(gtk_menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());

  // Wait until either an item has been selected or the popup menu has been closed
  while (running)
  {
    void SWELL_RunMessageLoop();
    SWELL_RunMessageLoop();
    Sleep(10);
  }

  if (!(flags&TPM_NONOTIFY))
    SendMessage(hwnd, WM_COMMAND, selected_wID, 0);

  return selected_wID;
}

void SWELL_Menu_AddMenuItem(HMENU hMenu, const char *name, int idx, unsigned int flags)
{
  MENUITEMINFO mi={sizeof(mi),MIIM_ID|MIIM_STATE|MIIM_TYPE,MFT_STRING,
    (flags)?MFS_GRAYED:0,idx,NULL,NULL,NULL,0,(char *)name};
  if (!name)
  {
    mi.fType = MFT_SEPARATOR;
    mi.fMask&=~(MIIM_STATE|MIIM_ID);
  }
  InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&mi);
}


SWELL_MenuResourceIndex *SWELL_curmodule_menuresource_head; // todo: move to per-module thingy

static SWELL_MenuResourceIndex *resById(SWELL_MenuResourceIndex *head, const char *resid)
{
  SWELL_MenuResourceIndex *p=head;
  while (p)
  {
    if (p->resid == resid) return p;
    p=p->_next;
  }
  return 0;
}

HMENU SWELL_LoadMenu(SWELL_MenuResourceIndex *head, const char *resid)
{
  SWELL_MenuResourceIndex *p;
  
  if (!(p=resById(head,resid))) return 0;
  HMENU hMenu=CreatePopupMenu();
  if (hMenu) p->createFunc(hMenu);
  return hMenu;
}

HMENU SWELL_DuplicateMenu(HMENU menu)
{
  if (!menu) return 0;
  return menu->Duplicate();
}

BOOL SetMenu(HWND hwnd, HMENU menu)
{
  if(!hwnd) return FALSE;

  hwnd->m_menu = menu;
  if (menu && !menu->gtk_menu)
  {
    // Create menu bar for the first time
    menu->gtk_menu = gtk_menu_bar_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), hwnd->m_menu->gtk_menu, FALSE, FALSE, 0);

    // Remove hwnd from old parent and put it into new box
    GtkWidget *parent = gtk_widget_get_parent(hwnd->m_oswindow);
    g_object_ref(hwnd->m_oswindow);
    gtk_container_remove(GTK_CONTAINER(parent), hwnd->m_oswindow);
    gtk_box_pack_start(GTK_BOX(box), hwnd->m_oswindow, TRUE, TRUE, 0);
    g_object_unref(hwnd->m_oswindow);

    // Put box into old parent of hwnd and show it
    gtk_container_add(GTK_CONTAINER(parent), box);
    gtk_widget_show_all(hwnd->m_menu->gtk_menu);
    gtk_widget_show(box);
  }
  // TODO: Reparent gtk menu bar if already present

  return TRUE;
}

HMENU GetMenu(HWND hwnd)
{
  if (!hwnd) return NULL;
  return hwnd->m_menu;
}

static void swell_gtkMenuBarItemActivate(GtkMenuItem *menuitem, gpointer data)
{
  SendMessage((HWND)data, WM_COMMAND,
	      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "wID")), 0);
}

void DrawMenuBar(HWND hwnd)
{
  if(!hwnd || !hwnd->m_menu) return;

  if (hwnd->m_menu->gtk_menu)
  {
    HMENU menu = hwnd->m_menu;
    GtkWidget *gtk_menu = menu->gtk_menu;
    // Remove all elements from GTK menu
    GList *children = gtk_container_get_children(GTK_CONTAINER(gtk_menu));
    for(GList *iter = children; iter != NULL; iter = g_list_next(iter))
      gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);

    // Refill menu with items from WDL_PtrList
    swell_fillGtkMenu(menu, gtk_menu, G_CALLBACK(swell_gtkMenuBarItemActivate), hwnd);
    gtk_widget_show_all(gtk_menu);
  }

  InvalidateRect(hwnd,NULL,FALSE);
}


// copied from swell-menu.mm, can have a common impl someday
int SWELL_GenerateMenuFromList(HMENU hMenu, const void *_list, int listsz)
{
  SWELL_MenuGen_Entry *list = (SWELL_MenuGen_Entry *)_list;
  const int l1=strlen(SWELL_MENUGEN_POPUP_PREFIX);
  while (listsz>0)
  {
    int cnt=1;
    if (!list->name) SWELL_Menu_AddMenuItem(hMenu,NULL,-1,0);
    else if (!strcmp(list->name,SWELL_MENUGEN_ENDPOPUP)) return list + 1 - (SWELL_MenuGen_Entry *)_list;
    else if (!strncmp(list->name,SWELL_MENUGEN_POPUP_PREFIX,l1)) 
    { 
      MENUITEMINFO mi={sizeof(mi),MIIM_SUBMENU|MIIM_STATE|MIIM_TYPE,MFT_STRING,0,0,CreatePopupMenuEx(list->name+l1),NULL,NULL,0,(char *)list->name+l1};
      cnt += SWELL_GenerateMenuFromList(mi.hSubMenu,list+1,listsz-1);
      InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&mi);
    }
    else SWELL_Menu_AddMenuItem(hMenu,list->name,list->idx,list->flags);

    list+=cnt;
    listsz -= cnt;
  }
}
#endif
