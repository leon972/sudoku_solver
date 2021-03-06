/*

SudokuEngine 
engine per la soluzione del Sudoku
Code by Leonardo Berti (c) 2006

--
Questo engine è indipendente dall'interfaccia grafica.
--

*/

#include <math.h>

#ifndef _SUDOKU_ENGINE_
#define _SUDOKU_ENGINE_

//numero di caselle per lato (nota bene: questo valore deve avere come
//radice quadrata un numero intero)
//ogni regione è un quadrato di sqrt(MAX_ROW) caselle)
//(E' stato testato solo per tabelle di 9 celle di lato)

#define MAX_ROW 9
//numero di caselle nella tabella
#define CELL_COUNT MAX_ROW*MAX_ROW

#ifndef NULL
#define NULL 0L
#endif

#define SOLVE_OK 0
#define SOLVE_ERR_STACK -1
#define SOLVE_ERR_INVALID -2
#define SOLVE_ERR_ALGO -3
#define SOLVE_ERR_IMPOSSIBLE -4
#define SOLVE_ERR_CALLS -5
#define SOLVE_ERR_BREAK -6 

//una riga o colonna
typedef int ROW[MAX_ROW];

//tabella BOARD[riga][colonna]
typedef int BOARD[MAX_ROW][MAX_ROW];

typedef struct CELL_POS_TAG
{
    int row; //riga (hanno base 1)
    int col; //colonna 
    ROW admValues; //valori ammesi per questa cella
    int admCount; //numero di valori possibili

} CELL_POS,*CELL_POS_PTR;

typedef enum 
{

	ENGST_INITIALIZED=0, //inizializzato 
	ENGST_ERROR=1, //si è verificato un errore durante la soluzione
	ENGST_SOLVING_STRATEGIES=2, //sta risolvendo con l'algoritmo standard
    ENSGT_SOLVING_BRUTE_FORCE=4, //sta risolvendo con l'algoritmo brute force
	ENSGT_SOLVED=8, //tabella risolta
	ENGST_BREAK=16  //elaborazione interrotta 

} ENGINE_STATE;

//un vettore con le posizioni delle celle della tabella
typedef CELL_POS BOARDCELLS[CELL_COUNT];

class CSudokuEngine {

    private:
        
		int _rside; //numero di celle per lato di una regione (è sqrt(MAX_ROW);
        int _stackCnt; 
		int _totCall;
		int _maxCalls; //numero massimo di chiamate ricorsive
		int _maxBifurcations; //numero massimo di bifircazioni
		int _state;
		bool _breakRequest; //è stato chiesto di interrompere l'elaborazione
        
        BOARD _tmpBoard;
       
         
        CSudokuEngine(void);
        //è un singleton
        static CSudokuEngine _instance;

        static ROW _sample_row;

		//nota: per sequenza

        //riempie la sequenza missing con i numeri mancanti in row
        //restituisce il numero di numeri mancanti
        int findMissing(ROW missing,ROW row);

        //Indica se la sequenza è valida (anche se è incompleta)
        bool isValid(ROW row,int* err_index);

        //Indica se la sequenza è tutta riempita, rende true anche se
        //non è valida
        bool isComplete(ROW row);

        //riempie la sequenza con tutti i simboli
        void createSampleRow(ROW dest);

        //Restituisce la riga della tabella.
        //la prima riga ha indice 1
        void getRow(ROW row,BOARD board,int rowindex);

        //Restituisce la colonna della tabella
        //la prima colonna ha indice 1
        void getCol(ROW col,BOARD board,int colindex);

        //Restituisce una regione della tabella
        //rgx indica la colonna della regione (la prima a sx è la 1)
        //rgy indica la riga della regione 

        void getRegion(ROW region,BOARD board,int rgx,int rgy);
        
        //restituisce l'indice della regione di una casella
        //row e col hanno base 1 cosi' come rgx e rgy
        void getRegionIndex(int row,int col,int* rgx,int* rgy);
        
        //restituisce in res valori possibili per la casella alla riga row e colonna col      
        int findValues(ROW res,int row,int col,BOARD board,bool checkBoard);        
        
