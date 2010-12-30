
/*
Copyright (c) 2009-2010 Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <windows.h>
#include "..\common\common.hpp"
#include "..\common\pluginW789.hpp"
#include "PluginHeader.h"


struct PluginStartupInfo *InfoW789=NULL;
struct FarStandardFunctions *FSFW789=NULL;


void ProcessDragFrom789()
{
	if (!InfoW789 || !InfoW789->AdvControl)
		return;

	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	PanelInfo PInfo;
	WCHAR *szCurDir=gszDir1; szCurDir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	InfoW789->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, NULL, (LONG_PTR)&PInfo);
	if ((PInfo.PanelType == PTYPE_FILEPANEL || PInfo.PanelType == PTYPE_TREEPANEL) && PInfo.Visible)
	{
		InfoW789->Control(PANEL_ACTIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szCurDir);
		int nDirLen=0, nDirNoSlash=0;
		if (szCurDir[0])
		{
			nDirLen=lstrlen(szCurDir);
			if (nDirLen>0)
				if (szCurDir[nDirLen-1]!=L'\\')
					nDirNoSlash=1;
		}

		// ��� ������ �������������� ������, ��� ������������� �� ����� ��������
		OutDataAlloc(sizeof(int)+PInfo.SelectedItemsNumber*((MAX_PATH+2)+sizeof(int)));

		//Maximus5 - ����� ������ ��������
		int nNull=0; // ItemsCount
		//WriteFile(hPipe, &nNull, sizeof(int), &cout, NULL);
		OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		
		if (PInfo.SelectedItemsNumber<=0) {
			OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		} else {
			PluginPanelItem **pi = (PluginPanelItem**)calloc(PInfo.SelectedItemsNumber, sizeof(PluginPanelItem*));
			bool *bIsFull = (bool*)calloc(PInfo.SelectedItemsNumber, sizeof(bool));
			int ItemsCount=PInfo.SelectedItemsNumber, i;

			int nMaxLen=MAX_PATH+1, nWholeLen=1;

			// ������� ��������� ������������ ����� ������
			for (i=0;i<ItemsCount;i++)
			{
				size_t sz = InfoW789->Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, NULL);
				if (!sz)
					continue;
				pi[i] = (PluginPanelItem*)calloc(sz, 1); // ������ ������������ � ������
				if (!InfoW789->Control(PANEL_ACTIVE, FCTL_GETSELECTEDPANELITEM, i, (LONG_PTR)(pi[i])))
				{
					free(pi[i]); pi[i] = NULL;
					continue;
				}
				if (i == 0 
					&& ((pi[i]->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
					&& !wcscmp(pi[i]->FindData.lpwszFileName, L".."))
				{
					free(pi[i]); pi[i] = NULL;
					continue;
				}

				int nLen=nDirLen+nDirNoSlash;
				if ((pi[i]->FindData.lpwszFileName[0] == L'\\' && pi[i]->FindData.lpwszFileName[1] == L'\\') ||
				    (ISALPHA(pi[i]->FindData.lpwszFileName[0]) && pi[i]->FindData.lpwszFileName[1] == L':' && pi[i]->FindData.lpwszFileName[2] == L'\\'))
				    { nLen = 0; bIsFull[i] = TRUE; } // ��� ��� ������ ����!
				nLen += lstrlenW(pi[i]->FindData.lpwszFileName);
				if (nLen>nMaxLen)
					nMaxLen = nLen;
				nWholeLen += (nLen+1);
			}
			//WriteFile(hPipe, &nWholeLen, sizeof(int), &cout, NULL);
			OutDataWrite(&nWholeLen, sizeof(int));

			WCHAR* Path=new WCHAR[nMaxLen+1];

			for (i=0;i<ItemsCount;i++)
			{
				//WCHAR Path[MAX_PATH+1];
				//ZeroMemory(Path, MAX_PATH+1);
				//Maximus5 - ������ � ������ ����� � ������������ overflow
				//wsprintf(Path, L"%s\\%s", szCurDir, PInfo.SelectedItems[i]->FindData.lpwszFileName);
				Path[0]=0;

				if (!pi[i] || !pi[i]->FindData.lpwszFileName) continue; //���� ������� �������� �� �������

				int nLen=0;
				if (nDirLen>0 && !bIsFull[i]) {
					lstrcpy(Path, szCurDir);
					if (nDirNoSlash) {
						Path[nDirLen]=L'\\';
						Path[nDirLen+1]=0;
					}
					nLen = nDirLen+nDirNoSlash;
				}
				lstrcpy(Path+nLen, pi[i]->FindData.lpwszFileName);
				nLen += lstrlen(pi[i]->FindData.lpwszFileName);

				nLen++;
				//WriteFile(hPipe, &nLen, sizeof(int), &cout, NULL);
				OutDataWrite(&nLen, sizeof(int));
				//WriteFile(hPipe, Path, sizeof(WCHAR)*nLen, &cout, NULL);
				OutDataWrite(Path, sizeof(WCHAR)*nLen);
			}

			for (i=0;i<ItemsCount;i++)
			{
				if (pi[i]) free(pi[i]);
			}
			free ( pi ); pi = NULL;

			free(bIsFull);
			delete [] Path; Path=NULL;

			// ����� ������
			//WriteFile(hPipe, &nNull, sizeof(int), &cout, NULL);
			OutDataWrite(&nNull, sizeof(int));
		}
	}
	else
	{
		int ItemsCount=0;
		OutDataWrite(&ItemsCount, sizeof(int));
		OutDataWrite(&ItemsCount, sizeof(int)); // ����� �������
	}
	//free(szCurDir);
}

void ProcessDragTo789()
{
	if (!InfoW789 || !InfoW789->AdvControl)
		return;

	WindowInfo WInfo;				
    WInfo.Pos=0;
	InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	if (!WInfo.Current)
	{
		//InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		if (gpCmdRet==NULL)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}
	//InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
	
	PanelInfo PAInfo, PPInfo;
	ForwardedPanelInfo *pfpi=NULL;
	int nStructSize = sizeof(ForwardedPanelInfo)+4; // ����� �������� �� ����� �����
	//ZeroMemory(&fpi, sizeof(fpi));
	BOOL lbAOK=FALSE, lbPOK=FALSE;
	WCHAR *szPDir=gszDir1; szPDir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	WCHAR *szADir=gszDir2; szADir[0]=0; //(WCHAR*)calloc(0x400,sizeof(WCHAR));
	
	//if (!(lbAOK=InfoW789->Control(PANEL_ACTIVE, FCTL_GETPANELSHORTINFO, &PAInfo)))
	lbAOK=InfoW789->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PAInfo) &&
		  InfoW789->Control(PANEL_ACTIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szADir);
	if (lbAOK && szADir)
		nStructSize += (lstrlen(szADir))*sizeof(WCHAR);

	lbPOK=InfoW789->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&PPInfo) &&
		  InfoW789->Control(PANEL_PASSIVE, FCTL_GETCURRENTDIRECTORY, 0x400, (LONG_PTR)szPDir);
	if (lbPOK && szPDir)
		nStructSize += (lstrlen(szPDir))*sizeof(WCHAR); // ������ WCHAR! �� TCHAR

	pfpi = (ForwardedPanelInfo*)calloc(nStructSize,1);
	if (!pfpi) {
		int ItemsCount=0;
		//WriteFile(hPipe, &ItemsCount, sizeof(int), &cout, NULL);				
		if (gpCmdRet==NULL)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}


	pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
	pfpi->pszActivePath = (WCHAR*)(((char*)pfpi)+pfpi->ActivePathShift);

	pfpi->PassivePathShift = pfpi->ActivePathShift+2; // ���� ActivePath ���������� - ��������

	if (lbAOK)
	{
		pfpi->ActiveRect=PAInfo.PanelRect;
		if (!PAInfo.Plugin && (PAInfo.PanelType == PTYPE_FILEPANEL || PAInfo.PanelType == PTYPE_TREEPANEL) && PAInfo.Visible)
		{
			if (szADir[0]) {
				lstrcpyW(pfpi->pszActivePath, szADir);
				pfpi->PassivePathShift += lstrlenW(pfpi->pszActivePath)*2;
			}
		}
	}

	pfpi->pszPassivePath = (WCHAR*)(((char*)pfpi)+pfpi->PassivePathShift);
	if (lbPOK)
	{
		pfpi->PassiveRect=PPInfo.PanelRect;
		if (!PPInfo.Plugin && (PPInfo.PanelType == PTYPE_FILEPANEL || PPInfo.PanelType == PTYPE_TREEPANEL) && PPInfo.Visible)
		{
			if (szPDir[0])
				lstrcpyW(pfpi->pszPassivePath, szPDir);
		}
	}

	// ����������, ��������� ����������
	//WriteFile(hPipe, &nStructSize, sizeof(nStructSize), &cout, NULL);
	//WriteFile(hPipe, pfpi, nStructSize, &cout, NULL);
	if (gpCmdRet==NULL)
			OutDataAlloc(nStructSize+4);
	OutDataWrite(&nStructSize, sizeof(nStructSize));
	OutDataWrite(pfpi, nStructSize);

	free(pfpi); pfpi=NULL;
}

void SetStartupInfoW789(void *aInfo)
{
	::InfoW789 = (PluginStartupInfo*)calloc(sizeof(PluginStartupInfo),1);
	::FSFW789 = (FarStandardFunctions*)calloc(sizeof(FarStandardFunctions),1);
	if (::InfoW789 == NULL || ::FSFW789 == NULL)
		return;
	*::InfoW789 = *((struct PluginStartupInfo*)aInfo);
	*::FSFW789 = *((struct PluginStartupInfo*)aInfo)->FSF;
	::InfoW789->FSF = ::FSFW789;

	lstrcpynW(gszRootKey, InfoW789->RootKey, MAX_PATH);
	WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	if (*pszSlash == L'\\') *(pszSlash--) = 0;
	while (pszSlash>gszRootKey && *pszSlash!=L'\\') pszSlash--;
	*pszSlash = 0;

	
	/*if (!FarHwnd)
		InitHWND((HWND)InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_GETFARHWND, 0));*/
}

