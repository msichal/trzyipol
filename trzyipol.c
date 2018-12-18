#include <string.h>
#include <stdint.h>

#define MAX_MSGLEN 512
#define MSG_CIRCBUFF_LEN 512
#define MAX_WINDOWS 8
#define MAX_CHANNAME 64 // also query username

#define VT100_HOME(r,c) printf("\e["#r";"#c"H")

enum winstate{
	winstate_closed = 0,
	winstate_active,
	winstate_empty,
	winstate_inactive
}

enum wintype{
	wintype_unknown = -1,
	wintype_status,
	wintype_channel,
	wintype_query,
}

enum winact{
	winact_idle = 0,
	winact_newmsg,
	winact_newstatus,
	winact_highlight
}

typedef struct {
	char msgs[MSG_CIRCBUFF_LEN][MAX_MSGLEN];
	uint16_t first, last, len;
} msg_circbuff_t;

struct status;

typedef struct {
	msg_circbuff_t scrollback;
	char msg[MAX_MSGLEN];
	enum winstate state;
	enum winact act;
	int id;

	struct status *status;
} window_t;

typedef struct {
	uint16_t active;
	uint16_t last_ping;
	window_t windows[MAX_WINDOWS]; // please, Microsoft, don't sue me

	struct status *status;
} connection_t;

struct{
	struct winsize w;

	msg_circbuff_t msghistory;

	connection_t conn;
} status;

typedef struct status status_t;

void init(status_t *status)
{
	memset(status, 0, sizeof(*status));

	int i;

	for(i=0;i<MAX_WINDOWS; i++)
		status->conn.windows[i].id=i;

}


int checkwinsize(struct winsize *ww){
	struct winsize w; //ws_row, ws_col
	ioctl(0, TIOCGWINSZ, &w);
	*ww = w;

	if(w.ws_row < 24 || w.ws_col < 80) // minimal viable size
		return -1;

	return 0;
}


void draw(status_t *s, enum wintype type)
{
	char line [200] = {' ',};
	int breaks[16], nbreak = 0;
	struct winsize w; //ws_row, ws_col
	if(checkwinsize(*w) < 0)
		return;

	char list[3][64] = {"irc.pirc.pl/status", "#www.elektroda.pl", "ktostam"};
	VT100_HOME(0,0);
	int chars = 0, bytes = 0;
	int i;

	bytes += sprintf(line, "\u2551"); // '|'' on start
	chars++;
	breaks[nbreak++] = chars;
	for(i = 0; i < 3; i++){ // windows
		bytes += sprintf(line+cnt, "%s \u2551", list[i]);
		chars += strlen(list[i]) + 2;
		breaks[nbreak++] = chars;
	}

	// bytes += snprintf(line + w.ws_col - bytes, "\u2551");
	
	// while(chars < w.ws_col){
	// 	bytes += snprintf(line+cnt, "\u2551");
	// 	chars++;
	// }
	printf("%.*s", bytes, line);

	VT100_HOME(1,0);
	chars = 0 ; bytes = 0;
	while(chars < w.ws_col){
		bytes += sprintf(line+bytes, "\u2550")
	}
	for(i = 0; i < nbreak - 1; i++){
		
	}

}



//irc.pirc.pl/status | www.elektroda.pl | ktostam
//-------------------------------------------------



------
nick |





