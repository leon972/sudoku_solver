/*

  Copyright (C) 2006 by Leonardo Berti
  programma solutore per Sudoku
  Interfaccia grafica Win32 

*/

#define WIN32_LEAN_AND_MEAN

#define IDD_DLGSUDOKU 1000
#define IDC_BTNVALIDATE 1001
#define IDC_BTNEXIT 1002
#define IDC_BTNSOLVE 1003
#define IDC_SBR1 1004
#define IDC_BTNALLOWED 1005
#define IDC_BTNHINT 1006
#define IDC_BOARD_BASE 2000
#define IDR_MENU 10000
#define IDM_FILE 10001
#define IDM_EMPTY 10002
//id per il menu dell'applicazione
#define IDM_SAVE 10004
#define IDM_LOAD 10005
#define IDM_ABOUT 10006
#define IDM_NEW 10007
#define IDM_SETTINGS 10008
#define IDM_GEN 10009
#define IDM_EASY 10010
#define IDM_MED 10011
#define IDM_DIFF 10012

#include <windows.h>
#include <commdlg.h> 
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <tchar.h>
#include <string.h>
#include "resource.h"
#include "sudoku.h"

//dimensioni finestra
#define WIN_WIDTH 544
#define WIN_HEIGHT 375
//altezza predefinita dei pulsanti
#define BTU_DEFHEIGHT 30

#define SUDOKU_SIZE MAX_ROW

#define EDITW 20 //larghezza celle
#define EDITSPACE 2 //spazio fra le caselle

//informazioni sullo stato grafico di un cella (colore di primo piano e sfondo)
typedef struct
{
	int state; //0=non evidenziata, 1 evidenziata modo 1 (suggerimento), 2 evidenziata modo 2 (errore) 
	HWND phwnd;

} CELL_UI_INFO,*CELL_UI_INFO_PTR;

//globals
static HWND _hwndBtuValidate=0;
static HWND _hwndBtuSolve=0;
static HWND _hwndBtuExit=0;

//static HWND _hwndBtuExit=0;
static HWND _hwndSbar=0;
static HMENU _hwndMenu=0;
static HMENU _hwndPopUpMenu=0;
static HMENU _hwndSettingMenu=0;
static HMENU _hwndGen=0;

//handle dei text control della tabella
static HWND _hwndCells[SUDOKU_SIZE][SUDOKU_SIZE];
//stato di ogni cella (0=cella fissa 1=cella riempita dall'utente)
static int _cellUiState[SUDOKU_SIZE][SUDOKU_SIZE];  

//stato di evidenziazione di una cella
static CELL_UI_INFO _cellUiInfo;

static HBRUSH hbrSbar=0;
static HBRUSH hbrHilight1=0;
static HBRUSH hbrHilight2=0;
static HBRUSH hbrWhite=0;
static COLORREF clTxBar=RGB(120,120,255);
static COLORREF clBkBar=RGB(0,0,60);
static COLORREF clFixed=RGB(0,0,0); //colore celle fisse
static COLORREF clInput=RGB(0,0,255); //celle riempite 
static COLORREF clHilight1=RGB(255,0,0); //celle evidenziate
static COLORREF clHilight2=RGB(255,255,255); //celle evidenziate (errore)
static HPEN _pen=0;
static HPEN _pen2=0;
static TCHAR szFileName[MAX_PATH]; //percorso completo del file restituito dalla common dialog
static TCHAR szFileTitle[MAX_PATH]; //nome del file senza percorso

static OPENFILENAME ofn;  //struttura usata dalla common dialog

static int _BoardX=0;
static int _BoardY=0;
static int _BoardW=0;
static int _BoardH=0;

static int _curLang=0; //lingua corrente (0=lingua di default)

HWND hwnd; //handler della finestra principale

//engine sudoku
static CSudokuEngine* _Sudoku=NULL;
//tabella con i simboli
static BOARD _board;
static BOARD _solvedBoard; //usata per i suggerimenti
static TCHAR _temp[80]; //general purpose

/*Acquisisce la tabella dall'interfaccia utente*/
void GrabBoard(BOARD board);


/* Visualizza la tabella di simboli sull'interfaccia grafica */
void DisplayBoard(BOARD board);

void UpdateDisplay(int engine_state,int v1,int v2);

int CDECL MsgOut(UINT uType,TCHAR *lpstrTitle,TCHAR *lpstrMsg,...);

void SetInitialCellsColors(BOARD board);

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);


/*  Make the class name into a global variable  */
char szClassName[ ] = "SudokuSolver";
WORD wNotify;

//Imposta la cella correntemente evidenziata
void SetHilight(int row,int col,int state)
{
	if (row>=0 && row<SUDOKU_SIZE && col>=0 && col<SUDOKU_SIZE)
	{
		_cellUiInfo.phwnd=_hwndCells[row][col];
		
		if (state>=0 && state<=2) _cellUiInfo.state=state;
		else _cellUiInfo.state=0;

		InvalidateRect(hwnd,NULL,FALSE);

	}
}



void InitializeCellsState(void)
{
	memset(_cellUiState,0,sizeof(int)*SUDOKU_SIZE*SUDOKU_SIZE);
}