DWORD GetEditorModifiedState789()
{
	EditorInfo ei;
	InfoW789->EditorControl(ECTL_GETINFO, &ei);

	#ifdef SHOW_DEBUG_EVENTS
		char szDbg[255];
		wsprintfA(szDbg, "Editor:State=%i\n", ei.CurState);
		OutputDebugStringA(szDbg);
	#endif

	// ���� �� ��������, �� ��� �� �������������
	DWORD currentModifiedState = ((ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED) ? 1 : 0;

	InfoW789->EditorControl(ECTL_FREEINFO, &ei);

	return currentModifiedState;
}


extern int lastModifiedStateW;
// watch non-modified -> modified editor status change
int ProcessEditorInputW789(LPCVOID aRec)
{
	if (!InfoW789)
		return 0;

	const INPUT_RECORD *Rec = (const INPUT_RECORD*)aRec;
	// only key events with virtual codes > 0 are likely to cause status change (?)
	if (!gbRequestUpdateTabs && (Rec->EventType & 0xFF) == KEY_EVENT 
		&& (Rec->Event.KeyEvent.wVirtualKeyCode || Rec->Event.KeyEvent.wVirtualScanCode || Rec->Event.KeyEvent.uChar.UnicodeChar)
		&& Rec->Event.KeyEvent.bKeyDown)
	{
		//if (!gbRequestUpdateTabs)
		gbNeedPostEditCheck = TRUE;
	}

	return 0;
}

bool UpdateConEmuTabsW789(int anEvent, bool losingFocus, bool editorSave, void* Param/*=NULL*/)
{
	if (!InfoW789 || !InfoW789->AdvControl)
		return false;

    BOOL lbCh = FALSE; // ���� ���� ���������
	WindowInfo WInfo;

	int windowCount = (int)InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	lbCh = (lastWindowCount != windowCount);
	
	if (!CreateTabs ( windowCount ))
		return false;

	EditorInfo ei = {0}; BOOL bEditorRetrieved = FALSE;
	if (editorSave)
	{
		InfoW789->EditorControl(ECTL_GETINFO, &ei);
		bEditorRetrieved = TRUE;
	}

	ViewerInfo vi = {sizeof(ViewerInfo)};
	if (anEvent == 206)
	{
		if (Param)
			vi.ViewerID = *(int*)Param;
		InfoW789->ViewerControl(VCTL_GETINFO, &vi);
	}

	int tabCount = 0;
	BOOL lbActiveFound = FALSE;
	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			{
				if (WInfo.Current) lbActiveFound = TRUE;
				PRAGMA_ERROR("�� ������������ ViewerID");
				lbCh |= AddTab(tabCount, losingFocus, editorSave, 
					WInfo.Type, WInfo.Name, editorSave ? ei.FileName : NULL, 
					WInfo.Current, WInfo.Modified, 0);
				//if (WInfo.Type == WTYPE_EDITOR && WInfo.Current) //2009-08-17
				//	lastModifiedStateW = WInfo.Modified;
			}
			InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_FREEWINDOWINFO, (void*)&WInfo);
		}
	}
	
	// Viewer � FAR 2 build 9xx �� �������� � ������ ���� ��� ������� VE_GOTFOCUS
	if (!losingFocus && !editorSave && tabCount == 0 && anEvent == 206)
	{
		lbCh |= AddTab(tabCount, losingFocus, editorSave, 
			WTYPE_VIEWER, vi.FileName, NULL, 
			1, 0, vi.ViewerID);
	}

	// ������ ����� ��� ��������� �������� (��� ������?)
	if (!lbActiveFound && !losingFocus)
	{
		if (!bEditorRetrieved) // ���� ���������� � ��������� ��� �� ��������
		{
			InfoW789->EditorControl(ECTL_GETINFO, &ei);
			bEditorRetrieved = TRUE;
		}
		if (ei.CurState)
		{
			tabCount = 0;
			lbCh |= AddTab(tabCount, losingFocus, editorSave, 
				WTYPE_EDITOR, ei.FileName, NULL, 
				1, (ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED, ei.EditorID);
			//lastModifiedStateW = ei.CurState == ECSTATE_MODIFIED;
		}
	}

	if (bEditorRetrieved) 
		InfoW789->EditorControl(ECTL_FREEINFO, &ei);

	// 101224 - ����� ��������� ����������!
	gpTabs->Tabs.nTabCount = tabCount;

	//// 2009-08-17
	//if (gbHandleOneRedraw && /*gbHandleOneRedrawCh &&*/ lbCh) {
	//	gbHandleOneRedraw = false;
	//	gbHandleOneRedrawCh = false;
	//}

#ifdef _DEBUG
	//WCHAR szDbg[128]; wsprintfW(szDbg, L"Event: %i, count %i\n", anEvent, tabCount);
	//OutputDebugStringW(szDbg);
#endif

	//SendTabs(tabCount, lbCh && (gnReqCommand==(DWORD)-1));

	return lbCh;
}

