#include <dht.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

#define BUTTON_OK 6
#define BUTTON_CANCEL 7
#define BUTTON_PREV 8
#define BUTTON_NEXT 9
#define sen A0
#define bec A2

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
dht DHT;

enum Buttons {
  EV_OK,
  EV_CANCEL,
  EV_NEXT,
  EV_PREV,
  EV_NONE,
  EV_MAX_NUM
};

enum Menus {
  MENU_MAIN = 0,
  MENU_START,
  MENU_KP,
  MENU_KI,
  MENU_KD,
  MENU_TEMP,
  MENU_TINCAL,
  MENU_TMEN,
  MENU_TRAC,
  MENU_MAX_NUM
};

struct Parametrii
{
  float Tset = 38;
  float kp = 20;
  float ki = 10;
  float kd = 15;
  int tIncal = 30;
  int tMen = 30;
  int tRac = 15;
};

int modif=0;
unsigned long uptime = -1;
volatile int s = 0, m = 0, h = 0; // secunde, minute, ora
volatile float temperatura, prev_error, setpoint = 35, suma_error, moving_sp;
volatile float kp, ki, kd;
bool flag1 = false;
bool flag2 = false;
bool flag = false;
float dt_inc = 0, dt_rac = 0;
float T1 = 0, T2 = 0;

Menus scroll_menu = MENU_MAIN;
Menus current_menu =  MENU_MAIN;
Parametrii p;  
void state_machine(enum Menus menu, enum Buttons button);
Buttons GetButtons(void);
void print_menu(enum Menus menu);

typedef void (state_machine_handler_t)(void);

void btnInit()
{
  pinMode(6, INPUT);
  digitalWrite(6, LOW); // pull-down

  pinMode(7, INPUT);
  digitalWrite(7, LOW); // pull-down

  pinMode(8, INPUT);
  digitalWrite(8, LOW); // pull-down

  pinMode(9, INPUT);
  digitalWrite(9, LOW); // pull-down

  pinMode(bec, OUTPUT);
}

void timer1()
{
  TCCR0A=(1<<WGM01);   
  OCR0A=0xF9; 
  TIMSK0|=(1<<OCIE0A);
  sei();

  TCCR0B|=(1<<CS01);
  TCCR0B|=(1<<CS00);
}

ISR(TIMER0_COMPA_vect){
  s++;
  if (s == 60)
  {
    m++;
    s = 0;
    if (m == 60)
    {
      h++;
      m = 0;
    }
  }
}

void print_menu(enum Menus menu)
{
  lcd.clear();
  switch(menu)
  {
    case MENU_KP:
      lcd.print("KP = ");
      lcd.print(p.kp);
      break;
    case MENU_KI:
      lcd.print("KI = ");
      lcd.print(p.ki);
      break;
    case MENU_KD:
      lcd.print("KD = ");
      lcd.print(p.kd);
      break;
    case MENU_TEMP:
      lcd.print("Tset = ");
      lcd.print(p.Tset);
      break;
    case MENU_TINCAL:
      lcd.print("tIncal = ");
      lcd.print(p.tIncal);
      break;
    case MENU_TMEN:
      lcd.print("tMen = ");
      lcd.print(p.tMen);
      break;
    case MENU_TRAC:
      lcd.print("tRac = ");
      lcd.print(p.tRac);
      break;
    case MENU_MAIN:
    case MENU_START:
      lcd.print("-----START!-----");
      break;
    default:
      lcd.print("----PrS_2020----");
      break;
  }

  if(current_menu == MENU_START)
  {
    lcd.clear();
    int min = 0;
    int sec = 0;
    int remaining = 0;
    uptime ++;

    int timp_inc = p.tIncal;
    int timp_men = p.tMen;
    int timp_rac = p.tRac;
    float temp = p.Tset;

     if (temperatura > 0)
     {
      lcd.setCursor(11, 0);
      lcd.print(temperatura);
      if (flag1 == false)
      {
        flag1 = true;
        dt_inc = (temp - temperatura) / (timp_inc);
        T1 = temperatura;
      }
    }

    lcd.setCursor(0, 0);
    lcd.print("P: ");
    lcd.print (moving_sp);
    lcd.setCursor(0, 1);
    if (uptime < timp_inc)
    {
      lcd.print("TInc:");
      moving_sp = dt_inc * uptime + T1;
      remaining = timp_inc - uptime;
    }
    else if (uptime <= (timp_inc + timp_men))
    { 
      lcd.print("TMen:");
      remaining = (timp_inc + timp_men) - uptime;
    }
    else if (uptime <= (timp_inc + timp_men + timp_rac))
    { 
      lcd.print("TRac:");
      if (flag2 == false)
      {
        flag2 = true;
        dt_rac = (T1 - temperatura) / (timp_rac);
        T2 = temperatura;
      }
      moving_sp = dt_rac * (uptime - (timp_inc + timp_men)) + T2;
      remaining = (timp_inc + timp_men + timp_rac) - uptime;
    }
    else
    {
      lcd.print("Oprit!");
      flag=true;
    }
    min = remaining / 60;
    sec = remaining % 60;
    if(!flag)
    {
      if(min<10)
      {
        lcd.print("0");
        lcd.print(min);
      }
      else
        lcd.print(min);
      lcd.print(":");
      if(sec<10)
      {
        lcd.print("0");
        lcd.print(sec);
      }
      else
        lcd.print(sec);
    }
    PID();
  }

  if(current_menu != MENU_START && current_menu != MENU_MAIN)
  {
    lcd.setCursor(0,1);
    lcd.print("---Modificare---");
  }

}

