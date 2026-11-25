#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define MAX_BUFFER (1<<16) 
#define BITMAP_MAX_SIZE 64
#define MAX_PILA 65536 
#define MAX_DIPENDENTI 2048
#define MAX_PIATTI 30000
#define BLOCCHI_MENU_MAX_SIZE ((MAX_PIATTI / BITMAP_MAX_SIZE) + 1) 

#define MSB_1 0x8000000000000000ULL // numero a 64 bit con tutti zeri tranne il bit più significativo che è un uno
#define MSB_0 0x7fffffffffffffffULL // numero a 64 bit con tutti uni tranne il bit più significativo che è uno zero
#define MAX_LL 0xffffffffffffffffULL // numero a 64 bit con tutti uni
#define SEQ_ISP(hash, i, modulo) ((unsigned int)((unsigned int)(hash) + ((unsigned long long)(i+i*i)>>1))%modulo) 

//Ottimizzazioni operazioni: volendo posso ottimizzare anche l'operazione di modulo
#define DIV_64(num) (unsigned int)(((unsigned int)(num))>>6)

// ---------------- struct per liste di dipendenti per piatto ---------------

typedef struct {
    int* indici;
    int count;
    int capacity;
} ListaAdiacenza;

// ---------------- struct per menu e dipendenti ---------------

typedef struct{
    unsigned long long** bitmap_piatti_graditi;
    unsigned long long** bitmap_piatti_non_graditi;
    int num_dipendenti;
    int dimensione;
    unsigned short** blocchi_attivi;
    int* num_blocchi_attivi;
} Dipendenti;

// ----------- struct per hash table ----------------

typedef struct{
    unsigned int hash;
    int posizione;
} elemento_hash_table;

typedef struct{
    elemento_hash_table** tab;
    int dimensione;
    int num_elementi;
} TabellaHash;

// ----------- struct per piatti -------------------

typedef struct{
    int frequenza;
    int posizione;
} struttura_frq_piatti;

// ---------- struct per pila ----------------------

typedef struct{
    int posizione;
    int azione; // azione=0 se il piatto va tolto dal menu, azione=1 se il piatto va inserito nel menu
} elemento_pila;


// ----------DEFINIZIONI----------------------

TabellaHash* tabella_hash;
char delimitatori[2] = {' ', 10};

elemento_pila* pila;
int cima_pila = -1;

unsigned long long* ripristino_menu_globale;
unsigned long long* copia_dipendenti_globale;
int blocchi_menu_globale;
int blocchi_dipendenti_globale;
// --------------------------------
unsigned short* opzioni_base;
unsigned short* opzioni_globale;
// --------------------------------

// liste dei piatti per dipendente
ListaAdiacenza* dip_gradito;
ListaAdiacenza* dip_non_gradito;

// --------------------- FUNZIONI ----------------------------------

//tabella hash
unsigned int hash_fn(char* parola);
int trova_posizione_piatto(char* parola);
void inserisci_piatto_in_hash_table(char* parola);
void rialloca_tabella_hash();

//risoluzione logica
int risolvi_menu(Dipendenti* dipendenti, unsigned long long* dipendenti_salvi, int* numero_dipendenti, int* piatti_totali, int ultimo_piatto_considerato, struttura_frq_piatti** frequenza_piatti, unsigned long long* piatti_da_scartare_totalmente, unsigned long long* piatti_da_scartare_graditi, unsigned long long* piatti_da_scartare_non_graditi, unsigned long long* menu_considerati, unsigned long long* menu_scelti, unsigned long long** piatti_dipendenti_positivi, unsigned long long** piatti_dipendenti_negativi, int ric);

//sorting
int compare(const void* a, const void* b) {
    struttura_frq_piatti* elem_a = *(struttura_frq_piatti**)a;
    struttura_frq_piatti* elem_b = *(struttura_frq_piatti**)b;
    if (elem_a->frequenza < elem_b->frequenza) return 1;
    if (elem_a->frequenza > elem_b->frequenza) return -1;
    return 0; 
}

//funzione lista
void aggiungi_adiacenza(ListaAdiacenza* lista, int dipendente) {
    if (lista->count == lista->capacity) {
        lista->capacity = (lista->capacity == 0) ? 4 : lista->capacity * 2;
        lista->indici = realloc(lista->indici, lista->capacity * sizeof(int));
    }
    lista->indici[lista->count++] = dipendente;
}

