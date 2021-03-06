/*

SudokuEngine 
engine per la soluzione del Sudoku
Code by Leonardo Berti (c) 2006

-- Questo engine è indipendente dall'interfaccia grafica --

*/

#include <math.h>
#include <mem.h>
#include "sudoku.h"

CSudokuEngine::CSudokuEngine()
{
    for (int i=1;i<=MAX_ROW;i++) CSudokuEngine::_sample_row[i-1]=1;
    resetBoard(_tmpBoard);
	_stackCnt=_totCall=0;
	_maxCalls=15000; //numero massimo di chiamate ricorsive
	_maxBifurcations=30; //numero massimo di bifircazioni       
	_breakRequest=false;
	_state=ENGST_INITIALIZED;
	_rside=(int)sqrt(MAX_ROW); //ordine (sqrt(lato))

}

CSudokuEngine& CSudokuEngine::getInstance(void)
{
    return CSudokuEngine::_instance;
}

void CSudokuEngine::resetRow(ROW row)
{
    for (int i=0;i<MAX_ROW;i++)
    {
        row[i]=0;
    }
}

void CSudokuEngine::getTempBoard(BOARD board)
{
   memcpy(board,_tmpBoard,sizeof(int)*CELL_COUNT);    
}

void CSudokuEngine::resetBoard(BOARD board)
{
    for (int i=0;i<MAX_ROW;i++) resetRow(board[i]);
}

//Inizializzazione membri statici
//nota bene non va scritto CSudokuEngine CSudokuEngine::_instance() perchè è private

CSudokuEngine CSudokuEngine::_instance;

ROW CSudokuEngine::_sample_row;

//------------------------------------------------------------------------------

//riempie la sequenza missing con i numeri mancanti in row
//restituisce il numero di numeri mancanti
int CSudokuEngine::findMissing(ROW missing,ROW row)
{
    if (missing==NULL) return -1;
    if (row==NULL) return -1;

    bool found;

    int cnt=0;

    for (int i=1;i<=MAX_ROW;i++)
    {
        found=false;

        for (int j=0;j<MAX_ROW;j++)
        {
            if (row[j]==i)
            {
                found=true;
                break;
            }
        }

        if (found) continue; //il numero i è stato trovato nella sequenza

        //aggiunge un elemento mancante
        missing[cnt]=i;
        cnt++;
    }

    return cnt;
}

//Indica se la sequenza è valida (anche se è incompleta)
//err_index=restituisce l'indice non valido (base 1)
bool CSudokuEngine::isValid(ROW row,int* err_index)
{
    if (row==NULL) return false;

    ROW s;
    int v;

	if (err_index) *err_index=0;

    createSampleRow(s);

    for (int i=0;i<MAX_ROW;i++)
    {
		if (err_index) *err_index=i+1;

        v=row[i];

        if (v<0 || v>MAX_ROW) return false;

        if (v>0)
        {
            if (s[v-1]==0) return false; //il numero appare due volte

            s[v-1]=0;
        }

    }

	if (err_index) *err_index=0; //valida

    return true;
}

bool CSudokuEngine::isComplete(ROW row)
{
    for (int i=0;i<MAX_ROW;i++) if (row[i]<=0) return false;

    return true;
}

void CSudokuEngine::createSampleRow(ROW dest)
{
     for (int i=0;i<MAX_ROW;i++) dest[i]=i+1;   
}


int CSudokuEngine::getRegionSize(void)
{
    return _rside;
}

//Restituisce la riga della tabella.
//la prima riga ha indice 1
void CSudokuEngine::getRow(ROW row,BOARD board,int rowindex)
{
    if (board==NULL || row==NULL) return;

    if (rowindex>=1 && rowindex<=MAX_ROW)
    {
        for (int i=0;i<MAX_ROW;i++) row[i]=board[rowindex-1][i];
    }
}

//Restituisce la colonna della tabella
//la prima colonna ha indice 1
void CSudokuEngine::getCol(ROW col,BOARD board,int colindex)
{
    if (board==NULL || col==NULL) return;

    if (colindex>=1 && colindex<=MAX_ROW)
    {
        for (int i=0;i<MAX_ROW;i++) col[i]=board[i][colindex-1];
    }
}