void LoadSettings(void)
{
   //carica il settaggio per la lingua dal file config.dat
   FILE* f=NULL;

   f=fopen("config.dat","rt");

   if (f)
   {

	   char line[80];	 

	   fgets(line,80,f);
	   	   
	   if (strncmp(line,"lang=en",7)==0) _curLang=1; //inglese
	   else _curLang=0; //lingua default
	   
	   fclose(f);
   }


}

//localization
typedef struct
{
	TCHAR *msg;
	TCHAR *msg1;

} LOCALE_TABLE,*LOCALE_TABLE_PTR;


static LOCALE_TABLE g_Msg[]=
{
	TEXT("&Nuovo"),TEXT("&New"), //0
    TEXT("&Apri"),TEXT("&Open"), //1
	TEXT("Esci"),TEXT("Exit"), //2
	TEXT("Carica schema"),TEXT("Load puzzle"), //3
	TEXT("Salva schema corrente"),TEXT("Save current board"), //4
	TEXT("Risolto! (%d chiamte ricorsive)"),TEXT("Solved! (%d recursive calls)"), //5
	TEXT("Controlla validità"),TEXT("Check board"), //6
	TEXT("Risolvi"),TEXT("Solve"), //7
	TEXT("Valori ammessi"),TEXT("Allowed values"), //8
	TEXT("Vuoi interrompere l'elaborazione ?"),TEXT("Stop computing?"), //9
	TEXT("Errore"),TEXT("Error"), //10
	TEXT("Vuoi uscire?"),TEXT("Quit program?"),//11
	TEXT("Cancellare lo schema corrente?"),TEXT("Do you want to clear the current board?"), //12
	TEXT("Il file %s esiste già, vuoi sovrascriverlo?"),TEXT("File %s is already existing,overwrite?"), //13
	TEXT("Errore scrivendo sul file %s"),TEXT("Error writing on file %s"), //14
	TEXT("Il file non esiste"),TEXT("File doesn't exist"), //15
	TEXT("Errore leggendo i dati dal file %s"),TEXT("Error reading from file %s"),//16
	TEXT("Schema già risolto!"),TEXT("The puzzle is already solved!"), //17
	TEXT("Lo schema è valido!"),TEXT("This board is valid!"), //18
	TEXT("La casella selezionata è già riempita o non valida!"),TEXT("This cell is already filled or is not valid!"), //19
	TEXT("Nessun valore è ammesso per la cella selezionata!"),TEXT("There aren't allowed values for this cell!"), //20
	TEXT("I valori ammessi in questa casella sono: "),TEXT("Allowed values in this cell:"), //21
	TEXT("Selezionare una casella!"),TEXT("Please, select a cell first!"), //22
	TEXT("Interrompi"),TEXT("Stop"),//23
	TEXT("Lo schema NON è valido!"),TEXT("This board is not valid!"), //24
	TEXT("Non riesco a trovare la soluzione (err_id=%d)"),TEXT("Cannot find a solution! (err_id=%d)"), //25
	TEXT("soluzione in corso...(fn %d)"),TEXT("solving...(fn %d)"), //26
	TEXT("elaborazione interrotta"),TEXT("aborted"), //27
	TEXT("Impossibile dare un suggerimento!"),TEXT("Hint not available!"), //28
	TEXT("Suggerimento"),TEXT("Hint"), //29
	TEXT("Genera schema..."),TEXT("Create puzzle..."),//30
	TEXT("Facile"),TEXT("Easy"), //31
	TEXT("Medio"),TEXT("Moderate"),//32
	TEXT("Difficile"),TEXT("Difficult"), //33
	TEXT("Impossibile generare lo schema!"),TEXT("Cannot generate the puzzle!"), //34
	TEXT("Risolto! Trovata soluzione unica."),TEXT("Solved! Unique solution found."), //35
	TEXT("Puzzle casuale generato."),TEXT("Random puzzle generated."), //36

};

TCHAR* _tr(int strid)
{
	if (_curLang==1) return g_Msg[strid].msg1;
	else return g_Msg[strid].msg;
}

bool UiIsValid(BOARD board)
{
	int r,c;

	SetHilight(0,0,0);

	if (_Sudoku->isValid(board,&r,&c)) return true;
	else
	{
		MsgOut(MB_ICONEXCLAMATION,_tr(10),_tr(24));
		SetHilight(c-1,r-1,2);
		return false;
	}

}

//Da un suggerimento risolvendo la tabella corrente
void hint(BOARD board)
{

	GrabBoard(board);

	//controlla la validità
	if (!UiIsValid(board)) return;

	if (_Sudoku->isComplete(board))
	{
		if (UiIsValid(board))
		{
			//già risolto
			MsgOut(MB_ICONINFORMATION,"Hint",_tr(17));		
		}
		return;
	}	

	_Sudoku->copyBoard(_solvedBoard,board);

	int r=_Sudoku->solve(_solvedBoard);

	if (r!=SOLVE_OK)
	{
		MsgOut(MB_ICONEXCLAMATION,"Hint",_tr(28));
	}
	else
	{

		BOARDCELLS cells;

		//restituisce la lista delle celle con un numero di valori
        //possibili pari a valcount
        int cnt=_Sudoku->findCells(cells,board,-1);       
		int idx,r,c;

		if (cnt>0)
		{
			
			idx=rand()%cnt;

			r=cells[idx].row;
			c=cells[idx].col;

			board[r-1][c-1]=_solvedBoard[r-1][c-1];
			SetHilight(c-1,r-1,1);
			DisplayBoard(board);

		}
		else
		{
			MsgOut(MB_ICONEXCLAMATION,"Hint",_tr(28));			
		}

	}
	
}