int main(){

    // ----------------------- INIT ----------------------------
    //inizio con dimensione 8 cosi da non dover riallocare al primo inserimento
    tabella_hash = (TabellaHash*) malloc(sizeof(TabellaHash));
    tabella_hash->dimensione = 8;
    tabella_hash->num_elementi = 0;
    tabella_hash->tab = (elemento_hash_table**)calloc(tabella_hash->dimensione, sizeof(elemento_hash_table*));

    //menu
    int piatti_menu = 0;

    //init dipendenti
    Dipendenti* dipendenti;
    dipendenti = (Dipendenti*) malloc(sizeof(Dipendenti));
    dipendenti->num_dipendenti = 0;
    dipendenti->dimensione = 8;
    dipendenti->bitmap_piatti_graditi = (unsigned long long**)malloc(8*sizeof(unsigned long long*)); 
    dipendenti->bitmap_piatti_non_graditi = (unsigned long long**)malloc(8*sizeof(unsigned long long*)); 
    dipendenti->blocchi_attivi = (unsigned short**)malloc(8*sizeof(unsigned short*));
    dipendenti->num_blocchi_attivi =(int*)malloc(8*sizeof(int));
    
    opzioni_base = (unsigned short*)calloc(8, sizeof(unsigned short));

    // --------------------- FINE INIT ---------------------
    
    char buffer[MAX_BUFFER];

    char* a = fgets(buffer, sizeof(buffer), stdin);

    if(*a == 0){}
    
    char* token = strtok(buffer, delimitatori);
    while(token != NULL){
        piatti_menu++;
        inserisci_piatto_in_hash_table(token);
        token = strtok(NULL, delimitatori);
    }

    pila = (elemento_pila*) malloc(piatti_menu * sizeof(elemento_pila));
    dip_gradito = (ListaAdiacenza*) calloc(piatti_menu, sizeof(ListaAdiacenza));
    dip_non_gradito = (ListaAdiacenza*) calloc(piatti_menu, sizeof(ListaAdiacenza));

    // a questo punto tutti i piatti disponibili sono in tabella hash
    // ora bisogna gestire i dipendenti

    int posizione_max_blocchi_bitmap = (int)(piatti_menu / 64);
    unsigned long long* piatti_da_scartare_non_graditi = (unsigned long long*) calloc(posizione_max_blocchi_bitmap+1, sizeof(unsigned long long));
    unsigned long long* piatti_da_scartare_graditi = (unsigned long long*) calloc(posizione_max_blocchi_bitmap+1, sizeof(unsigned long long));
    unsigned long long* piatti_da_scartare_totalmente = (unsigned long long*) calloc(posizione_max_blocchi_bitmap+1, sizeof(unsigned long long));
    

    // inizializzo un array che mi tenga quante volte viene citato un piatto
    struttura_frq_piatti** frequenza_piatti = (struttura_frq_piatti**) malloc(piatti_menu*sizeof(struttura_frq_piatti*));
    struttura_frq_piatti* blocco_frequenze_piatti = (struttura_frq_piatti*) calloc(piatti_menu, sizeof(struttura_frq_piatti));
    for(int i=0; i<piatti_menu; i++){
        frequenza_piatti[i] = &(blocco_frequenze_piatti[i]);
        frequenza_piatti[i]->posizione = -1;
    }
    int* frequenza_piatti_primo = (int*) malloc(piatti_menu*sizeof(int));
    for(int i=0; i<piatti_menu; i++){
        frequenza_piatti_primo[i] = -1;
    }

    int tmp_blocchi[BLOCCHI_MENU_MAX_SIZE];
    

    // ciclo per ogni riga, ovvero per ogni dipendente
    while(fgets(buffer, sizeof(buffer), stdin) != NULL){
        
        char* piatto;
        int bit_pos = 0;
        bool blocco_visto[BLOCCHI_MENU_MAX_SIZE] = {false};
        int numero_blocchi = 0;

        
        if(dipendenti->num_dipendenti == dipendenti->dimensione - 1){
            dipendenti->dimensione = (dipendenti->dimensione)<<1;
            dipendenti->bitmap_piatti_graditi = (unsigned long long**)realloc(dipendenti->bitmap_piatti_graditi, dipendenti->dimensione * sizeof(unsigned long long*));
            dipendenti->bitmap_piatti_non_graditi = (unsigned long long**)realloc(dipendenti->bitmap_piatti_non_graditi, dipendenti->dimensione * sizeof(unsigned long long*));
            
            dipendenti->blocchi_attivi = (unsigned short**)realloc(dipendenti->blocchi_attivi, dipendenti->dimensione * sizeof(unsigned short*));
            dipendenti->num_blocchi_attivi = (int*)realloc(dipendenti->num_blocchi_attivi, dipendenti->dimensione * sizeof(int));

            opzioni_base = (unsigned short*)realloc(opzioni_base, dipendenti->dimensione * sizeof(unsigned short));
            memset(opzioni_base + (dipendenti->dimensione >> 1), 0, (dipendenti->dimensione >> 1) * sizeof(unsigned short));
        }

        (dipendenti->bitmap_piatti_graditi)[dipendenti->num_dipendenti] = (unsigned long long*)calloc((posizione_max_blocchi_bitmap+1), sizeof(unsigned long long));
        (dipendenti->bitmap_piatti_non_graditi)[dipendenti->num_dipendenti] = (unsigned long long*)calloc((posizione_max_blocchi_bitmap+1), sizeof(unsigned long long));


        piatto = strtok(buffer, delimitatori);
        do{
            


            if((*piatto) == '-'){
                piatto++;

                bit_pos = trova_posizione_piatto(piatto);
                if(!blocco_visto[DIV_64(bit_pos)]){
                    blocco_visto[DIV_64(bit_pos)] = true;
                    tmp_blocchi[numero_blocchi] = DIV_64(bit_pos);
                    numero_blocchi++;
                }
                
                //conto le opzioni del dipendente
                unsigned long long mask = (MSB_1>>(bit_pos%64));
                int b = DIV_64(bit_pos);
                int num_d = dipendenti->num_dipendenti;
                bool gia_neg = ((dipendenti->bitmap_piatti_non_graditi)[num_d][b] & mask) != 0;
                bool gia_pos = ((dipendenti->bitmap_piatti_graditi)[num_d][b] & mask) != 0;

                if (!gia_neg && !gia_pos) {
                    opzioni_base[num_d]++;
                    aggiungi_adiacenza(&dip_non_gradito[bit_pos], dipendenti->num_dipendenti); 
                } else if (gia_pos && !gia_neg) {
                    opzioni_base[num_d] = MAX_PIATTI+1; 
                }

                (dipendenti->bitmap_piatti_non_graditi)[num_d][b] =(dipendenti->bitmap_piatti_non_graditi)[num_d][b] | mask;
                piatti_da_scartare_non_graditi[b] =piatti_da_scartare_non_graditi[b] | mask;
                
                if(frequenza_piatti_primo[bit_pos] != dipendenti->num_dipendenti){ 
                    (frequenza_piatti[bit_pos]->frequenza)++;
                    (frequenza_piatti[bit_pos]->posizione) = bit_pos;
                    frequenza_piatti_primo[bit_pos] = dipendenti->num_dipendenti;
                }
                
                
        
            }else{
                bit_pos = trova_posizione_piatto(piatto);
                
                if(!blocco_visto[DIV_64(bit_pos)]){
                    blocco_visto[DIV_64(bit_pos)] = true;
                    tmp_blocchi[numero_blocchi] = DIV_64(bit_pos);
                    numero_blocchi++;
                }

                //conto le opzioni del dipendente
                unsigned long long mask = (MSB_1>>(bit_pos%64));
                int b = DIV_64(bit_pos);
                int num_d = dipendenti->num_dipendenti;
                bool gia_pos = ((dipendenti->bitmap_piatti_graditi)[num_d][b] & mask) != 0;
                bool gia_neg = ((dipendenti->bitmap_piatti_non_graditi)[num_d][b] & mask) != 0;

                if ((!gia_pos) && (!gia_neg)) {
                    opzioni_base[num_d]++;
                    aggiungi_adiacenza(&dip_gradito[bit_pos], dipendenti->num_dipendenti); 
                } else if (gia_neg && !gia_pos) {
                    opzioni_base[num_d] = MAX_PIATTI+1; 
                }
                
                (dipendenti->bitmap_piatti_graditi)[num_d][b] =(dipendenti->bitmap_piatti_graditi)[num_d][b] | mask;
                piatti_da_scartare_graditi[b] =piatti_da_scartare_graditi[b] | mask;

                if(frequenza_piatti_primo[bit_pos] != dipendenti->num_dipendenti){ 
                    (frequenza_piatti[bit_pos]->frequenza)++;
                    (frequenza_piatti[bit_pos]->posizione) = bit_pos;
                    frequenza_piatti_primo[bit_pos] = dipendenti->num_dipendenti;
                }

                
            
            }
            piatto = strtok(NULL, delimitatori);
        }while(piatto != NULL);

        dipendenti->num_blocchi_attivi[dipendenti->num_dipendenti] = numero_blocchi;
        dipendenti->blocchi_attivi[dipendenti->num_dipendenti] = (unsigned short*)malloc(numero_blocchi*sizeof(unsigned short));
        for(int i=0; i<numero_blocchi; i++){
            dipendenti->blocchi_attivi[dipendenti->num_dipendenti][i] = tmp_blocchi[i];
        }

        (dipendenti->num_dipendenti)++;
    }


    int profondita_max = tabella_hash->num_elementi + 1;
    blocchi_menu_globale = (tabella_hash->num_elementi / BITMAP_MAX_SIZE) + 1;
    blocchi_dipendenti_globale = (dipendenti->num_dipendenti / BITMAP_MAX_SIZE) + 1;

    ripristino_menu_globale = (unsigned long long*) malloc((unsigned long long)(profondita_max*blocchi_menu_globale)*sizeof(unsigned long long));
    copia_dipendenti_globale = (unsigned long long*) malloc((unsigned long long)(profondita_max*blocchi_dipendenti_globale)*sizeof(unsigned long long));

    opzioni_globale = (unsigned short*) malloc((unsigned long long)(profondita_max * dipendenti->dimensione) * sizeof(unsigned short));
    // ------------------------------------------


    //ordino per avere prima i piatti che compaiono di più
    qsort(frequenza_piatti, piatti_menu, sizeof(struttura_frq_piatti*), compare);
    
    //Ora ho le bitmap pronte
    int ultimo_piatto_considerato = 0;
    unsigned long long* menu_considerati = (unsigned long long*) calloc(posizione_max_blocchi_bitmap+1, sizeof(unsigned long long));
    unsigned long long* menu_scelti = (unsigned long long*) calloc(posizione_max_blocchi_bitmap+1, sizeof(unsigned long long));
    
    for(int i=0; i<=posizione_max_blocchi_bitmap; i++){
        piatti_da_scartare_graditi[i] = ~piatti_da_scartare_graditi[i];
        piatti_da_scartare_non_graditi[i] = ~piatti_da_scartare_non_graditi[i];
        piatti_da_scartare_totalmente[i] = piatti_da_scartare_non_graditi[i] & piatti_da_scartare_graditi[i];
    }

    unsigned long long* dipendenti_salvi = (unsigned long long*) calloc( ((dipendenti->num_dipendenti)/BITMAP_MAX_SIZE) + 1, sizeof(unsigned long long));

    int result = 0;
    int indice = 1;
    int ricorsione = 0;


    do {

       
        result = risolvi_menu(dipendenti, dipendenti_salvi, &dipendenti->num_dipendenti, &tabella_hash->num_elementi, ultimo_piatto_considerato, frequenza_piatti, piatti_da_scartare_totalmente, piatti_da_scartare_graditi, piatti_da_scartare_non_graditi, menu_considerati, menu_scelti, dipendenti->bitmap_piatti_graditi, dipendenti->bitmap_piatti_non_graditi, ricorsione);
        if(result){
            printf("OK\n");
        } else {
            if(indice == 1){
                printf("KO\n");
                printf("-1\n");
                dipendenti->num_dipendenti--;
            } else {
                printf("-%d\n", indice);
                dipendenti->num_dipendenti--;
            }
            
        }
        indice++;
    } while(!result);
    
    return 0;
}

