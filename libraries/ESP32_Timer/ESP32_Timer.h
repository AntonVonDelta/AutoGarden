#include <time.h>

class ESP32_Timer{
			
	// Cronometru merge ....pompa se deschide dupa program
	// Overwrite		....pompa e deschisa indiferent de cronometru pentru un numar de minute. Cronometrul merge in fundal
	//   Utilizatorul apasa pe Stop..... se sterge Overwrite-ul.....cronometrul continua
	//   Utilizatorul apasa pe Stop(Overwrite-ul nu e setat).....se opreste cronometrul (dar NU se reseteaza)
	//   Acum textul butonului se schimba si scrie Porneste(porneste cronometrul)
	// Salveaza va rescrie limitele de timp. Asa ca daca limitele sunt mai mici decat timpul care a trecut deja se efectueaza actiunea.

	int timeouts[3]={0,60,0};		// The index is OPENED, CLOSED, CHRON_OVERWRITE
	time_t start_time;
	time_t temp_chron_start;	// Used for measuring temporarily chron timer(that is secondary timer which overwrites the primary)
	time_t pause_time;

	public:
		enum STATE{
			NORMAL=0,
			OVERWRITE=1,		// Inseamna ca nu mai este folosit contorul intern ci unul definit de utilizator = deschide temporal pompa
			PAUSED=2			// Daca timerul e oprit. La reluare se va continua de acolo de unde a fost lasat insa nu se va intoarce in modul OVERWRITE ci NORMAL
		};
		enum SOCKET_STATE{
			OPENED=0,
			CLOSED=1
		};	// Electrical socket that is

		STATE timer_state=NORMAL;
		SOCKET_STATE socket_state=CLOSED;
		bool timer_started=false;

		void Init(){
			timer_started=false;
			timer_state=NORMAL;
			socket_state=CLOSED;
			time(&start_time);
		}
		void Start(){
			timer_started=true;
			timer_state=NORMAL;
			socket_state=CLOSED;
			time(&start_time);
		}
		void SyncSysTime(time_t new_time){
			int64_t diff=new_time-time(0);
			start_time+=diff;
			temp_chron_start+=diff;
			pause_time+=diff;
		}

		void Continue(){
			// Can continue if in overwrite or pause mode
			if(timer_state==NORMAL) return;
			if(timer_state==OVERWRITE){
				timer_state=NORMAL;
				return;
			}
			time_t now;
			time(&now);

			timer_state=NORMAL;
			start_time+=now-pause_time;
		}
		void Pause(){
			if(timer_state==PAUSED) return;	// Should not pause twice because we will lose time between pauses

			timer_state=PAUSED;
			time(&pause_time);
		}
		void StartTempChron(int timeout){
			if(timer_state==PAUSED) return;	// Can't use CHRON_OVERWRITE when in pause

			timeouts[2]=timeout;
			timer_state=OVERWRITE;
			time(&temp_chron_start);
		}


		void setTimeouts(int input_open_time,int input_close_time){
			timeouts[0]=input_open_time;
			timeouts[1]=input_close_time;
		}

		void Process(){
			if(!timer_started) return;
			if(timer_state==PAUSED) return;

			time_t now;
			time(&now);

			if(timer_state==OVERWRITE){
				if(difftime(now,temp_chron_start)>=timeouts[2]){
					timer_state=NORMAL;
				}
			}

			if(difftime(now,start_time)>=timeouts[socket_state]){
				if(socket_state==OPENED) socket_state=CLOSED;
				else socket_state=OPENED;
				start_time=now;
			}
			
		}

		SOCKET_STATE getPrimaryClockState(){
			return socket_state;
		}

		// 0-opened 1-closed
		SOCKET_STATE getSocketState(){
			if(timer_state==PAUSED){
				return CLOSED;
			}
			if(timer_state==OVERWRITE) return OPENED;
			return socket_state;
		}
		STATE getState(){
			return timer_state;
		}

		double getRemainingTime(bool for_overwrite){
			time_t now;
			time(&now);

			if(for_overwrite){
				if(timer_state!=OVERWRITE) return 0;
				return max(timeouts[2]-difftime(now,temp_chron_start),(double)0);
			}

			if(timer_state==PAUSED){
				double passed_time=difftime(pause_time,start_time);
				return max(timeouts[socket_state]-passed_time,(double)0);	
			}else{
				double passed_time=difftime(now,start_time);
				return max(timeouts[socket_state]-passed_time,(double)0);	
			}
		
		}
};