//genera uno schema
//difficulty=0,1,2 

bool CreatePuzzle(BOARD board,int difficulty)
{	
	int tries=0;
	int ncells;
	int ncells1;
	int r,c;
	int n;
	int nloop=0;
	ROW row;                   

	_Sudoku->resetBoard(board);

	switch (difficulty)
	{	

		//moderate
	case 1:

		ncells=31;
		break;

		//difficult
	case 2:

		ncells=26;
		break;

    //easy
	default:

		ncells=38;
		break;

	}		

	//inizializza il generatore di numeri casuali
    srand((int)(GetTickCount()%0xFFFF));

	ncells1=10;

	//crea lo schema risolto
	while (ncells1>0)
	{			
		r=(rand()%MAX_ROW);
		c=(rand()%MAX_ROW);

		if (board[r][c]==0)
		{
			n=_Sudoku->findValues(row,r+1,c+1,board);

			if (n>0)
			{
				n=rand()%n;
				//assegna un valore casuale fra quelli ammissibili
				board[r][c]=row[n];				
				ncells1--;
			}
			else break;				

			if (++nloop>1000) break;
			
		}
	
	}
	
	//errore di generazione schema
	if (SOLVE_OK != _Sudoku->solve(board)) 
	{
		MsgOut(MB_ICONEXCLAMATION,"Sudoku Solver",_tr(34));
		return false;
	}

	ncells1=CELL_COUNT-ncells;

	nloop=0;

	while (ncells1>0)
	{
		r=(rand()%MAX_ROW);
		c=(rand()%MAX_ROW);

		if (board[r][c]!=0)
		{	

			_Sudoku->copyBoard(_solvedBoard,board);

			_solvedBoard[r][c]=0;

			if (SOLVE_OK == _Sudoku->solve(_solvedBoard))
			{
				if (_Sudoku->getTotCalls()==1)
				{
					board[r][c]=0;
					--ncells1;
				}
			}

		}

		if (++nloop>2000) break;

	}	

	if (ncells1<=0) 
	{
		//imposta i colori delle celle
		SetInitialCellsColors(_board);  
		DisplayBoard(board);
		//generato
		SetWindowText(_hwndSbar,_tr(36));
		return true; //ok
	}
	
	MsgOut(MB_ICONEXCLAMATION,"Sudoku Solver",_tr(34));
	return false;

}

//imposta i colori delle celle differenziando quelle fisse da quelle inseribili
void SetInitialCellsColors(BOARD board)
{
	for (int i=0;i<SUDOKU_SIZE;i++)
	{
		for (int j=0;j<SUDOKU_SIZE;j++)
		{
			if (board[i][j]!=0)
			{
				//nota: le coordinate fra edit box e strttura board sono invertite
				_cellUiState[j][i]=0; //cella fissa
			}
			else _cellUiState[j][i]=1; //celle da disegnare in blu (input utente)
		}
	}

}

/* Crea il menu in modo dinamico */
void CreateAppMenu(void)
{     
     _hwndMenu=CreateMenu();
     _hwndPopUpMenu=CreateMenu();
	 _hwndGen=CreateMenu();

	 AppendMenu(_hwndGen,MF_STRING,IDM_EASY,_tr(31));
	 AppendMenu(_hwndGen,MF_STRING,IDM_MED,_tr(32));
	 AppendMenu(_hwndGen,MF_STRING,IDM_DIFF,_tr(33));

     
     AppendMenu(_hwndPopUpMenu,MF_STRING,IDM_NEW,_tr(0)); //nuovo
     AppendMenu(_hwndPopUpMenu,MF_STRING,IDM_LOAD,_tr(3)); //carica schema
     AppendMenu(_hwndPopUpMenu,MF_STRING,IDM_SAVE,_tr(4)); //salva schema
	 AppendMenu(_hwndPopUpMenu,MF_POPUP,(UINT)_hwndGen,_tr(30)); //menu genera puzzle
//	 AppendMenu(_hwndPopUpMenu,MF_STRING,IDM_SETTINGS,"&Impostazioni...");
     AppendMenu(_hwndPopUpMenu,MF_STRING,IDM_ABOUT,"&About");
     AppendMenu(_hwndPopUpMenu,MF_SEPARATOR,0,NULL);
     AppendMenu(_hwndPopUpMenu,MF_STRING,IDC_BTNEXIT,_tr(2)); //esci     

	 
     AppendMenu(_hwndMenu,MF_POPUP,(UINT)_hwndPopUpMenu,"&File");  	 
		 
     
}