//Restituisce una regione della tabella
//Le regioni sono numerate da sinistra a destra e dall'alto
//verso il basso.La regione con indice 1 è quella nell'angolo in alto a
//sinistra

void CSudokuEngine::getRegion(ROW region,BOARD board,int rgx,int rgy)
{
    if (board==NULL || region==NULL) return;
        
    int lu=this->getRegionSize();   
    
    if (rgx>=1 && rgx<=lu && rgy>=1 && rgy<=lu)
    {        
        //indice della colonna di inizio nella matrice board
        int sc=(rgx-1)*lu;
        
        //indice della riga di inizio nella matrice BOARD
        int sr=(rgy-1)*lu;

        for (int i=0;i<lu;i++)
        {            
            region[i]=board[sr][sc+i];
            region[i+lu]=board[sr+1][sc+i];
            region[i+2*lu]=board[sr+2][sc+i];
        }
    }
}


/*
Restituisce in res i valori possibili per la casella con riga row e colonna col
la funzione restituisce il numero dei valori possibili.
Se la tabella non è valida, o se la casella è occupata rende -1
row e col vanno da 1 a MAX_ROW
esegue il controllo di validità tabella
*/

int CSudokuEngine::findValues(ROW res,int row,int col,BOARD board)
{
    return findValues(res,row,col,board,true);   
}

/*
Restituisce in res i valori possibili per la casella con riga row e colonna col
la funzione restituisce il numero dei valori possibili.
Se la tabella non è valida, o se la casella è occupata rende -1
row e col vanno da 1 a MAX_ROW
*/

int CSudokuEngine::findValues(ROW res,int row,int col,BOARD board,bool checkBoard)
{    
    int i,v;
     
    for (i=0;i<MAX_ROW;i++) res[i]=0;
    
    if (!res || !board) return -1;
    //controlla che la tabella sia valida
    if (checkBoard && !isValid(board,0,0)) return -1;
    
    if (!(row>=1 && row<=MAX_ROW)) return -1;
    if (!(col>=1 && col<=MAX_ROW)) return -1;
    
    row--;
    col--;
    
    if (board[row][col]!=0) return -1; //già occupata
    
    ROW vals;
    
    //riempie il risultato con tutti i valori
    this->createSampleRow(vals);
    
    //esclude i valori della riga e colonna di appartenenza
    for (i=0;i<MAX_ROW;i++)
    {
        //esamina la riga di appartenenza
        if (i!=col)
        {
            v=board[row][i];
            
            if (v>0 && v<=MAX_ROW) vals[v-1]=0;        
        }        
        
        //esamina la colonna di appartenenza
        if (i!=row)
        {
            v=board[i][col];
            
            if (v>0 && v<=MAX_ROW) vals[v-1]=0;
        }                
    } 
    
    //esamina la regione
    int rx=0,ry=0;
    
    getRegionIndex(row+1,col+1,&rx,&ry);
    
    ROW region;
    
    getRegion(region,board,rx,ry);
    
    for (i=0;i<MAX_ROW;i++)
    {
        v=region[i];
        
        if (v>0 && v<=MAX_ROW) vals[v-1]=0;        
    }         
    
    rx=0;
    
    for (i=0;i<MAX_ROW;i++) 
    {
        if (vals[i]>0) res[rx++]=vals[i];        
    }                
    
    return rx;
}

/*
Restituisce l'indice della regione in base alla riga e la colonna 
*/
void CSudokuEngine::getRegionIndex(int row,int col,int* rgx,int* rgy)
{
    *rgx=0;
    *rgy=0;
     
    int lu=getRegionSize();
     
    if (row>=1 && row<=MAX_ROW && col>=1 && col<=MAX_ROW)
    {
        *rgx=((int)((col-1)/lu))+1;             
        *rgy=((int)((row-1)/lu))+1;           
    }        
    
}

//restituisce true se la tabella è valida anche se incompleta
//err_row ed err_col hanno base 1 e restituiscono la riga e colonna della cella non valida
       