void ExitFARW789(void)
{
	if (InfoW789) {
		free(InfoW789);
		InfoW789=NULL;
	}
	if (FSFW789) {
		free(FSFW789);
		FSFW789=NULL;
	}
}

int ShowMessage789(int aiMsg, int aiButtons)
{
	if (!InfoW789 || !InfoW789->Message || !InfoW789->GetMsg)
		return -1;
	return InfoW789->Message(InfoW789->ModuleNumber, FMSG_ALLINONE|FMSG_MB_OK|FMSG_WARNING, NULL, 
		(const wchar_t * const *)InfoW789->GetMsg(InfoW789->ModuleNumber,aiMsg), 0, aiButtons);
}

LPCWSTR GetMsg789(int aiMsg)
{
	if (!InfoW789 || !InfoW789->GetMsg)
		return L"";
	return InfoW789->GetMsg(InfoW789->ModuleNumber,aiMsg);
}

void ReloadMacro789()
{
	if (!InfoW789 || !InfoW789->AdvControl)
		return;

	ActlKeyMacro command;
	command.Command=MCMD_LOADALL;
	InfoW789->AdvControl(InfoW789->ModuleNumber,ACTL_KEYMACRO,&command);
}

void SetWindow789(int nTab)
{
	if (!InfoW789 || !InfoW789->AdvControl)
		return;

	if (InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)nTab))
		InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_COMMIT, 0);
}