//Common Dialog:
//Inizializza la struttura usata sia per aprire che per salvare
BOOL PopFileInitialize(HWND hwnd)
{
	static TCHAR szFilter[]= TEXT ("Sudoku (*.SUD)\0*.sud\0");
	
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner=hwnd;
	ofn.hInstance=NULL;
	ofn.lpstrFilter=szFilter; //pair di filtri terminanti da NULL
	ofn.lpstrCustomFilter=NULL;
	ofn.nMaxCustFilter=0;
	ofn.nFilterIndex=0;    //indice del filtro corrente fra quelli definiti
	ofn.lpstrFile=NULL; //usato per inizializzare con un nome di file, quando si esce dal dialog questo buffer contiene il file scelto 
	ofn.nMaxFile=MAX_PATH;      //numero di byte (ASCII) o caratteri (UNICODE) del buffer pstrFile
	ofn.lpstrFileTitle=NULL; //punta il buffer che contiene il nome del file senza percorso ma con l'estensione
	ofn.nMaxFileTitle=MAX_PATH;       //dimemsione del buffer lpstrFileTitle
	ofn.lpstrInitialDir=NULL;
	ofn.lpstrTitle=NULL;               //titolo della common dialog	
	ofn.nFileOffset=0;
	ofn.nFileExtension=0;
	ofn.lpstrDefExt=TEXT("sud");
	ofn.lCustData=0L;
    ofn.lpfnHook=NULL;
	ofn.lpTemplateName=NULL;
	return TRUE;
}


//Finestra di dialogo 'Apri'
BOOL PopFileOpenDlg(HWND hwnd,PTSTR pstrFileName, PTSTR pstrTitleName)
{
	ofn.hwndOwner=hwnd;
	ofn.lpstrFile=pstrFileName;
	ofn.lpstrFileTitle=pstrTitleName;
	ofn.Flags=OFN_HIDEREADONLY | OFN_CREATEPROMPT;
	return GetOpenFileName(&ofn);

}

//Finsetra di dialogo 'Salva'
BOOL PopFileSaveDlg(HWND hwnd,PTSTR pstrFileName, PTSTR pstrTitleName)
{
	ofn.hwndOwner=hwnd;
	ofn.lpstrFile=pstrFileName;
	ofn.lpstrFileTitle=pstrTitleName;
	ofn.Flags=OFN_OVERWRITEPROMPT;
	return GetSaveFileName(&ofn);  

}

int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)
{
    
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */	 
    wincl.hIcon = LoadIcon (hThisInstance, MAKEINTRESOURCE(IDI_SUDOKU_ICON));
    wincl.hIconSm = NULL; //LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    //utilizza il colore della faccia dei pulsanti
    wincl.hbrBackground = (HBRUSH) (COLOR_BTNFACE+1);

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 1;

   
		//carica il settaggio (lingua)
    LoadSettings(); 

	//crea il menu
	CreateAppMenu();
    
    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
               0,                   /* Extended possibilites for variation */
               szClassName,         /* Classname */
               "SudokuSolver",       /* Title Text */
               WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX	, /* come un dialog che non puo' essere ridimensionato*/
               CW_USEDEFAULT,       /* Windows decides the position */
               CW_USEDEFAULT,       /* where the window ends up on the screen */
               WIN_WIDTH,                 /* The programs width */
               WIN_HEIGHT,                 /* and height in pixels */
               HWND_DESKTOP,        /* The window is a child-window to desktop */
               _hwndMenu,                /* Menu principale */
               hThisInstance,       /* Program Instance handler */
               NULL                 /* No Window Creation data */
           );


    //Inizializza le finestre di dialogo Apri e Salva
    PopFileInitialize(hwnd);
    
    /* Make the window visible on the screen */
    ShowWindow (hwnd, nFunsterStil);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}