bool CSudokuEngine::isValid(BOARD board,int* err_row,int* err_col)
{
	int i,j;
	int e;

    ROW r;    
    
    for (i=1;i<=MAX_ROW;i++)
    {
        getRow(r,board,i);

        if (!isValid(r,&e)) 
		{
			if (err_row) *err_row=i;
			if (err_col) *err_col=e;
			return false;
		}

        getCol(r,board,i);

        if (!isValid(r,&e)) 
		{
			if (err_row) *err_row=e;
			if (err_col) *err_col=i;
			return false;
		}
        
    }
    
    int lu=this->getRegionSize();               
    
    for (i=1;i<=lu;i++)
    {                
      for (j=1;j<=lu;j++)
      {               
         getRegion(r,board,i,j);
         if (!isValid(r,&e)) 
		 {
			//indice della colonna di inizio nella matrice board
			int sc=(i-1)*lu;
        
			//indice della riga di inizio nella matrice BOARD
			int sr=(j-1)*lu;

			if (err_row) *err_row=sr+(int)((e-1)/_rside)+1;
			if (err_col) *err_col=sc+(int)((e-1)%_rside)+1;
			
			return false;
		 }
      }
    } 
    
    return true;
}

//restituisce true se la tabella è completa anche se non valida
bool CSudokuEngine::isComplete(BOARD board)
{
    if (board==NULL) return false;

    for (int i=0;i<MAX_ROW;i++)
    {
        for (int j=0;j<MAX_ROW;j++)
        {
            if (board[i][j]<=0) return false;
        }
    }
    
    return true;
}

int CSudokuEngine::solve(BOARD board)
{
	_breakRequest=false;
	_stackCnt=0;
	_totCall=0;

	_state=ENGST_SOLVING_STRATEGIES;
	if (pfnUpdateGUI) pfnUpdateGUI(_state,0,0);

    int res=solve(board,false);

	if (res==SOLVE_OK)
	{
		//risolto
		_state=ENSGT_SOLVED;
		if (pfnUpdateGUI) pfnUpdateGUI(_state,0,0);
	}
	else if (res==SOLVE_ERR_BREAK)
	{
		//interrotto
		_state=ENGST_BREAK;
		if (pfnUpdateGUI) pfnUpdateGUI(_state,0,0);
	}
	else
	{
		//errore o impossibile risolvere
		_state=ENGST_ERROR;
		if (pfnUpdateGUI) pfnUpdateGUI(_state,res,0);
		
	}

	return res;

}

/*
//risolve il sudoku
//board=tabella parzialmente riempita da risolvere
//bStep=esegue un solo step (non implementato) 
*/

