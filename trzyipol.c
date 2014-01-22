/*  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author: Michał Stoń
    Copyright (c) 2014
    E-mail: M.Ston (at) mion.elka.pw.edu.pl
            or michal.ston (at) gmail.com
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/fcntl.h>

#define SERV "irc.pirc.pl"
#define PORT 6667
#define NICK "jakisnick"
//#define KANAL "#testtt"
#define KANAL ""
#define WERSJA "-0.45"
#define BUFFSIZE 4096
#define KANMAX 20

int polaczono=0;
int okno=1;
int debug=0;
char wiadomosc[512];
char bufor[512];
char dopisania[512];
char utf[8];
enum enumwiadomosc {MOJA,WIAD,INFO,BLAD,DEBUG} typwiadomosci;
//enum enumwiadomosc typwiadomosci;
struct winsize w;

//char serwid[50];
struct conf{
    int port;
    int gniazdo;
    char serv[64];
    char nick[32];
    char kanaly[KANMAX][40];
    char username[16];
    char realname[64];
} konfig;

int umrzyj();
void setterm();
int polacz();
int sockhandler();
void eschandler();
int iohandler();
int wyslij(char*);
int liniahandler(char*, int);
void irchandler(int, char**, char*);
void wyslijs(char*, char*);
void costamhandler(char*);
void help();
int uscisk(int);
int polacz(char*, int);

int main(int argc, char** argv){
    int i;
    konfig.port = PORT;
    strcpy(konfig.serv,SERV);
    strcpy(konfig.nick,NICK);
    for(i=0;i<20;i++) strcpy(konfig.kanaly[i],"");
    strcpy(konfig.kanaly[1],KANAL);
    strcpy(konfig.realname,"Wujowy klient msichala na zaliczenie");
    strcpy(konfig.username,"trzyipol");

    switch(argc){
        case 4:
            konfig.port=atoi(argv[3]);
        case 3:
            strcpy(konfig.serv,argv[2]);
        case 2:
            if(strstr(argv[1],"-h")){
                help();
                return 0;
            }
            else if(strstr(argv[1],"-g") || strstr(argv[1],"--debug")) debug=1;
            else strcpy(konfig.nick,argv[1]);
            break;
    }
    setterm();

    if((konfig.gniazdo=polacz(konfig.serv,konfig.port))<0) return 1;

    uscisk(0);
    while(1){
        if(sockhandler()) break;
        if(iohandler()) break;
    }
    return 0;
}

int polacz(char *serv, int port){
    int gniazdo;
    struct sockaddr_in adresgniazda;
    struct hostent *he;

    gniazdo = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0);

    if((he = gethostbyname(serv))==NULL){
        puts("Nie można znaleźć adresu!");
        return -1;
    }
    memcpy(&adresgniazda.sin_addr, he->h_addr_list[0], he->h_length);
    adresgniazda.sin_family = AF_INET;
    adresgniazda.sin_port=htons(port);

    printf("Serwer: %s\r\n Port: %d\r\nŁączę...\r\n",serv,port);
    while(connect(gniazdo,(struct sockaddr *) &adresgniazda,sizeof(adresgniazda))){
    }
    puts("Połączono!\r");
    return gniazdo;
}

void wypisz(enum enumwiadomosc typwiadomosci){

    switch(typwiadomosci){
    case MOJA:
        printf("\e[2;m"); //"dim"
        break;
    case INFO:
        printf("\e[36m");
        break;
    case BLAD:
        printf("\e[1;31m");
        break;
    case DEBUG:
        printf("\e[34m");
        break;
    case WIAD:
        printf("\e[33m");
        break;
    default:
        break;
    }

    if(strlen(dopisania)) printf("%s",dopisania);
//    printf("\r\n");
    printf("\e[0m"); //reset atrybutów
    strcpy(dopisania,"");
}
int umrzyj(char* powod){ // TODO: zrobić unsetterm()?
    wyslijs("QUIT :%s",powod);
    printf("\ec\ec");
    sprintf(dopisania,"\nZakończono :%s\n",powod);
    wypisz(BLAD);
    exit(1);
    return 1;
}

void setterm(){
    struct termios *termios_p = malloc(sizeof(struct termios));
    termios_p->c_lflag &= ~(ICANON);
//    termios_p->c_lflag |= ~ICANON;
    termios_p->c_cc[VMIN] = 0;
    termios_p->c_cc[VTIME] = 1;
    if(tcsetattr(0,TCSAFLUSH,termios_p)) puts("nie udało się zmienić opcji terminala.");
    free(termios_p);
    ioctl(0, TIOCGWINSZ, &w); //w.ws_row, w.ws_col
    printf("\e[1;%dr",w.ws_row-1); //scrolling area oprócz ostatniego wiersza
    printf("\e[%d;1H\e[s\e[%d;1H",w.ws_row,w.ws_row-1); //zapisz dol
}

int uscisk(int etap){ //"USER laborka hostname servername :realname\r\nNICK labsichal\r\n");
    char wiadomosc[128];
    if(etap==0){
        sprintf(wiadomosc,"USER %s hn sn :%s",konfig.username,konfig.realname);
        wyslij(wiadomosc);
        sprintf(wiadomosc,"NICK %s",konfig.nick);
        wyslij(wiadomosc);
        polaczono=1;
        return 0;
    }
    return 0;
}

int sockhandler(){ //TEN JUŻ DZIAŁA NA PEWNO
    int n,i=0;
    char bufor[BUFFSIZE];
    char linia[BUFFSIZE];
    n=recv(konfig.gniazdo,bufor,BUFFSIZE-1,MSG_PEEK);
    if(n>0){
        for(i=0;bufor[i]!='\n'&&i<(BUFFSIZE-1);i++);
        n=read(konfig.gniazdo,linia,i+1);
        if(n>0) liniahandler(linia,0);
    }
    return 0;
}
int liniahandler(char *linia, int wymus){ //TODO
    char lin[BUFFSIZE];
    strcpy(lin,linia);
    int i=1;
    char *ost;
    char *tmp;
    char *gdzie;
    char *arg[5];
    if(linia[0]==':' || wymus==1){

        if(strstr(linia," :")){
            ost=strstr(linia," :")+2;
            tmp=strstr(ost,"\n");
            if(tmp) *(tmp+1)='\0';
        }
        else ost=NULL;

        gdzie=strstr(lin+5,":");
        arg[0]=strtok(lin," ");
        while(((((arg[i]=strtok(NULL," ")) < gdzie) || !(ost) ) && arg[i]!=NULL) || i<1) i++;
//        if(arg[0][0]==':' && polaczono==2){
//            strcpy(serwid,arg[0]);
//            printf("\nserwid= %s\n",serwid);
//            polaczono=3;
//        }

        irchandler(i,arg,ost);
        if(wymus==1) wymus=0;
    }
    else if(linia[0]>'A' && linia[0]<'Z') costamhandler(linia);
    else strcpy(linia,"");
    return 0;
}

void irchandler(int argc, char *arg[5], char *ost){ //TODO
    int i,cmd;
    char nick[35];
    char *gdzie=0;

void hajlajt(void){
    printf("\e[1;32m");
}

if(argc){
    cmd=atoi(arg[1]);
    if((gdzie=strstr(arg[0],"!")) && arg[0]+32>gdzie) strncpy(nick,arg[0]+1,(int) (gdzie-arg[0]-1));

    if(debug){
        for(i=0;i<argc;i++){ sprintf(bufor,"arg%d: %s ",i,arg[i]); strcat(dopisania,bufor); }
        if(ost){ sprintf(bufor,"ost: %s",ost); strcat(dopisania,bufor); }
        wypisz(4);
    }

    if(!strncmp(arg[0],"PING",4)) wyslijs("PONG :%s",ost);
    else if(!strncmp(arg[1],"JOIN",4)){
        strtok(ost,"\r\n");
        if(!strcmp(nick,konfig.nick)) strcpy(konfig.kanaly[okno],ost);
        sprintf(dopisania,"%15s %s wszedł na  %s.\r\n","-->",nick,ost);
        wypisz(INFO);
    }
    else if(!strncmp(arg[1],"PART",4)){
        strtok(ost,"\r\n");
        sprintf(dopisania,"%15s %s wyszedł z  %s (%s).\r\n","<--",nick,arg[2],ost);
        wypisz(INFO);
        if(!strcmp(nick,konfig.nick)) strcpy(konfig.kanaly[okno],"");
    }
    else if(!strncmp(arg[1],"NICK",4)){
        strtok(ost,"\r\n");
        if(!strcmp(nick,konfig.nick)) strcpy(konfig.nick,ost);
        sprintf(dopisania,"%s zmienił nick na %s",nick,ost);
        wypisz(INFO);
    }
    else if(!strncmp(arg[1],"MODE",4)){
        strtok(ost,"\r\n");
        sprintf(dopisania,"%s ustawił tryb %s %s",(char*) arg[0]+1,ost,arg[2]);
        wypisz(INFO);
    }
    else if(!strncmp(arg[1],"332",3)){
        sprintf(dopisania,"Temat na %s to: %s",arg[3],ost);
        wypisz(INFO);
    }
    else if(!strncmp(arg[1],"353",3)){
        sprintf(dopisania,"Użytkownicy na %s: %s",arg[4],ost);
        wypisz(INFO);
    }
    else if(!strncmp(arg[1],"404",3)){
        sprintf(dopisania,"Nie można wysłać wiadomości: %s",ost);
        wypisz(INFO);
    }
    else if(!strncmp(arg[1],"433",3) || !strncmp(arg[1],"475",3) || !strncmp(arg[1],"461",3)){
        sprintf(dopisania,"%s: %s",arg[3],ost);
        wypisz(INFO);
    }
//    else if(!strcmp(arg[0],serwid)){
//        printf("%s: %s",arg[1],ost);
//    }
    else if(ost && ost[0]==1){
        strtok(ost,"\r\n");
        sprintf(dopisania,"Otrzymano CTCP %s od %s\r\n",ost,nick);
        wypisz(INFO);
        if(strstr(ost,"VERSION")) wyslijs("NOTICE %s :VERSION ZaliczenIRC, wersja "WERSJA,nick);
    }
    else if(!strcmp(arg[1],"PRIVMSG")){
        if(strstr(ost,konfig.nick)) hajlajt();
        sprintf(bufor,"%s%s%s","<",nick,">");
        sprintf(dopisania,"%15s %s",bufor,ost);
        wypisz(WIAD);
    }
    else if(!strcmp(arg[1],"NOTICE")){
        sprintf(dopisania,"%15s %s",bufor,ost);
        wypisz(INFO);
    }
    else if(!strncmp(arg[1],"376",3)){ //koniec MOTD
        for(i=0;i<KANMAX;i++) { //joinuj jesli ustawione
            if(strlen(konfig.kanaly[i])>1){
                sprintf(wiadomosc,"JOIN %s\r\n",konfig.kanaly[i]);
                wyslij(wiadomosc);
            
                if(debug){
                    strcpy(dopisania,wiadomosc);
                    wypisz(DEBUG);
                }
            }
        polaczono=2;
    }
    }


    else if(!debug){
        if(cmd){
            if((cmd>=372 && cmd<=376) || (cmd>=251 &&cmd<=266) || cmd==421 || cmd==475 || cmd==433) sprintf(dopisania,"%s",ost);
            wypisz(BLAD);
        }
        else{
          //  for(i=0;i<argc;i++) { sprintf(bufor,"arg%d: %s ",i,arg[i]); strcat(dopisania,bufor); }
            //if(ost){ sprintf(bufor,"ost: %s",ost); strcat(dopisania,bufor); }
            //wypisz(DEBUG);
        }
    }

}
}

void costamhandler(char *linia){
    if(!strncmp(linia,"PING",4)) liniahandler(linia,1);
    else if(!strncmp(linia,"ERROR :Closing Link: ",20)) umrzyj("Serwer zakończył połączenie");
    else printf("?: %s",linia);
}

void eschandler(){ //TODO
    if(getchar()=='[') switch(getchar()){
        case '5':
            if(getchar()=='~'){}// printf("\eM"); //NIE DZIALA I TAK
            break;
        case '6':
            if(getchar()=='~'){}// printf("\eD");
            break;
        default:
            break;
    }
}

void cmdhandler(char *cmd){ //TODO
    if(!strncmp(cmd,"debug",strlen("debug"))){
        debug ^= 1;
        sprintf(dopisania,"tryb debugowania=%d\r\n",debug);
        wypisz(DEBUG);
    }
    else wyslij(cmd);
}

void wiadhandler(char *wiad){ //TODO
    char *wiadomosc=malloc(strlen(wiad)+40);
    sprintf(wiadomosc,"PRIVMSG %s :%s",konfig.kanaly[okno],wiad);
    wyslij(wiadomosc);
    if(debug){
        strcpy(dopisania,wiadomosc);
        strcat(dopisania," a kanal=");
        strcat(dopisania,konfig.kanaly[okno]);
        strcat(dopisania," a\r\n");
        
        wypisz(DEBUG);
    }
    //kursor(1);
    sprintf(bufor,"%s%s%s","<",konfig.nick,">");
    sprintf(dopisania,"%15s %s\r\n",bufor,wiad);
    wypisz(MOJA);
    free(wiadomosc);
    //kursor(0);
}

int utfhandler(char c){
    int i,j;
    for(i=0;c&(0x80>>i);i++); //ile jedynek z porzodu
    if(i>6) return 0; //coś jest nie tak
    utf[0]=c;
    for(j=1;j<i && j<6 ;j++) utf[j]=getchar();
    utf[j]='\0';
    return strlen(utf);
}
int iohandler(){ //TODO'
    static int i;
    int x;
    char c=0;
    static int j;
    if(j==0) strcpy(wiadomosc,"");
    if((unsigned char) (c=getchar())<0xff && c){
        printf("\e[u");

        if(c==3) return umrzyj("Użyto ^C");
        if((unsigned char) c>=0xc2 && (unsigned char) c<=0xf4){
            if((i=utfhandler(c))){
                strcat(wiadomosc,utf);
                printf("%s",utf);
                j+=i;
            }
            else i=1;
        }
        if(c=='\e') eschandler();

        if(c==127 && j>0){
            for(x=1;wiadomosc[j-x]<0 && ((wiadomosc[j-x]&0xc0)!=0xc0);x++);
            printf("\b \b");
            if(j>0) j-=x;
            else j=0;
//            printf("-%d",x);
        }
        if(c>31 && c<127){ wiadomosc[j++] = c; putchar(c); }
        if(c==13){
            wiadomosc[j]='\0';
            printf("\e[%d;1f\e[s\e[2K",w.ws_row); //wyczysc ostatnia linijke
            printf("\e[%d;1f",w.ws_row-1); //idz z powrotem
            if(wiadomosc[0]=='/') cmdhandler(wiadomosc+1);
            else if(strlen(wiadomosc)>0) wiadhandler(wiadomosc);
            j=0;
        }
//        else if(c=='\n' || c=='\r') putchar(c);
        else printf("\e[s");
        wiadomosc[j]='\0';

    }
//    printf("\e[%d,1H",w.ws_row-3);
    printf("\e[%d;1f",w.ws_row-1); //idz z powrotem

    return 0;
}

int wyslij(char *wiad){
    char *wiadomosc=malloc(strlen(wiad)+20);
    strcpy(wiadomosc,wiad);

    strcat(wiadomosc,"\r\n");
//    printf("\n< %s",wiadomosc);
    if(write(konfig.gniazdo,wiadomosc,strlen(wiadomosc))){
        free(wiadomosc);
        return 0;
    }
    else free(wiadomosc);
    return 1;
}
void wyslijs(char *format, char *string){
    char *bufor = malloc(strlen(format)+strlen(string)+5);
//    char bufor[512];
    sprintf(bufor,format,string);
    wyslij(bufor);
    strcpy(dopisania,bufor);
    if(debug) wypisz(DEBUG);
    free(bufor);
}

void help(void){
    printf("TrzyiPół,\n\
klient IRC trochę z nudów i trochę na zaliczenie.\
\nWersja %s, skompilowano %s, %s\n\n\
Użycie:\n./trzyipol [nick [serwer [port]]\n\n\
./trzyipol -h: pomoc (to, co czytasz)\n\
./trzyipol -g lub --debug: tryb debugowania domyślnie włączony\n\n\
© Michał Stoń 2013\n\
michal.ston@gmail.com\nGNU-GPL v3.0\n\
http://home.elka.pw.edu.pl/~mston/trzyipol/info.php\n",WERSJA,__DATE__,__TIME__);
}