int risolvi_menu(Dipendenti* dipendenti, unsigned long long* dipendenti_salvi, int* numero_dipendenti, int* piatti_totali, int ultimo_piatto_considerato, struttura_frq_piatti** frequenza_piatti, unsigned long long* piatti_da_scartare_totalmente, unsigned long long* piatti_da_scartare_graditi, unsigned long long* piatti_da_scartare_non_graditi, unsigned long long* menu_considerati, unsigned long long* menu_scelti,unsigned long long** piatti_dipendenti_positivi,unsigned long long** piatti_dipendenti_negativi, int ric){

    const int posizione_max_blocchi_bitmap = ((*piatti_totali)/BITMAP_MAX_SIZE) + 1;
    

    unsigned long long* copia_dipendenti_salvi = copia_dipendenti_globale + (unsigned long long)ric*blocchi_dipendenti_globale;
    unsigned long long* ripristino_menu = ripristino_menu_globale + (unsigned long long)ric*blocchi_menu_globale;


    memcpy(ripristino_menu, menu_considerati, posizione_max_blocchi_bitmap * sizeof(unsigned long long));
    memcpy(copia_dipendenti_salvi, dipendenti_salvi, (((*numero_dipendenti)/BITMAP_MAX_SIZE) + 1) * sizeof(unsigned long long));

    
    bool contraddizione = false;

    unsigned short* copia_opzioni = opzioni_globale + (unsigned long long)ric * dipendenti->dimensione;
    if (ric == 0) {
        memcpy(copia_opzioni, opzioni_base, (*numero_dipendenti) * sizeof(unsigned short));
    } else {
        unsigned short* opzioni_prec = opzioni_globale + (unsigned long long)(ric - 1) * dipendenti->dimensione;
        memcpy(copia_opzioni, opzioni_prec, (*numero_dipendenti) * sizeof(unsigned short));
    }
    // -------------------------------------------------
    
    // Cosi ora quando aggiungo un nuovo piatto non controllo tutti i dipendenti ma solo quelli interessati da quel piatto
    while(cima_pila > -1) {
        
        int posizione_assoluta = pila[cima_pila].posizione;
        int curr_pos = posizione_assoluta / BITMAP_MAX_SIZE;
        int curr_bit = posizione_assoluta % BITMAP_MAX_SIZE;
        int azione = pila[cima_pila].azione;

        //int pos_totali = (*piatti_totali) / BITMAP_MAX_SIZE;
        
        cima_pila--;

        //se il piatto è già considerato controllo che non ci siano problemi
        if ((menu_considerati[curr_pos] & (MSB_1 >> curr_bit)) != 0) {
            int current_azione = 0;
            if( (menu_scelti[curr_pos] & (MSB_1 >> curr_bit)) != 0){
                current_azione = 1;
            }
            if (current_azione != azione) {
                contraddizione = true;
                break;
            }
            continue; 
        }

        //aggiorno il menu
        menu_considerati[curr_pos] = menu_considerati[curr_pos] | (MSB_1 >> curr_bit);
        
        if(azione){
            menu_scelti[curr_pos] = menu_scelti[curr_pos] | (MSB_1 >> curr_bit);
        }
        else{
            menu_scelti[curr_pos] = menu_scelti[curr_pos] & (~(MSB_1 >> curr_bit));
        }


        //inizializzo le liste dei dipendenti da controllare
        ListaAdiacenza* dip_salvati = (azione == 1) ? &dip_gradito[posizione_assoluta] : &dip_non_gradito[posizione_assoluta];
        ListaAdiacenza* dip_rischio = (azione == 1) ? &dip_non_gradito[posizione_assoluta] : &dip_gradito[posizione_assoluta];

        //salvo alcuni dipendenti
        for(int i=0; i<dip_salvati->count; i++){
            int dipendente = dip_salvati->indici[i];
            if(dipendente >= (*numero_dipendenti)) continue;
            copia_dipendenti_salvi[dipendente/BITMAP_MAX_SIZE] = copia_dipendenti_salvi[dipendente/BITMAP_MAX_SIZE] | (MSB_1 >> (dipendente%BITMAP_MAX_SIZE));
        }


        //controllo i dipendenti che potrebbero crearmi una contraddizione
        for(int i=0; i<dip_rischio->count; i++){
            
            int dipendente = dip_rischio->indici[i];
            if(dipendente >= (*numero_dipendenti)) continue; 
            
            int pos_dipendente = dipendente/BITMAP_MAX_SIZE;
            int bit_dipendente = dipendente%BITMAP_MAX_SIZE;

            if( (copia_dipendenti_salvi[pos_dipendente] & (MSB_1 >> bit_dipendente)) != 0) continue;

            
            copia_opzioni[dipendente]--;

            if (copia_opzioni[dipendente] == 0) {
                contraddizione = true;
                break;
            }

            if (copia_opzioni[dipendente] == 1) {
                
                unsigned long long piatti_liberi = 0;
                int posizione_piatto_libero = 0;
                int azione_libera = 0;

                for(int j=0; j<dipendenti->num_blocchi_attivi[dipendente]; j++){
                    int blocco = dipendenti->blocchi_attivi[dipendente][j];
                    piatti_liberi = (piatti_dipendenti_positivi[dipendente][blocco] | piatti_dipendenti_negativi[dipendente][blocco]) & ~menu_considerati[blocco];

                    if(piatti_liberi != 0) {
                        posizione_piatto_libero = blocco * BITMAP_MAX_SIZE;
                        
                        int curr_bit_libero = 0;
                        unsigned long long temp = piatti_liberi;

                        if ((temp & 0xFFFFFFFF00000000ULL) == 0) { curr_bit_libero += 32; temp <<= 32; }
                        if ((temp & 0xFFFF000000000000ULL) == 0) { curr_bit_libero += 16; temp <<= 16; }
                        if ((temp & 0xFF00000000000000ULL) == 0) { curr_bit_libero += 8;  temp <<= 8;  }
                        if ((temp & 0xF000000000000000ULL) == 0) { curr_bit_libero += 4;  temp <<= 4;  }
                        if ((temp & 0xC000000000000000ULL) == 0) { curr_bit_libero += 2;  temp <<= 2;  }
                        if ((temp & 0x8000000000000000ULL) == 0) { curr_bit_libero += 1; }

                        posizione_piatto_libero += curr_bit_libero;
                        azione_libera = ((piatti_liberi & piatti_dipendenti_positivi[dipendente][blocco]) != 0) ? 1 : 0;
                        
                        break;
                    }
                }
                
                cima_pila++;
                pila[cima_pila].azione = azione_libera;
                pila[cima_pila].posizione = posizione_piatto_libero;
            }
        }
        if (contraddizione) break;
    }

    if (contraddizione) {
        cima_pila = -1;
        memcpy(menu_considerati, ripristino_menu, posizione_max_blocchi_bitmap * sizeof(unsigned long long));
        return 0;
    }

    // ora che ho messo le scelte obbligate devo scorrere su tutti i piatti che non sono mai 
    // citati dai dipendenti, e sui piatti già considerati. L'ordine con cui seguo i piatti è
    // quello delle frequenze

    int posizione_piatto = 0;
    int curr_pos = 0;
    int curr_bit = 0;

    while(ultimo_piatto_considerato < (*piatti_totali)){

        posizione_piatto = frequenza_piatti[ultimo_piatto_considerato]->posizione;
        if(posizione_piatto == -1) return 1;
        curr_pos = posizione_piatto/BITMAP_MAX_SIZE;
        curr_bit = posizione_piatto%BITMAP_MAX_SIZE;


        if( (((MSB_1>>curr_bit) & piatti_da_scartare_totalmente[curr_pos]) != 0) || (((MSB_1>>curr_bit) & menu_considerati[curr_pos]) != 0) ){
            ultimo_piatto_considerato++;
        }else{
            break;
        } 

    }

    if(ultimo_piatto_considerato >= (*piatti_totali)){
        return 1;
    } 
    
    if((piatti_da_scartare_graditi[curr_pos] & (MSB_1>>curr_bit)) == 0){
        cima_pila++;
        pila[cima_pila].posizione = posizione_piatto;
        pila[cima_pila].azione = 1;

        if(risolvi_menu(dipendenti, copia_dipendenti_salvi, numero_dipendenti, piatti_totali, ultimo_piatto_considerato, frequenza_piatti, piatti_da_scartare_totalmente, piatti_da_scartare_graditi, piatti_da_scartare_non_graditi, menu_considerati, menu_scelti, piatti_dipendenti_positivi, piatti_dipendenti_negativi, ric+1)){
            
            return 1;
        }
    }
    
    if((piatti_da_scartare_non_graditi[curr_pos] & (MSB_1>>curr_bit)) == 0){
        cima_pila++;
        pila[cima_pila].posizione = posizione_piatto;
        pila[cima_pila].azione = 0;

        if(risolvi_menu(dipendenti, copia_dipendenti_salvi, numero_dipendenti, piatti_totali, ultimo_piatto_considerato, frequenza_piatti, piatti_da_scartare_totalmente, piatti_da_scartare_graditi, piatti_da_scartare_non_graditi, menu_considerati, menu_scelti, piatti_dipendenti_positivi, piatti_dipendenti_negativi, ric+1)){
            
            return 1;
        }
    }

    memcpy(menu_considerati, ripristino_menu, posizione_max_blocchi_bitmap * sizeof(unsigned long long));
    
    return 0;
}