/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HINSTANCE hInstance;
    static HWND hLastWnd;
    const int dbtu=120; //distanza dal bordo inf.
	const int btuw=110; //larghezza pulsanti
	int btux=5; //punto di partenza a sinsistra
    int res;  
	int i,j;
    bool b;
    HDC hdc;
    RECT rcClient;
    PAINTSTRUCT ps;

    switch (message)                  /* handle the messages */
    {
        case WM_CREATE:		

            hInstance=((LPCREATESTRUCT) lParam)->hInstance;

            //res=DialogBox(hInstance,MAKEINTRESOURCE(IDD_DLGSUDOKU),hwnd,SudokuDlgProc);
			//controlla validità
            _hwndBtuValidate=CreateWindow(TEXT("button"),_tr(6),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,btux,WIN_HEIGHT-dbtu,(UINT)(btuw*1.1f),BTU_DEFHEIGHT,hwnd,(HMENU)IDC_BTNVALIDATE,hInstance,NULL);
			//risolvi
			btux+=(int)(btuw*1.1f);
            _hwndBtuSolve=CreateWindow(TEXT("button"),_tr(7),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,btux,WIN_HEIGHT-dbtu,(UINT)(btuw*0.8f),BTU_DEFHEIGHT,hwnd,(HMENU)IDC_BTNSOLVE,hInstance,NULL);
            btux+=(int)(btuw*0.8f);
			//valori ammessi
			CreateWindow(TEXT("button"),_tr(8),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,btux,WIN_HEIGHT-dbtu,btuw,BTU_DEFHEIGHT,hwnd,(HMENU)IDC_BTNALLOWED,hInstance,NULL);
			btux+=btuw;
            //suggerimento
			CreateWindow(TEXT("button"),_tr(29),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,btux,WIN_HEIGHT-dbtu,btuw,BTU_DEFHEIGHT,hwnd,(HMENU)IDC_BTNHINT,hInstance,NULL);
			btux+=btuw;
            //uscita           
            _hwndBtuExit=CreateWindow(TEXT("button"),_tr(2),WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,btux,WIN_HEIGHT-dbtu,(UINT)(btuw*0.8f),BTU_DEFHEIGHT,hwnd,(HMENU)IDC_BTNEXIT,hInstance,NULL);
            //barra di stato
            _hwndSbar=CreateWindow(TEXT("static"),TEXT("sudoku solver"),WS_CHILD|WS_VISIBLE|SS_LEFT,2,WIN_HEIGHT-80,WIN_WIDTH-8,25,hwnd,(HMENU)IDC_SBR1,hInstance,NULL);

            //usato per il colore di background della status bar
            hbrSbar=(HBRUSH) CreateSolidBrush(clBkBar);
			//colori usati per evidenziare le celle
			hbrHilight1=(HBRUSH) CreateSolidBrush(RGB(255,255,0)); //giallo
			hbrHilight2=(HBRUSH) CreateSolidBrush(RGB(255,0,0)); //rosso
			hbrWhite=(HBRUSH) CreateSolidBrush(RGB(255,255,255)); //bianco

            //angolo sup. sinistro della tabella
            _BoardX=(WIN_WIDTH-SUDOKU_SIZE*(EDITW+EDITSPACE))/2;
            _BoardY=10;

            //larghezza ed altezza della tabella
            _BoardW=(EDITW+EDITSPACE)*SUDOKU_SIZE;
            _BoardH=(EDITW+EDITSPACE)*SUDOKU_SIZE;

            //resetta lo stato di evidenziazione
			memset(&_cellUiInfo,0,sizeof(CELL_UI_INFO));

            //tabella
            for (i=0;i<SUDOKU_SIZE;i++) //colonne
            {
                for (j=0;j<SUDOKU_SIZE;j++) //righe
                {
                    _hwndCells[i][j]=CreateWindow(TEXT("edit"),NULL,WS_CHILD|WS_VISIBLE|ES_CENTER,_BoardX+i*(EDITW+EDITSPACE),_BoardY+j*(EDITW+EDITSPACE),EDITW,EDITW,hwnd,(HMENU)(IDC_BOARD_BASE+j+i*SUDOKU_SIZE),hInstance,NULL);
                }
            }

            _pen=(HPEN)CreatePen(PS_SOLID,3,RGB(64,64,64)); //cornice
            _pen2=(HPEN)CreatePen(PS_SOLID,2,RGB(64,64,64)); //linee sottili

            //acquisisce l'istanza dell'engine
            _Sudoku=&CSudokuEngine::getInstance();
			//imposta la funzione callback che viene chiamata dall'engine
			//e che si occupa di aggiornare la barra di stato ed elaborare i messaggi del sistema 
			//operativo
			_Sudoku->pfnUpdateGUI=UpdateDisplay;
            _Sudoku->resetBoard(_board);
			SetInitialCellsColors(_board);
			

        break;
             
         //Imposta il colore d primo piano e di sfondo della status bar
         //questo messaggio viene inviato prima di disegnare i controlli static

        case WM_CTLCOLORSTATIC:

            res=GetWindowLong((HWND)lParam,GWL_ID);

            if (res==IDC_SBR1)
            {
                //colora in modo personalizzato la barra di stato
                SetBkColor((HDC)wParam,clBkBar);
                SetTextColor((HDC)wParam,clTxBar);
                return (LRESULT)hbrSbar;                
            }

            break;

			//elabora questo messaggio prima di diegnare le caselle EDIT,
			//serve a cambiare colore alla cella correntemente evidenziata
        case WM_CTLCOLOREDIT:
			{
				HWND hwndE=(HWND) lParam;

				//acquisisce l'id dell'edit box
    			res=GetWindowLong((HWND)lParam,GWL_ID);

				//deve risalire alle coordinate della cella
				res-=IDC_BOARD_BASE;
				
				int i,j;
				
				i=(int)(res / SUDOKU_SIZE);
				j=(int)(res % SUDOKU_SIZE);
				
				if (i>=0 && i<SUDOKU_SIZE && j>=0 && j<SUDOKU_SIZE)
				{
					switch (_cellUiState[i][j])
					{
					case 1:

						SetTextColor((HDC)wParam,clInput); //cella blu (inserita dall'utente)
						break;

					}
				}

				//evidenzia la cella se necessario
				if (hwndE==_cellUiInfo.phwnd)
				{			
					switch(_cellUiInfo.state)
					{
					case 1:

						//evidenza per suggerimento rosso su sfondo giallo
						SetTextColor((HDC)wParam,RGB(255,0,0));
						SetBkColor((HDC)wParam,RGB(255,255,0));
						//restituisce il pennello da usare per colorare lo sfondo
						//(se si restituisse NULL, colorerebbe solo lo sfondo intorno al testo)
						return (LRESULT) hbrHilight1; 
						

					case 2:

						//evidenzia per segnalare un errore
						SetTextColor((HDC)wParam,RGB(255,255,255));
						SetBkColor((HDC)wParam,RGB(255,0,0));
						//restituisce il pennello da usare per colorare lo sfondo
						//(se si restituisse NULL, colorerebbe solo lo sfondo intorno al testo)
						return (LRESULT) hbrHilight2; 
						
					default:
						
						SetBkColor((HDC)wParam,RGB(255,255,255));
						return (LRESULT) hbrWhite;
					
						
					}
				}

				return NULL;

			}

			break;

        case WM_SIZE:


            break;

        case WM_COMMAND:
            
            //acquisisce il codice di notifica
            wNotify=HIWORD(wParam);
            
            switch(wNotify)
            {
                case EN_SETFOCUS:
                     
                     //notifica che la casella di modifica ha preso l'input focus
                     //salva l'handle della casella
                     hLastWnd=(HWND)lParam;                                                            
                      
                     break;                  
            }
               
        //processa i vari comandi dell'applicazione
            switch(wParam)
            {
                case IDC_BTNEXIT:

					{

						int st=_Sudoku->getState();

						if (st==ENGST_SOLVING_STRATEGIES || st==ENSGT_SOLVING_BRUTE_FORCE || st==ENGST_BREAK)
						{
							//interrompere l'elaborazione
							 if (IDYES==MessageBox(hwnd,_tr(9),TEXT("SudokuSolver"),MB_ICONQUESTION|MB_YESNO))
							 {
								 _Sudoku->RequestBreak();
							 }		 

						}
						else
						 {

							//vuoi uscire?
							if (IDYES==MessageBox(hwnd,_tr(11),TEXT("SudokuSolver"),MB_ICONQUESTION|MB_YESNO))
							{					

								PostQuitMessage(0);
							}

						 }

					}

                break;
                
                case IDM_ABOUT:
                     
					MessageBox(hwnd,TEXT("SudokuSolver\n\rCode by Leonardo Berti (C) 2006"),TEXT("About"),MB_ICONINFORMATION);     
					break;
                
                case IDM_NEW:
                
					//pulire lo schema corrente?
                if (IDYES==MessageBox(hwnd,_tr(12),TEXT("Sudoku Solver"),MB_ICONQUESTION|MB_YESNO))
                {
                    _Sudoku->resetBoard(_board);
					
					//imposta i colori delle celle
        			SetInitialCellsColors(_board);

                    DisplayBoard(_board);                                                                         
                }          
                break;                     
                
                case IDM_SAVE:

                    if (PopFileSaveDlg(hwnd,szFileName,szFileTitle))
    			    {
                         
                        WIN32_FIND_DATA fnd;
                        HANDLE hFile;
                        BOOL bExists=FALSE;
                        int iRes=IDYES;
                        
                        //controlla se il file esiste
        				hFile=FindFirstFile(szFileName,&fnd);
        
        				bExists=(!(hFile == INVALID_HANDLE_VALUE));			 
        				
                        if (bExists)
                        {
							     //file già esistente
        						 iRes=MsgOut(MB_ICONQUESTION+MB_YESNO,TEXT("Sudoku Solver"),_tr(13),szFileName);
        
        				}
        				
        				if (iRes==IDYES)
        				{
                           FILE* f=NULL;             
                           if (!(f=fopen(szFileName,"wb"))) 
                           {
							   //errore scrittura
                               MsgOut(MB_ICONEXCLAMATION,TEXT("Errore"),_tr(14),szFileName);
                               break;                                                       
                           }         
                           
                           //acquisisce lo schema corrente
                           GrabBoard(_board);
                           
                           fwrite(_board,sizeof(int)*CELL_COUNT,1,f);
                           
                           fclose(f);                          
                        }			 
    					                                      
                    }                
                break;  
                
                case IDM_LOAD:
                
                if (PopFileOpenDlg(hwnd,szFileName,szFileTitle))
                {					 
                     BOOL bExists=FALSE;  
                     WIN32_FIND_DATA fnd;                                                             
                     //controlla se il file esiste
        			 HANDLE hFile=FindFirstFile(szFileName,&fnd);
        
        			 bExists=(!(hFile == INVALID_HANDLE_VALUE));			 
        			 
        			 if (!bExists)
        			 {
						 //il file non esiste
                         MessageBox(hwnd,_tr(15),_tr(10),MB_ICONEXCLAMATION);
                         break;
                     }         
                     
                     FILE* f=NULL;
                     
                     if (!(f=fopen(szFileName,"rb")))
                     {
                          MsgOut(MB_ICONEXCLAMATION,_tr(10),_tr(16),szFileName);
                          break;                                                       
                     }
                     
                     fread(_board,sizeof(int)*CELL_COUNT,1,f);

					 //imposta i colori delle celle
					 SetInitialCellsColors(_board);
                     
					 SetHilight(0,0,0);					 

                     DisplayBoard(_board);
                     
                     fclose(f);                                                   
                                                               
                                                               
                }         
                break;

                //Controlla la validità della tabella 
                case IDC_BTNVALIDATE:

                //acquisisce i valori dalla interfaccia grafica
                GrabBoard(_board);

                if (UiIsValid(_board))
                {
                    if (_Sudoku->isComplete(_board))
                    {
						//schema risolto
                        MessageBox(hwnd,_tr(17),TEXT("SudokuSolver"),MB_ICONINFORMATION);                                         
                    }                        
                    else
                    {
                        MessageBox(hwnd,_tr(18),TEXT("SudokuSolver"),MB_ICONINFORMATION);
                    }
                }

                break;
                
                //suggerisce la lista dei possibili valori per la casella corrente
                case IDC_BTNALLOWED:                     
                     
                     b=false;
                     
                     for (i=0;i<SUDOKU_SIZE;i++)
                     {
                         for (j=0;j<SUDOKU_SIZE;j++)
                         {
                             //MsgOut(0,"",TEXT("i=%d j=%d h=%X"),i,j,_hwndCells[i][j]);
                             if (hLastWnd==_hwndCells[i][j])
                             //if (i==2 && j==0)
                             {
                                b=true;                             
                                
                                ROW vals;
                                
                                GrabBoard(_board);
                                
                                if (UiIsValid(_board)) 
                                {
                                     int res=_Sudoku->CSudokuEngine::findValues(vals,j+1,i+1,_board);
                                     
									 //cella già riempita
                                     if (res<0) MessageBox(hwnd,_tr(19),TEXT("SudokuSolver"),MB_ICONEXCLAMATION);
									 //nessun valore ammesso
                                     else  if (res==0) MessageBox(hwnd,_tr(20),TEXT("SudokuSolver"),MB_ICONEXCLAMATION);
                                     else
                                     {
                                         TCHAR* szFormat=new TCHAR[1024];
                                         TCHAR szTmp[4];
                                         memset(szFormat,0,sizeof(TCHAR)*(1024));
                                         
                                         strcat(szFormat,_tr(21));
                                         
                                         for (int c=0;c<res;c++)
                                         {
                                             memset(szTmp,0,sizeof(TCHAR)*4);
                                             
                                             sprintf(szTmp,"%d,",vals[c]);
                                             
                                             strcat(szFormat,szTmp);                                             
                                          }                         
                                          
                                          MessageBox(hwnd,szFormat,TEXT("SudokuSolver"),MB_ICONINFORMATION);                                             
                                          
                                          delete[] szFormat; 
                                          
                                          break;                                                                                       
                                         
                                     }                                
                                }                                
                             }                                                          
                                                                        
                         }
                     }   
                     
					 //selezionare una casella
                     if (!b) MessageBox(hwnd,_tr(22),TEXT("SudokuSolver"),MB_ICONEXCLAMATION);                          
                                          
                     break;

					 //suggerisce un valore
					case IDC_BTNHINT:

						hint(_board);
						break;
                     
                     //risolve il sudoku
                     case IDC_BTNSOLVE:                      				
                         
						  SetHilight(0,0,0);
						 
                          GrabBoard(_board);

						  if (!UiIsValid(_board)) break;

					      if (_Sudoku->isComplete(_board))
						  {
							  //schema già risolto
							  MessageBox(hwnd,_tr(17),TEXT("Sudoku Solver"),MB_ICONINFORMATION);
						  }
						  else
						  {
                          
							  //interrompi
							  SetWindowText(_hwndBtuExit,_tr(23));

							  res=_Sudoku->solve(_board);

							  SetWindowText(_hwndBtuExit,_tr(2)); //esci
                          
							  if (res==SOLVE_OK)
							  {   
								  //puzzle risolto
								  DisplayBoard(_board);

								  if (_Sudoku->getTotCalls()==1)
								  {
									 //trovata soluzione unica
									 SetWindowText(_hwndSbar,_tr(35));
								  }
								  else
								  {
									  //aggiorna la status bar
									  memset(_temp,0,80*sizeof(TCHAR));
									  sprintf(_temp,_tr(5),_Sudoku->getTotCalls());
									  SetWindowText(_hwndSbar,_temp); //risolto!
								  }
							  }
							  else if (res==SOLVE_ERR_INVALID)
							  {
								  MsgOut(MB_ICONEXCLAMATION,_tr(10),_tr(24));//non valido 	 
							  }
							  else
							  {
								  //non risco a trovre una soluzione
								  MsgOut(MB_ICONEXCLAMATION,_tr(10),_tr(25),res);                              
								                                 
							  }

						  }
                          
                     break;

					 //crea dei puzzle casuali a soluzione unica
					 case IDM_EASY:
						 CreatePuzzle(_board,0);
						 break;
					 case IDM_MED:
						 CreatePuzzle(_board,1);
						 break;
					 case IDM_DIFF:
						 CreatePuzzle(_board,2);
						 break;

            }

        break;

        case WM_PAINT:

        	hdc=BeginPaint(hwnd,&ps);

			GetClientRect(hwnd,&rcClient);			
            
			SetBkMode(hdc,TRANSPARENT);

            SelectObject(hdc,_pen);
            //seleziona il pennello trasparente
            SelectObject(hdc,GetStockObject(NULL_BRUSH));

            Rectangle(hdc,_BoardX-EDITSPACE,_BoardY-EDITSPACE,_BoardX+_BoardW,_BoardY+_BoardH);

            res=(int)sqrt(SUDOKU_SIZE);
            {
                int s;

                //disegna le linee della griglia di separazione delle regioni
                SelectObject(hdc,_pen2);

                for (int i=1;i<res;i++)
                {
                    s=i*(EDITW+EDITSPACE)*res-(int)(EDITSPACE/2);
                    MoveToEx(hdc,s+_BoardX,_BoardY-2,NULL);
                    LineTo(hdc,s+_BoardX,_BoardY+_BoardH);
                    MoveToEx(hdc,_BoardX-2,_BoardY+s,NULL);
                    LineTo(hdc,_BoardX+_BoardW,_BoardY+s);
                }
            }

			EndPaint(hwnd,&ps);

        break;

		case WM_KEYDOWN:
		
			SetHilight(0,0,0);
			break;

        case WM_DESTROY:

            DeleteObject(hbrSbar);
			DeleteObject(hbrHilight1);
			DeleteObject(hbrHilight2);
			DeleteObject(hbrWhite);
            DeleteObject((HPEN)_pen);
            DeleteObject((HPEN)_pen2);
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */

        break;

        default:                      /* for messages that we don't deal with */
        return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}

