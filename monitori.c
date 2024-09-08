#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define BROJ_CITACA 10
#define BROJ_PISACA 3
#define BROJ_BRISACA 1

typedef struct Node{
    int vrijednost;
    struct Node* iduci;
}Node;


Node *head;

int broj_citaca_ceka = 0, broj_citaca_cita = 0, broj_brisaca_ceka = 0, broj_brisaca_brise = 0, broj_pisaca_ceka = 0, broj_pisaca_pise = 0;

pthread_mutex_t monitor;
pthread_cond_t citaci, brisaci, pisaci;

int velicina(){
    Node* head_copy = head;
    int result = 0;
    while(head_copy!=NULL){
        head_copy=head_copy->iduci;
        result++;
    }
    return result;
}

void prikaz_aktivnih(){
    printf("Aktivnih: citaca: %d pisaca: %d brisaca: %d\n", broj_citaca_cita, broj_pisaca_pise, broj_brisaca_brise);
    printf("Lista (%d): ", velicina());
    Node* head_copy = head;
    while(head_copy!=NULL){
        printf("%d ", head_copy->vrijednost);
        head_copy=head_copy->iduci;
    }
    printf("\n");
}


int procitaj_element(int index){
    Node* head_copy = head;
    while(index!=0){
        head_copy=head_copy->iduci;
        index--;
    }
    return head_copy->vrijednost;
}

void obrisi(int index){
    Node* head_copy = head;

    if (index == 0){
        head = head->iduci;
        free(head_copy);
    } else {
        while(index!=1){
            head_copy = head_copy->iduci;
            index--;
        }
        Node* pomocna = head_copy->iduci;
        head_copy->iduci = pomocna->iduci;
        free(pomocna);
    }
}

void zapisi(int vrijednost){
    Node* novi = malloc(sizeof(Node));
    novi->vrijednost = vrijednost;
    novi->iduci = NULL;
    if(head==NULL){
        head=novi;
    }else{
        Node* head_copy = head;
        while(head_copy->iduci!=NULL){
            head_copy = head_copy->iduci;
        }
        head_copy->iduci = novi;
    }
}

int getVrijednost(int index){
    Node* head_copy = head;
    while(index>0){
        head_copy = head_copy->iduci;
        index--;
    }
    return head_copy->vrijednost;
}


void *citac(void *arg) {
    int I = *(int *)arg;
    while (1==1) {
        int index = rand() % velicina();
        pthread_mutex_lock(&monitor);
        printf("citac %d želi čitati element %d liste\n", I, index);
        prikaz_aktivnih();
        broj_citaca_ceka++;
        
        while(broj_brisaca_brise + broj_brisaca_ceka > 0){
            pthread_cond_wait(&citaci, &monitor);
        }

        broj_citaca_cita++;
        broj_citaca_ceka--;

        if(index>=velicina()){
            printf("citac %d: %d je obrisan dok je cekao, ne moze ga procitat\n", I, index);
        } else{
            int y = procitaj_element(index);
            printf("citac %d cita element %d liste (vrijednost %d)\n", I, index, y);
            prikaz_aktivnih();
        
            pthread_mutex_unlock(&monitor);
            
            sleep(rand() % 4 + 3); 
            
            pthread_mutex_lock(&monitor);
        }
        broj_citaca_cita--;
        if (broj_brisaca_ceka > 0) {
            pthread_cond_broadcast(&brisaci);
        }
        printf("citac %d vise ne koristi listu\n", I);
        prikaz_aktivnih();
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 2 + 3);
    }
    return NULL;
}