unsigned int hash_fn(char* parola){

    unsigned int tmp_hash = 0;

    while((*parola)!='\0'){
        tmp_hash = ((tmp_hash<<5) + tmp_hash) + (*parola);
        parola++;
    }
    return tmp_hash;

}

int trova_posizione_piatto(char* parola){

    unsigned int hash = hash_fn(parola);
    // sequenza di ispezione h(k, i) = hash_fn(k) + (i+i^2)/2 mod dim_hash_table
    int i = 0;
    int curr_pos = SEQ_ISP(hash, i, tabella_hash->dimensione);

    while((tabella_hash->tab)[curr_pos]->hash != hash ){
        i++;
        curr_pos = SEQ_ISP(hash, i, tabella_hash->dimensione);
    }

    return (tabella_hash->tab)[curr_pos]->posizione;
}

void inserisci_piatto_in_hash_table(char* parola){
    
    unsigned int hash = hash_fn(parola);
    (tabella_hash->num_elementi)++;
    
    //se vedo che consumo troppa memoria qui posso provare ad aumentare la soglia di riallocazione, perdendo un po' in efficenza temporale
    //devo trovare il giusto compromesso
    if(tabella_hash->num_elementi >= ((tabella_hash->dimensione >> 4))){ 
        rialloca_tabella_hash();
    }

    int i = 0;
    int curr_pos = SEQ_ISP(hash, i, tabella_hash->dimensione);
    
    while((tabella_hash->tab)[curr_pos] != 0){
        i++;
        curr_pos = SEQ_ISP(hash, i, tabella_hash->dimensione);
    }

    //creo un nuovo elemento e lo metto nella tabella
    elemento_hash_table* nuovo; 
    nuovo = (elemento_hash_table*)malloc(sizeof(elemento_hash_table));
    nuovo->hash = hash;
    nuovo->posizione = tabella_hash->num_elementi - 1;
    (tabella_hash->tab)[curr_pos] = nuovo;


}