/* Acquisisce la tabella di simboli dall'interfaccia grafica */
void GrabBoard(BOARD board)
{
    static TCHAR t[3];

    for (int i=0;i<SUDOKU_SIZE;i++) //colonna 
    {
        for (int j=0;j<SUDOKU_SIZE;j++) //riga
        {
            board[j][i]=0;

            memset(t,0,3);

			//_hwndCells[colonna][riga]

            GetWindowText(_hwndCells[i][j],t,2);

            if (strlen(t)>0)
            {
                board[j][i]=atoi(t);
            }

        }
    }

}

/* Visualizza la tabella di simboli sull'interfaccia grafica */
void DisplayBoard(BOARD board)
{
    static TCHAR t[3];

    for (int i=0;i<SUDOKU_SIZE;i++)
    {
        for (int j=0;j<SUDOKU_SIZE;j++)
        {
            if (board[j][i]>0 && board[j][i]<=SUDOKU_SIZE)
            {
                memset(t,0,3);
                sprintf(t,"%d",board[j][i]);
                SetWindowText(_hwndCells[i][j],t);
            }
            else SetWindowText(_hwndCells[i][j],TEXT(""));
        }
    }
}

//Funzione message box modificata in modo da consentire di output di testo formattato 
//in base ai parametri su un message box