int CSudokuEngine::solve(BOARD board,bool bStep)
{   
     int res;     
     
     if (!board) return SOLVE_ERR_INVALID;

	 if (_breakRequest) 
	 {		
		 return SOLVE_ERR_BREAK; //interruzione richiesta 
	 }

     //controlla che la tabella sia valida
     if (!isValid(board,0,0)) 
	 {		 
		 return SOLVE_ERR_INVALID;	      
	 }

	 //aumentare questo valori per aumentare la potenza di calcolo
	 if (_stackCnt>_maxBifurcations)
	 {		
		 return SOLVE_ERR_STACK;
	 }

	 //aumentare questo valori per aumentare la potenza di calcolo
	 if (++_totCall>_maxCalls)
	 {		
		return SOLVE_ERR_CALLS;
	 }

	 //chiama la funzione call back
	 if (pfnUpdateGUI)
	 {
		pfnUpdateGUI(_state,_stackCnt,_totCall);
	 }
          
     bool bloop=true;
     bool bsing;     
     
     //attenzione: questa viene copiata ad ogni chiamata ricorsiva
     BOARD bf;
	 //valori possibili temporanei
	 BOARDCELLS cells;
     
     int count,count1;
     int v,r,c;	
     
     while(bloop)
     {                   
        //ricerca tutte le celle con un singolo valore possibile
        count=findCells(cells,board,1);            
           
        if (count>0)
        {
           for (v=0;v<count;v++)
           {
               //riempie con i singleton trovati
               board[cells[v].row-1][cells[v].col-1]=cells[v].admValues[0];               
           }                                                
           
            //restituisce true se ha completato la tabella
            if (isComplete(board)) 
			{			
				return SOLVE_OK;       
			}
                      
        } 
       
		do
		{

            bsing=false;
             
            //ricerca tutti valori possibili per ogni cella libera
            count=findCells(cells,board,-1); 
			
			//elimina i valori univoci per riga e colonna
			findSingles(cells,count);

			//elimina i vaolori unici per regione
			findSingleInRegions(cells,count);
            
			//stategia 1 : "naked pairs"
			stripSequence(cells,count);

			//strategia 2 : elimina gli "hidden single"
			stripHiddenSingle(cells,count);			
            
            for (v=0;v<count;v++) 
            {
                if (cells[v].admCount==1)
                {
                   bsing=true;
                                         
                   board[cells[v].row-1][cells[v].col-1]=cells[v].admValues[0];                                  
                } 
                
                else if (cells[v].admCount==0) 
				{								
					return SOLVE_ERR_IMPOSSIBLE; //irrisolvibile (vicolo cieco)                              
				}
            }

		} while (bsing);


		//restituisce true se ha completato la tabella
            if (isComplete(board)) 
			{			
				return SOLVE_OK;       
			}
			
			if (!bsing)
			{ 			
	
				//inizia la biforcazione 
				int bindex=findBestCells(cells,count);

				if (bindex<0) return SOLVE_ERR_IMPOSSIBLE;

				count1=cells[bindex].admCount;
				
				if (count1==0) return SOLVE_ERR_IMPOSSIBLE;	
				
				r=cells[bindex].row-1;
				c=cells[bindex].col-1;

				for (v=0;v<count1;v++)
				{
					board[r][c]=cells[bindex].admValues[v];

					copyBoard(bf,board);

					// _stackCnt++;	

					res=solve(bf,false);

					// _stackCnt--;	

					if (res==SOLVE_OK) 
					{
						copyBoard(board,bf);							
						return SOLVE_OK;

					}						

				}

				board[r][c]=0;

				return SOLVE_ERR_IMPOSSIBLE;	
		
                       
            } //fine bsing                                                  
       
        
        if (isComplete(board)) 
		{		
			return SOLVE_OK;         
		}
             
        //if (bStep) bloop=false;        
                       
     }                       
    
     if (isComplete(board)) 
	 {		
		 return SOLVE_OK;
	 }
	 else 
	 {		
		 return SOLVE_ERR_ALGO;
	 }
}

//restisuisce la cella con il numero minimo di valori possibili
int CSudokuEngine::findBestCells(BOARDCELLS cells,int cellbound)
{
	int bestInd=-1;
	int min=MAX_ROW+1;

	for (int i=0;i<cellbound;i++)
	{
		if (cells[i].admCount<min)
		{
			min=cells[i].admCount;
			bestInd=i;
		}
	}

	return bestInd;

}

void CSudokuEngine::copyBoard(BOARD dest,BOARD src)
{
	memcpy(dest,src,sizeof(int)*CELL_COUNT);

}

//restituisce la lista delle celle con un numero di valori
//possibili pari a valcount
//se valcount è -1 riempie con tutti i valori ammissibili

int CSudokuEngine::findCells(BOARDCELLS cells,BOARD board,int valcount)
{    
    memset(cells,0,sizeof(CELL_POS)*MAX_ROW*MAX_ROW);    
        
    int idx=0;
    int res=0;
    ROW seq;
    
    for (int i=1;i<=MAX_ROW;i++)
    {
        for (int j=1;j<=MAX_ROW;j++)
        {
            //esegue il controllo solo per le celle vuote
            if (board[i-1][j-1]==0)
            {
                res=findValues(seq,i,j,board);
                
                if (res==valcount || valcount==-1) 
                {
                    cells[idx].row=i;
                    cells[idx].col=j;
                    cells[idx].admCount=res;
                    memcpy(cells[idx].admValues,seq,sizeof(int)*MAX_ROW);                                
                    idx++;                               
                }
            }         
        }
    }
    
    return idx;    
}

//Controlla le coppie e sequenze univoche di valori ammessi 
//se ad esempio per due celle di una riga sono possibili solo i valori 3 e 4
//e per un'altra cella della stessa riga sono possibili 3,4,7 allora per la tarza cella sarà possibile solo il valore 7