void *pisac(void *arg) {
    int I = *(int *)arg;
    while (1==1) {

        pthread_mutex_lock(&monitor);

        int vrijednost = rand()%100;
        printf("pisac %d zeli dodati element %d\n", I, vrijednost);
        prikaz_aktivnih();

        broj_pisaca_ceka++;
        while (broj_brisaca_brise > 0 || broj_brisaca_ceka > 0 || broj_pisaca_pise > 0) {
            pthread_cond_wait(&pisaci, &monitor);
        }
        broj_pisaca_ceka--; 
        broj_pisaca_pise++;

        printf("pisac %d krece zapisivati %d\n", I, vrijednost);
        prikaz_aktivnih();
        pthread_mutex_unlock(&monitor);
        
        sleep(rand() % 4 + 3); 
        
        pthread_mutex_lock(&monitor);

        zapisi(vrijednost);
        printf("pisac %d je zapisao %d\n", I, vrijednost);
        prikaz_aktivnih();

        broj_pisaca_pise--;
        if(broj_brisaca_ceka > 0){
            pthread_cond_broadcast(&brisaci);
        } else if (broj_pisaca_ceka > 0) {
            pthread_cond_broadcast(&pisaci);
        }
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 2 + 3); 
    }
    return NULL;
}

void *brisac(void *arg) {
    int I = *(int *)arg;
    while (1==1) {
        pthread_mutex_lock(&monitor);

        int index = rand() % velicina();
        int vrijednost = getVrijednost(index);
        
        printf("brisac %d zeli obrisati element %d (vrijednost=%d)\n", I, index, vrijednost);
        prikaz_aktivnih();

        broj_brisaca_ceka++;
        while (broj_pisaca_pise > 0 || broj_citaca_cita > 0 || broj_brisaca_brise > 0) {
            pthread_cond_wait(&brisaci, &monitor);
        }
        broj_brisaca_ceka--; 
        broj_brisaca_brise++;

        printf("brisac %d zapocinje s brisanjem elementa %d liste (vrijednost=%d)\n", I, index, vrijednost);
        prikaz_aktivnih();

        pthread_mutex_unlock(&monitor);
        
        sleep(rand() % 4 + 3); 
        
        pthread_mutex_lock(&monitor);
        
        obrisi(index);
        printf("brisac %d je obrisao element %d (vrijednost=%d)\n", I, index, vrijednost);
        prikaz_aktivnih();

        broj_brisaca_brise--;
        if (broj_pisaca_ceka > 0) {
            pthread_cond_broadcast(&pisaci);
        }
        if (broj_citaca_ceka > 0) {
            pthread_cond_broadcast(&citaci);
        }
        pthread_mutex_unlock(&monitor);

        sleep(rand() % 4 + 3); 
    }
    return NULL;
}

int main() {
    srand(time(NULL));

    head = NULL;
    pthread_t citac_dretve[BROJ_CITACA], pisac_dretve[BROJ_PISACA], brisac_dretve[BROJ_BRISACA];
    int citac_dretve_id[BROJ_CITACA], pisac_dretve_id[BROJ_PISACA], brisac_dretve_id[BROJ_BRISACA];

    for (int i = 0; i < BROJ_PISACA; i++) {
        pisac_dretve_id[i] = i;
        pthread_create(&pisac_dretve[i], NULL, pisac, &pisac_dretve_id[i]);
    }

    sleep(15);

    for (int i = 0; i < BROJ_CITACA; i++) {
        citac_dretve_id[i] = i;
        pthread_create(&citac_dretve[i], NULL, citac, &citac_dretve_id[i]);
    }

    sleep(2);

    for (int i = 0; i < BROJ_BRISACA; i++) {
        brisac_dretve_id[i] = i;
        pthread_create(&brisac_dretve[i], NULL, brisac, &brisac_dretve_id[i]);
    }


    for (int i = 0; i < BROJ_CITACA; i++) {
        pthread_join(citac_dretve[i], NULL);
    }
    for (int i = 0; i < BROJ_PISACA; i++) {
        pthread_join(pisac_dretve[i], NULL);
    }

    for (int i = 0; i < BROJ_BRISACA; i++) {
        pthread_join(brisac_dretve[i], NULL);
    }
    return 0;
}