int CDECL MsgOut(UINT uType,TCHAR *lpstrTitle,TCHAR *lpstrMsg,...)
{
	TCHAR szBuffer[2048]; //2k per il messaggio
	va_list pArgList;

	//Questa macro fa si che pArgList punti all'inizio del buffer
	//dei valori opzionali
	va_start(pArgList,lpstrMsg);
	
	//formatta e mette la stringa formattata nel buffer szBuffer
	_vsnprintf(szBuffer,sizeof(szBuffer) / sizeof(TCHAR) ,lpstrMsg,pArgList);
    
	va_end(pArgList);

	return MessageBox(NULL,szBuffer,lpstrTitle,uType);

}


//Funzione call back chiamata dall'engine (aggiorna il display ed elabora i messaggi del
//sistema operativo)

void UpdateDisplay(int engine_state,int v1,int v2)
{

	static MSG msg;
	static TCHAR szDisplay[60];

	//controlla se c'è un messaggio in attesa da elaborare da 
	//PM_REMOVE indica di eliminare il messaggio dalla coda
	if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
	{
		if (msg.message==WM_QUIT) _Sudoku->RequestBreak();

		//elabora il messagio del sistema operativo (esempio:l'utente sposta la finestra)
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		
	}
	else
	{
		//aggiorna il display;
		memset(szDisplay,0,40*sizeof(TCHAR));

		switch (engine_state)
		{
			//controlla lo stato dell'engine
		case ENGST_INITIALIZED:
			
			wsprintf(szDisplay,"%s",TEXT("init ok"));
			break;

		case ENGST_ERROR:
			
			wsprintf(szDisplay,"%s",_tr(10));
			break;

		case ENGST_SOLVING_STRATEGIES:
			
			//visualizza tra parentesi il numero di chiamate ricorsive
			wsprintf(szDisplay,_tr(26),v2,v1); 
			break;

		case ENSGT_SOLVING_BRUTE_FORCE:
		//non serve il brute force!	
		//	wsprintf(szDisplay,TEXT("soluzione con algoritmo brute-force (col=%d)"),v1);
			break;

		case ENSGT_SOLVED:

			//risolto
			wsprintf(szDisplay,_tr(5),_Sudoku->getTotCalls());		
			break;

		case ENGST_BREAK:

			wsprintf(szDisplay,"%s",_tr(27)); //elaborazione interrotta
			break;

		}

		//aggiorna la status bar
		SetWindowText(_hwndSbar,szDisplay);

	}//fine if

}
			