void rialloca_tabella_hash(){

    //creo la nuova hash table allocando la memoria che mi serve. A ogni riallocazione raddoppio la grandezza
    elemento_hash_table** nuova_tabella = (elemento_hash_table**)calloc((tabella_hash->dimensione)<<1, sizeof(elemento_hash_table*));
    
    TabellaHash* nuova_tabella_hash;
    nuova_tabella_hash = (TabellaHash*) malloc(sizeof(TabellaHash));

    nuova_tabella_hash->tab = nuova_tabella;
    nuova_tabella_hash->dimensione = (tabella_hash->dimensione)<<1;
    nuova_tabella_hash->num_elementi = tabella_hash->num_elementi;

    elemento_hash_table* tmp;
    
    for(int i=0; i<tabella_hash->dimensione; i++){
        
        tmp = (tabella_hash->tab)[i];
        if(tmp != 0){    
            
            int i = 0;
            int curr_pos = SEQ_ISP(tmp->hash, i, nuova_tabella_hash->dimensione);
            
            while((nuova_tabella_hash->tab)[curr_pos] != 0){
                i++;
                curr_pos = SEQ_ISP(tmp->hash, i, nuova_tabella_hash->dimensione);
            }

            (nuova_tabella_hash->tab)[curr_pos] = tmp;
        }
    }
    
    
    free(tabella_hash);
    tabella_hash = nuova_tabella_hash;

}