        //Controlla le coppie e sequenze univoche di valori ammessi 
        //se ad esempio per due celle di una riga sono possibili solo i valori 3 e 4
        //e per un'altra cella della stessa riga sono possibili 3,4,7 allora per la tarza cella sarà possibile solo il valore 7

        int stripSequence(BOARDCELLS cells,int cellBound);        

		//elimina gli "hidden single"
		void stripHiddenSingle(BOARDCELLS cells,int cellbound);

		//trova tutti i valori che hanno una sola cella possibile su una certa riga o colonna
		void findSingles(BOARDCELLS cells,int cellbound);

		//valori univoci regione per regione
		void findSingleInRegions(BOARDCELLS cells,int cellbound);
        
	    bool getCell(CELL_POS_PTR cell,int row,int col,BOARDCELLS cells);

		findBestCells(BOARDCELLS cells,int cellbound);

		//risolve il sudoku
        //board=tabella parzialmente riempita da risolvere
        //nBifurcations=numero massimo di chiamate ricorsive consentite
        //bStep=esegue un solo step 
        //restituisce:
        // SOLVE_OK se riesca a risolvere il sudoku
        // SOLVE_ERR_STACK non riesce per mancanza di spazio nello stack
        // SOLVE_ERR_INVALID errore interno è arrivato ad una tabella non valida
        // SOLVE_ERR_ALGO non riesce a risolvere per limitazione dell'algoritmo
        // SOLVE_ERR_IMPOSSIBLE irrisolvibile
                      
        int solve(BOARD board,bool bStep);
		
		void subtractValues(CELL_POS_PTR p1,CELL_POS_PTR p2);

		void subtractSingleValue(CELL_POS_PTR p,int value);

	    void resetRow(ROW row); 		
	
    public:
        
       
        void resetBoard(BOARD board);

        //restituisce un'istanza dell'engine (è un singleton)
        static CSudokuEngine& getInstance(void);

        //restituisce true se la tabella è valida anche se incompleta
		//err_row ed err_col hanno base 1 e restituiscono la riga e colonna della cella non valida
        bool isValid(BOARD board,int* err_row,int* err_col);

        //restituisce true se la tabella è completa anche se non valida
        bool isComplete(BOARD board);

        //restituisce il numero di celle per lato della regione
        int getRegionSize(void);
        
        //restituisce in res valori possibili per la casella alla riga row e colonna col
        //ed esegue il controllo della validità della tabella
        int findValues(ROW res,int row,int col,BOARD board);

        //cerca di trovare la soluzione
        int solve(BOARD board);

		//copia uno schema su un altro
		void copyBoard(BOARD dest,BOARD src);      
        
        /* Restituisce la soluzione temporanea (incompleta) */
        void getTempBoard(BOARD board);

        //Restituisce il numero e la posizione delle caselle non valide
        //si applica anche a una tabella incompleta
        int getInvalidCells(CELL_POS_PTR cells,BOARD board);
        //imposta numero massimo di chiamate ricorsive 
		bool setMaxCalls(int n);
        //imposta il massimo numero di biforcazioni
		bool setMaxBifurcations(int n);

		int getMaxCalls(void);

		int getTotCalls(void);

		int getMaxBifurcations(void);

	    //restituisce la lista delle celle con un numero di valori
        //possibili pari a valcount
        int findCells(BOARDCELLS cells,BOARD board,int valcount);       

        //funzione call back chiamata dalla funzione di soluzione per aggiornare l'interfaccia 
		//grafica (serve ad esempio a permettere l'elaborazione dei messaggi di sistema per non bloccare 
        //state=stato attuale dell'engine (vedere costanti ENGST_,
		//val1=restituisce il numero di biforcazioni attuali o il numero di riga o colonna se l'algoritmo è il brute force
		//val2=numero di chiamate alla funzione di soluzione dello schema
		void (*pfnUpdateGUI)(int state,int val1,int val2);

		//chiede di interrompere l'elaborazione
		void RequestBreak(void);

		int getState(void);
};

#endif