int CSudokuEngine::stripSequence(BOARDCELLS cells,int cellBound)
{
    int admCount,row,col,v;
	
    for (int i=0;i<cellBound;i++)
    {   
        admCount=cells[i].admCount;
        row=cells[i].row;
        col=cells[i].col;
        
        if (admCount==2)
        {                        
            for (int j=0;j<cellBound;j++)
            {
                if (i!=j && cells[j].admCount==admCount && (cells[j].row==row || cells[j].col==col))
                {
                   bool beq=true;				   
                                                
                   for (v=0;v<admCount;v++)
                   {
                       beq = (cells[j].admValues[v]==cells[i].admValues[v]);                    
                       if (!beq) break;                       
                   }
                   
                   if (beq)
                   {
                       if (cells[j].row==row)
                       {
                           for (v=0;v<cellBound;v++)
                           {
                               if (cells[v].admCount>admCount && cells[v].row==row)
                               {
                                  subtractValues(&cells[v],&cells[i]);                                                                                                                        
                                                              
                               }                               
                               
                           }                                                
                       } 
                       
                       if (cells[j].col==col)
                       {                           
                           for (v=0;v<cellBound;v++)
                           {
                               if (cells[v].admCount>admCount && cells[v].col==col)
                               {
                                  subtractValues(&cells[v],&cells[i]);                                                      
                               }                                
                           }                                      
                       }                       
                                                  
                   }//fine bew                                                                  
                }           
            }//fine for j
        }        
         
    }//fine for i    
    
    return 0;
}    
//sottrae p2 da p1 (p1 viene modificato)

void CSudokuEngine::subtractValues(CELL_POS_PTR p1,CELL_POS_PTR p2)
{
     int val,v,i;
	
     
     for (v=0;v<p2->admCount;v++)
     {
         val=p2->admValues[v];
         
         for (i=0;i<p1->admCount;i++)
         {
             if (p1->admValues[i]==val) p1->admValues[i]=0;             
         }               
         
     }     
     

	 ROW tmp;
	 memset(tmp,0,sizeof(int)*MAX_ROW);
	 memcpy(tmp,p1->admValues,sizeof(int)*MAX_ROW);
	 memset(p1->admValues,0,sizeof(int)*MAX_ROW);
	 i=0;

     //riarrangia il vettore
     for (v=0;v<MAX_ROW;v++)
	 {
		 if (tmp[v]>0)
		 {
			 p1->admValues[i++]=tmp[v];
		 }
	 }

	 p1->admCount=i;
}

//togli il valore ammesso value dalla cella
void CSudokuEngine::subtractSingleValue(CELL_POS_PTR p,int value)
{
	for (int i=0;i<p->admCount;i++)
	{
		if (p->admValues[i]==value)
		{
			for (int k=i;k<p->admCount-1;k++) p->admValues[k]=p->admValues[k+1];
			
			p->admValues[p->admCount-1]=0;
			
			--p->admCount;

			if (p->admCount==1)
			{
				//DEBUG

				p->admCount=1;

			}
			
			break;			
		}
	}

}