void PostMacro789(wchar_t* asMacro)
{
	if (!InfoW789 || !InfoW789->AdvControl)
		return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // �� ��������� - ����� �� ����� ��������
	if (*asMacro == L'@' && asMacro[1] && asMacro[1] != L' ') {
		mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
		asMacro ++;
	}
	mcr.Param.PlainText.SequenceText = asMacro;
	InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
}

int ShowPluginMenu789()
{
	if (!InfoW789)
		return -1;

	FarMenuItemEx items[] = {
		{ConEmuHwnd ? MIF_SELECTED : MIF_DISABLE,  InfoW789->GetMsg(InfoW789->ModuleNumber,CEMenuEditOutput)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW789->GetMsg(InfoW789->ModuleNumber,CEMenuViewOutput)},
		{MIF_SEPARATOR},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW789->GetMsg(InfoW789->ModuleNumber,CEMenuShowHideTabs)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW789->GetMsg(InfoW789->ModuleNumber,CEMenuNextTab)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW789->GetMsg(InfoW789->ModuleNumber,CEMenuPrevTab)},
		{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW789->GetMsg(InfoW789->ModuleNumber,CEMenuCommitTab)},
		{MIF_SEPARATOR},
		{ConEmuHwnd||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED,  InfoW789->GetMsg(InfoW789->ModuleNumber,CEMenuAttach)},
		{MIF_SEPARATOR},
		//#ifdef _DEBUG
		//{0, L"&~. Raise exception"},
		//#endif
		{IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0,    InfoW789->GetMsg(InfoW789->ModuleNumber,CEMenuDebug)}
	};
	int nCount = sizeof(items)/sizeof(items[0]);

	int nRc = InfoW789->Menu(InfoW789->ModuleNumber, -1,-1, 0, 
		FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
		InfoW789->GetMsg(InfoW789->ModuleNumber,CEPluginName),
		NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);

	#ifdef _DEBUG
	if (nRc == (nCount - 2))
	{
		// ������� ���������� ��� �������� ���������
		LPVOID ptrSrc;
		wchar_t szDst[MAX_PATH];
		ptrSrc = NULL;
		memmove(szDst, ptrSrc, sizeof(szDst));
	}
	#endif

	return nRc;
}