void enter_menu(void)
{
  current_menu = scroll_menu;
}

void go_home(void)
{
  scroll_menu = MENU_MAIN;
  current_menu = scroll_menu;
  modif=0;
}

void go_next(void)
{
  scroll_menu = (Menus) ((int)scroll_menu + 1);
  scroll_menu = (Menus) ((int)scroll_menu % MENU_MAX_NUM);
}

void go_prev(void)
{
  scroll_menu = (Menus) ((int)scroll_menu - 1);
  scroll_menu = (Menus) ((int)scroll_menu % MENU_MAX_NUM);
}

//-----------------------
//Constanta proportionala
//-----------------------

void cancel_kp()
{
  p.kp -= modif;
  go_home();
}
void inc_kp(void)
{
  p.kp++;
  modif++;
}
void dec_kp(void)
{
  p.kp--;
  modif--;
}

//-----------------------
//Constanta integratoare
//-----------------------

void cancel_ki()
{
  p.ki -= modif;
  go_home();
}
void inc_ki(void)
{
  p.ki++;
  modif++;
}
void dec_ki(void)
{
  p.ki--;
  modif--;
}

//-----------------------
//Constanta derivatoare
//-----------------------

void cancel_kd()
{
  p.kd -= modif;
  go_home();
}
void inc_kd(void)
{
  p.kd++;
  modif++;
}
void dec_kd(void)
{
  p.kd--;
  modif--;
}

//-----------------------
//Temperatura setata
//-----------------------

void cancel_temp()
{
  p.Tset -= modif;
  go_home();
}
void inc_temp(void)
{
  p.Tset++;
  modif++;
}
void dec_temp(void)
{
  p.Tset--;
  modif--;
}

//-----------------------
//Timpul de incalzire
//-----------------------

void cancel_tIncal(void)
{
  p.tIncal -= modif;
  go_home();
}
void inc_tIncal(void)
{
  p.tIncal++;
  modif++;
}
void dec_tIncal(void)
{
  p.tIncal--;
  modif--;
}

//-----------------------
//Timpul de mentinere
//-----------------------

void cancel_tMen(void)
{
  p.tMen -= modif;
  go_home();
}
void inc_tMen(void)
{
  p.tMen++;
  modif++;
}
void dec_tMen(void)
{
  p.tMen--;
  modif--;
}

//-----------------------
//Timpul de racire
//-----------------------

void cancel_tRac(void)
{
  p.tRac -= modif;
  go_home();
}
void inc_tRac(void)
{
  p.tRac++;
  modif++;
}
void dec_tRac(void)
{
  p.tRac--;
  modif--;
}

//-----------------------
//Salvare modificari
//-----------------------

void save(void)
{
  EEPROM.put(0, p);
  go_home();
}

void ok()
{
  lcd.setCursor(0, 1);
  lcd.print("ok");
}
void todo(void)
{
  lcd.setCursor(0, 1);
  lcd.print("To be continued...");
}

state_machine_handler_t* sm[MENU_MAX_NUM][EV_MAX_NUM] = 
{ //events: OK , CANCEL , NEXT, PREV
  {enter_menu, go_home, go_next, go_prev},           // MENU_MAIN
  {todo, go_home, todo, ok},                         // MENU_START
  {save, cancel_kp, inc_kp, dec_kp},                 // MENU_KP
  {save, cancel_ki, inc_ki, dec_ki},                 // MENU_KI
  {save, cancel_kd, inc_kd, dec_kd},                 // MENU_KD
  {save, cancel_temp, inc_temp, dec_temp},           // MENU_TEMP
  {save, cancel_tIncal, inc_tIncal, dec_tIncal},     // MENU_TINCAL
  {save, cancel_tMen, inc_tMen, dec_tMen},           // MENU_TMEN
  {save, cancel_tRac, inc_tRac, dec_tRac},           // MENU_TRAC
};

void state_machine(enum Menus menu, enum Buttons button)
{
  sm[menu][button]();
}

Buttons GetButtons(void)
{
  enum Buttons ret_val = EV_NONE;
  if (digitalRead(BUTTON_OK))
  {
    ret_val = EV_OK;
  }
  else if (digitalRead(BUTTON_CANCEL))
  {
    ret_val = EV_CANCEL;
  }
  else if (digitalRead(BUTTON_NEXT))
  {
    ret_val = EV_NEXT;
  }
  else if (digitalRead(BUTTON_PREV))
  {
    ret_val = EV_PREV;
  }
  Serial.print(ret_val);
  return ret_val;
}

void setup()
{
  Serial.begin(9600);
  lcd.begin(16,2);
  btnInit();
  EEPROM.get(0, p);
}

void loop()
{
  DHT.read11(sen);
  temperatura = DHT.temperature;
  kp = p.kp;
  kd = p.kd;
  ki = p.ki;
  volatile Buttons event = GetButtons();
  if (event != EV_NONE)
  {
    state_machine(current_menu, event);
  }
  print_menu(scroll_menu);
  delay(1000);
}

void PID()
{
  float error =  moving_sp - temperatura;
  if (error < 3 && error > -3)
    suma_error += error;
  suma_error = constrain(suma_error, -15, 15);
  float diff = (error - prev_error);
  float output = kp * error + ki * suma_error + kd * diff;
  output = constrain(output, 0, 100); //maxim 5.5 V
  prev_error = error;
  //  Serial.print(moving_sp);
  //  Serial.print("  ");
  // Serial.print(error);
  Serial.print(0.05 * output); //tensiune pe bec
  Serial.print(" ");
  Serial.println(temperatura);
  analogWrite(bec, int(output));
}