void CSudokuEngine::stripHiddenSingle(BOARDCELLS cells,int cellbound)
{
	CELL_POS_PTR preg[MAX_ROW];//puntatori alle celle di una regione
	CELL_POS_PTR p;
	int row1,row2,col1,col2;
	int c;
	int r;
	int rr,cc;
	int br,bc;
	int ind;
	int i,j,k;

	for (int ir=1;ir<=_rside;ir++)
	{
		for (int ic=1;ic<=_rside;ic++)
		{
			//resetta i punatori
			memset(preg,0,sizeof(CELL_POS_PTR));
			ind=0;
			
			//righe che definiscono la regione corrente
			row1=(ir-1)*_rside+1;
			row2=row1+_rside-1;

			col1=(ic-1)*_rside+1;
			col2=col1+_rside-1;

			//ricerca tutte le celle della regione corrente
			for (i=0;i<cellbound;i++)
			{
				c=cells[i].col;
				r=cells[i].row;

				if (c>=col1 && c<=col2 && r>=row1 && r<=row2) preg[ind++]=&cells[i];

			}

			//esegue un controllo di tutti i valori su questa regione
			for (i=1;i<=MAX_ROW;i++)
			{
				rr=cc=0;
				br=bc=0;

				j=0;

				//ciclo su tutte le celle della regione
				while (j<ind)
				{
					p=preg[j];

					//ciclo su tutti i valori ammessi per la cella
					for (k=0;k<p->admCount;k++)
					{
						if (p->admValues[k]==i)
						{
							if (rr==0) {rr=p->row;br=1;}
							else if (rr!=p->row) rr=-1; //non è un hidden per quel che riguarda le righe
							else if (rr==p->row && rr>0) br++; //incrementa gli elementi sulla stesa riga

							//colonne
							if (cc==0) {cc=p->col;bc=1;}
    						else if (cc!=p->col) cc=-1; //non è un hidden per quel che riguarda le righe
							else if (cc==p->col && cc>0) bc++; //incrementa gli elementi sulla stesa riga

							break;

						}
					}

					++j;
				} //fine while

				if (br>1 && rr>0)
				{
					//è un hidden single: ci sono due celle sulla stessa riga rr con lo stesso valore e nessun altra cella della regione con quel valore
					//il valore puo' essere eliminato in tutte le celle della stessa riga che non appartengono alla regione corrente
                    for (k=0;k<cellbound;k++)
					{
						r=cells[k].row;
						c=cells[k].col;

						if (r==rr && !(c>=col1 && c<=col2))
						{
							//la cella appartiene alla stessa riga ma non alla stessa regione
							subtractSingleValue(&cells[k],i);
						}
					}

				}
				else if (bc>1 && cc>0)
				{
					for (k=0;k<cellbound;k++)
					{
						r=cells[k].row;
						c=cells[k].col;

						if (c==cc && !(r>=row1 && r<=row2))
						{
							//la cella appartiene alla stessa riga ma non alla stessa regione
							subtractSingleValue(&cells[k],i);
						}
					}
				}

			}//fine ciclo valori (i)

		}//fine for ic

	}//fine for ir
}

//trova tutte i valori singoli (quelli che compaiono una sola volta per riga colonna o regione)
void CSudokuEngine::findSingles(BOARDCELLS cells,int cellbound)
{
	
	int v,i,j,k;
	int cc,rr;
	int ind;
	CELL_POS_PTR p;

	//ciclo righe
	for (i=1;i<=MAX_ROW;i++)
	{
		for (v=1;v<=MAX_ROW;v++)
		{
			cc=0;
			ind=-1;

			for (j=0;j<cellbound;j++)
			{
				p=&cells[j];

				if (p->row==i)
				{
					for (k=0;k<p->admCount;k++)
					{
						if (p->admValues[k]==v)
						{
							if (cc==0) 
							{
								cc=p->col;
								ind=j;//memorizza la cella
							}

							//il valore esiste già
							else 
							{
								cc=0;
								ind=-1;
								j=cellbound;
								k=p->admCount;
							}

							break;

						}//fine if
						
					}
				}//fine if p->row==i

			}//fine j

			if (ind>0)
			{
				cells[ind].admCount=1;
				cells[ind].admValues[0]=v; //unico valore ammesso

				//toglie il valore dai valori ammessi della colonna di appartenenza
				for (j=0;j<cellbound;j++)
				{
					p=&cells[j];

					if (p->col==cc && p->row!=i)
					{
						//toglie il valore dalla colonna
						subtractSingleValue(p,v);
					}
				}
				//il valore è singolo sulla riga i
			}
		}//ciclo valori (v)

	}//ciclo righe i
	

	//ciclo colonne
	for (i=1;i<=MAX_ROW;i++)
	{
		for (v=1;v<=MAX_ROW;v++)
		{
			rr=0;
			ind=-1;

			for (j=0;j<cellbound;j++)
			{
				p=&cells[j];

				if (p->col==i)
				{
					for (k=0;k<p->admCount;k++)
					{
						if (p->admValues[k]==v)
						{
							if (rr==0) 
							{
								rr=p->row;
								ind=j;//memorizza la cella
							}

							//il valore esiste già
							else 
							{
								rr=0;
								ind=-1;
								j=cellbound;
								k=p->admCount;
							}

							break;

						}//fine if
						
					}
				}//fine if p->col==i

			}//fine j

			if (ind>0)
			{
				cells[ind].admCount=1;
				cells[ind].admValues[0]=v; //unico valore ammesso

				//toglie il valore dai valori ammessi della colonna di appartenenza
				for (j=0;j<cellbound;j++)
				{
					p=&cells[j];

					if (p->row==rr && p->col!=i)
					{
						//toglie il valore dalla colonna
						subtractSingleValue(p,v);
					}
				}
				//il valore è singolo sulla riga i
			}
		}//ciclo valori (v)

	}//ciclo colonne i
}