BOOL EditOutput789(LPCWSTR asFileName, BOOL abView)
{
	if (!InfoW789)
		return FALSE;

	BOOL lbRc = FALSE;
	if (!abView) {
		int iRc =
			InfoW789->Editor(asFileName, InfoW789->GetMsg(InfoW789->ModuleNumber,CEConsoleOutput), 0,0,-1,-1, 
			EF_NONMODAL|EF_IMMEDIATERETURN|EF_DELETEONLYFILEONCLOSE|EF_ENABLE_F6|EF_DISABLEHISTORY,
			0, 1, 1200);
		lbRc = (iRc != EEC_OPEN_ERROR);
	} else {
		#ifdef _DEBUG
		int iRc =
		#endif
			InfoW789->Viewer(asFileName, InfoW789->GetMsg(InfoW789->ModuleNumber,CEConsoleOutput), 0,0,-1,-1, 
			VF_NONMODAL|VF_IMMEDIATERETURN|VF_DELETEONLYFILEONCLOSE|VF_ENABLE_F6|VF_DISABLEHISTORY,
			1200);
		lbRc = TRUE;
	}

	return lbRc;
}

BOOL CallSynchro789(SynchroArg *Param, DWORD nTimeout /*= 10000*/)
{
	return FALSE;
}

BOOL IsMacroActive789()
{
	if (!InfoW789) return FALSE;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_KEYMACRO, &akm);
	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;
	return TRUE;
}

void RedrawAll789()
{
	if (!InfoW789) return;
	InfoW789->AdvControl(InfoW789->ModuleNumber, ACTL_REDRAWALL, NULL);
}

bool LoadPlugin789(wchar_t* pszPluginPath)
{
	if (!InfoW789) return false;
	InfoW789->PluginsControl(INVALID_HANDLE_VALUE,PCTL_LOADPLUGIN,PLT_PATH,(LONG_PTR)pszPluginPath);
	return true;
}