//valori univoci regione per regione
void CSudokuEngine::findSingleInRegions(BOARDCELLS cells,int cellbound)
{

	int ir,ic,i,r,c,k;
	int ind;
	int fc;
	int pf;
	int row1,row2,col1,col2;	
	CELL_POS_PTR preg[MAX_ROW];
	CELL_POS_PTR pcell,pcell1;

	for (ir=1;ir<=_rside;ir++)
	{
		for (ic=1;ic<=_rside;ic++)
		{
			//resetta i punatori
			memset(preg,0,sizeof(CELL_POS_PTR));
			ind=0;

			//righe che definiscono la regione corrente
			row1=(ir-1)*_rside+1;
			row2=row1+_rside-1;

			col1=(ic-1)*_rside+1;
			col2=col1+_rside-1;

			//ricerca tutte le celle della regione corrente
			for (i=0;i<cellbound;i++)
			{
				c=cells[i].col;
				r=cells[i].row;

				if (c>=col1 && c<=col2 && r>=row1 && r<=row2) preg[ind++]=&cells[i];
			}

			//esegue un controllo di tutti i valori su questa regione
			for (i=1;i<=MAX_ROW;i++)
			{
				fc=0; //numero di celle trovate in cui è ammesso il valore i
                
				//ciclo su tutte le cella della regione
				for (k=0;k<ind;k++)
				{
					pcell=preg[k];

					for (int j=0;j<pcell->admCount;j++)
					{
						if (pcell->admValues[j]==i)
						{
							fc++;
							pf=k;
							break;
						}
					}
				}//fine k


				if (fc==1)
				{
					//imposta l'unico valore possibile per la cella
					pcell=preg[pf];
					pcell->admCount=1;
					pcell->admValues[0]=i;
					r=pcell->row;
					c=pcell->col;

					//elimina quel valore dalla riga e colonna corrispondente
					for (k=0;k<cellbound;k++)
					{
						pcell1=&cells[k];
						//nota l'operatore xor
						//se sia la riga che la colonna sono uguali non deve sottrarre perchè la cella 
						//è coincidente
						if ((r==pcell1->row) ^ (c==pcell1->col))
						{
							//toglie il valore ammesso
							subtractSingleValue(pcell1,i);
						}
						
					}

				}

			}//fine ciclo valori

		}//fine ic

	}//fine ir

}

/*
Restituisce in cell la cella con riga e colonna row,col
*/
bool CSudokuEngine::getCell(CELL_POS_PTR cell,int row,int col,BOARDCELLS cells)
{
     if (!cell) return false;
     
     memset(cell,0,sizeof(CELL_POS));
     
     for (int i=0;i<CELL_COUNT;i++)
     {
         if (cells[i].row==row && cells[i].col==col)         
         {
             *cell=cells[i];
             return true;                  
         }
     }    
               
     return false;     
}


 //imposta numero massimo di chiamate ricorsive 
bool CSudokuEngine::setMaxCalls(int n)
{
	if (n>=1) 
	{
		_maxCalls=n;
		return true;
	}
	else return false;
}


//imposta il massimo numero di biforcazioni
bool CSudokuEngine::setMaxBifurcations(int n)
{
	if (n>=1)
	{
		_maxBifurcations=n;
		return true;	

	}
	else return false;
}

int CSudokuEngine::getMaxCalls(void)
{
	return _maxCalls;
}

//numero totale di chiamate alla funzione di soluzione
//se è 1 allora ammette di sicuro una unica soluzione
int CSudokuEngine::getTotCalls(void)
{
	return _totCall;
}

int CSudokuEngine::getMaxBifurcations(void)
{
	return _maxBifurcations;
}

void CSudokuEngine::RequestBreak(void)
{
	//richiede in modo asincrono l'interruzione dell'elaborazione
	_breakRequest=true;
};

//restituisce lo stato
int CSudokuEngine::getState(void)
{
	return _state;
}