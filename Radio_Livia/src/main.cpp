/*
    Radio Educativo 
    Feito como presente de aniversário para Livia
    Projeto por Fábio Souza
    Dezembro de 2019

    Licença MIT

*/

//bibliotecas
#include "Arduino.h"
#include "SoftwareSerial.h"
#include "DFRobotDFPlayerMini.h"
#include "U8glib.h"
#include <EEPROM.h>

#define MAX_MUSICAS 133
#define MAX_HISTORIAS 20
#define MAX_SONS  53
#define MAX_NINAR 24

#define TIMEOUT_BT 50
#define TIMEOUT_BT_MODO 75
#define TIMEOUT_BT_NEXT 100
#define TIMEOUT_BT_PREV 100
#define TIMEOUT_BT_START 100

#define END_ULTIMA_MUSICA 0

//constantes
const byte bt_NEXT  = 2 ;
const byte bt_PREV  = 3;
const byte bt_START = 4;
const byte bt_MODO  = 5;
const byte BUSY_PIN = 6;
const byte BUZZER_PIN = 9;
const byte POT = A0;

//Enumeração para controle do funcionamento
enum estado{
  STOP = 0,
  PLAY,
  PAUSE
} estadoAtual;

//Enumeração para menu
enum modo {
  MUSICAS=1,
  HISTORIAS,
  JOGO,
  DORMIR
}modoOperacao;

//inicialização do DFplayer
SoftwareSerial mySoftwareSerial(11, 10); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

//iniicialização do OLED
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

//variáveis auxiliares
byte volume = 0;
byte lastVolume;
byte faixa = 1;
byte valores_sorteados[MAX_SONS];
byte indice_sorteio = 0;

//funções
void beep(void){
  digitalWrite(BUZZER_PIN,HIGH);
  delay(30);
  digitalWrite(BUZZER_PIN,LOW);
  delay(30);
}

void initHardware(){
    //configura pinos dos botões como entradas com resistor de pullup habilitado
    for(int i = 2;i<7;i++){
    pinMode(i,INPUT_PULLUP);
  }

  pinMode(BUZZER_PIN,OUTPUT);

  //Inicializa a serial do Arduino
  Serial.begin(115200);

  //Configura DISPLAY OLED
  u8g.setColorIndex(1);         // pixel on
  u8g.setFont(u8g_font_unifont);

  beep();
}

void initDFplayer(){
  //Comunicacao serial com o DFplayer
  mySoftwareSerial.begin(9600);
 

  //setup do DFplayer
  //Verifica se o modulo esta respondendo e se o
  //cartao SD foi encontrado
  if (!myDFPlayer.begin(mySoftwareSerial))
  {
    #ifdef DEBUG 
    Serial.println(F("Nao inicializado:"));
    #endif
    while (true);
  }
 
  //Definicoes iniciais parao DFplayer
  myDFPlayer.setTimeOut(500); //Timeout serial 500ms
  myDFPlayer.volume(0); //Volume 5
  myDFPlayer.EQ(0); //Equalizacao normal
  
}


void draw(void)
{
    //Write text. (x, y, text)
    switch(modoOperacao){
      case MUSICAS:
        u8g.setPrintPos(30, 20);
        u8g.print(" MUSICAS");
      break;
      case HISTORIAS:
        u8g.setPrintPos(30, 20);
        u8g.print("HISTORIAS");
      break;
      case JOGO:
        u8g.setPrintPos(45, 20);
        u8g.print("JOGO");
      break;

      case DORMIR:
        u8g.setPrintPos(10, 20);
        u8g.print("HORA DE DORMIR");
      break;
    }
    u8g.setPrintPos(55, 40);
    u8g.print(faixa);
    u8g.setPrintPos(70, 60);
    u8g.print("VOL:");
    u8g.print(volume);
    

    u8g.setPrintPos(5, 60);
    switch(estadoAtual){
      case STOP:
        u8g.print("      ");
      break;

      case PLAY:
        u8g.print("TOCANDO"); 
      break;

      case PAUSE:
        u8g.print("PAUSA");
      break;
    }
}

/*Faz média das leituras*/
int adRead(byte entrada){

  int valor = 0;
  for(int i=0;i<4;i++){
    valor +=analogRead(entrada);
  }
  valor = valor>>2; //divide por 4
  return valor;

}

void setup()
{
  initHardware();   //configura hw
  initDFplayer();   //configura DFplayer

  //inicializa enumerações
  modoOperacao = MUSICAS;
  estadoAtual = STOP;

  byte v = EEPROM.read(END_ULTIMA_MUSICA);
  if(v == 0 || v == 255)
  {
    faixa = 1;
  }
  else
  {
    faixa = v;
  }
  

}
void ajustaVolume(void)
{
  volume = map(adRead(POT), 1, 1000, 0, 30);
  if(volume != lastVolume)
  {
    lastVolume = volume;
    myDFPlayer.volume(volume);
  }
}

void trataBtModo(void)
{
  if(digitalRead(bt_MODO)== LOW)
   {
      int contador = 0;
      while((digitalRead(bt_MODO)== LOW) & (contador <TIMEOUT_BT_MODO))
      {
        delay(10);
        contador++;
      }

      if(contador >=TIMEOUT_BT_MODO)
      {
        beep();
        myDFPlayer.pause();
        faixa = 1;
        estadoAtual = STOP;
        delay(100);
        myDFPlayer.playFolder(5, 1);
        switch(modoOperacao){
          case MUSICAS:
        
           modoOperacao = HISTORIAS;
          break;

          case HISTORIAS:
          randomSeed(analogRead(1));
           modoOperacao = JOGO;
           for(int i = 0;i<MAX_SONS;i++){
              valores_sorteados[i]=0;
            }
          break;

          case JOGO:
            modoOperacao = DORMIR;
          break;

          case DORMIR:
              modoOperacao = MUSICAS;
              byte v = EEPROM.read(END_ULTIMA_MUSICA);
              if(v == 0 || v == 255)
              {
                faixa = 1;
              }
              else
              {
                faixa = v;
              }
          break;
        }
        delay(300);
        while(digitalRead(bt_START)==LOW); 
      }
   }

}

void trataBtNext()
{
    if(digitalRead(bt_NEXT)== LOW){
      int contador = 0;
      switch (modoOperacao)
      {
          case MUSICAS:

          while((digitalRead(bt_NEXT)== LOW) & (contador <TIMEOUT_BT_NEXT)){
            delay(10);
            contador++;
          }

          if(contador < TIMEOUT_BT_NEXT){
            beep();
            if(faixa<MAX_MUSICAS+1) faixa++;
            if(faixa == MAX_MUSICAS +1 )faixa = 1;
            if((estadoAtual == PLAY) || (estadoAtual == PAUSE)){
              myDFPlayer.playFolder(modoOperacao, faixa);
              EEPROM.write(END_ULTIMA_MUSICA, faixa); 
              estadoAtual = PLAY;
            }
          }
          
          break;

          case HISTORIAS:

          while((digitalRead(bt_NEXT) == LOW) & (contador <TIMEOUT_BT_NEXT)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_NEXT){
            beep();
            if(faixa<MAX_HISTORIAS+1) faixa++;

            if(faixa == MAX_HISTORIAS +1 )faixa = 1;

            if((estadoAtual == PLAY) || (estadoAtual == PAUSE)){
              myDFPlayer.playFolder(modoOperacao, faixa); 
              estadoAtual = PLAY;
            }

          }
          break;

          case JOGO:
          while((digitalRead(bt_PREV)== LOW) & (contador <TIMEOUT_BT_PREV)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_PREV){
             beep();
             myDFPlayer.playFolder(modoOperacao, faixa); 
             delay(100);
          }
          break;

          case DORMIR:

          while((digitalRead(bt_NEXT)== LOW) & (contador <TIMEOUT_BT_NEXT)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_NEXT){
            beep();
            if(faixa<MAX_NINAR+1) faixa++;

            if(faixa == MAX_NINAR +1 )faixa = 1;

            if((estadoAtual == PLAY) || (estadoAtual == PAUSE)){
              myDFPlayer.playFolder(modoOperacao, faixa); 
              estadoAtual = PLAY;
            }

          }

          break;
      }
        delay(300);
        while(digitalRead(bt_NEXT)==LOW); 
  }

}

void trataBtPrev(void)
{
  if(digitalRead(bt_PREV)== LOW){
      int contador = 0;
      switch (modoOperacao)
      {
          case MUSICAS:

          while((digitalRead(bt_PREV)== LOW) & (contador <TIMEOUT_BT_PREV)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_PREV){
            beep();
            if(faixa>0) faixa--;
            if(faixa == 0)faixa = MAX_MUSICAS;

            if((estadoAtual == PLAY) || (estadoAtual == PAUSE)){
              myDFPlayer.playFolder(modoOperacao, faixa); 
              EEPROM.write(END_ULTIMA_MUSICA, faixa); 
              estadoAtual = PLAY;
            }
          }
          break;

          case HISTORIAS:

          while((digitalRead(bt_PREV) == LOW) & (contador <TIMEOUT_BT_PREV)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_PREV){
            beep();
            if(faixa>0) faixa--;
            if(faixa == 0)faixa = MAX_HISTORIAS;

            if((estadoAtual == PLAY) || (estadoAtual == PAUSE)){
              myDFPlayer.playFolder(modoOperacao, faixa); 
               estadoAtual = PLAY;
            }


          }
          break;

          case JOGO:

          while((digitalRead(bt_PREV)== LOW) & (contador <TIMEOUT_BT_PREV)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_PREV){
             beep();
             myDFPlayer.playFolder(modoOperacao, faixa); 
             delay(100);
          }

          break;

          case DORMIR:

          while((digitalRead(bt_PREV)== LOW) & (contador <TIMEOUT_BT_PREV)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_PREV){
            beep();
            if(faixa>0) faixa--;
            if(faixa == 0)faixa = MAX_NINAR;

            if((estadoAtual == PLAY) || (estadoAtual == PAUSE)){
              myDFPlayer.playFolder(modoOperacao, faixa); 
              estadoAtual = PLAY;
            }
          }
          break;
      }
      delay(300);
      while(digitalRead(bt_PREV)==LOW); 
  }
}

void  trataBtStart()
{
  if(digitalRead(bt_START)== LOW){
    int contador = 0;
    switch (modoOperacao)
    {
      case MUSICAS:

        while((digitalRead(bt_START)== LOW) & (contador <TIMEOUT_BT_START)){
          delay(10);
          contador++;
        }

          if(contador <TIMEOUT_BT_START){
            beep();
            switch(estadoAtual){
              case STOP:
                myDFPlayer.playFolder(modoOperacao, faixa);
                EEPROM.write(END_ULTIMA_MUSICA, faixa); 
                estadoAtual = PLAY;
              break;

              case PLAY:
                myDFPlayer.pause();
                estadoAtual = PAUSE;
              break;

              case PAUSE:
                myDFPlayer.start();
                estadoAtual = PLAY;
              break;
            }
          }
          break;

          case HISTORIAS:

          while((digitalRead(bt_START) == LOW) & (contador <TIMEOUT_BT_START)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_START){
            beep();
            switch(estadoAtual){
              case STOP:
                myDFPlayer.playFolder(modoOperacao, faixa); 
                estadoAtual = PLAY;
              break;

              case PLAY:
                myDFPlayer.pause();
                estadoAtual = PAUSE;
              break;

              case PAUSE:
                myDFPlayer.start();
                estadoAtual = PLAY;
              break;
            }

          }

          break;

          case JOGO:

          //if(digitalRead(BUSY_PIN)==0)break;

          while((digitalRead(bt_START)== LOW) & (contador <TIMEOUT_BT_START)){
            delay(10);
            contador++;
          }
          beep();
          byte x;
  sorteio:          
            x = random(1, MAX_SONS); // Le valor sorteado
            
            for(int i = 0;i<MAX_SONS;i++)
            {

                if((x==valores_sorteados[i]) & (indice_sorteio < MAX_SONS)){
                   goto sorteio;
                  }
            }
            

            faixa = valores_sorteados[indice_sorteio] = x;
            if( indice_sorteio< MAX_SONS) indice_sorteio++;
            myDFPlayer.playFolder(modoOperacao, faixa);

          
          break;

          case DORMIR:

          while((digitalRead(bt_START)== LOW) & (contador <TIMEOUT_BT_START)){
            delay(10);
            contador++;
          }

          if(contador <TIMEOUT_BT_START){
            beep();
            switch(estadoAtual){
              case STOP:
                myDFPlayer.playFolder(modoOperacao, faixa); 
                estadoAtual = PLAY;
              break;

              case PLAY:
                myDFPlayer.pause();
                estadoAtual = PAUSE;
              break;

              case PAUSE:
                myDFPlayer.start();
                estadoAtual = PLAY;
              break;
            }

          }
          break;
      }
     delay(300);
     while(digitalRead(bt_START)==LOW); 
  }
}

void atualizaDisplay()
{
  //Serial.print("Modo: ");
  //Serial.println(u8g.getMode());
  //Serial.print("Erro: ");
  //Serial.println(u8g.getWriteError());
  if(u8g.getWriteError())
  {
    u8g.begin();
  }
  
  u8g.firstPage();
    do 
    {
        draw();
    } while (u8g.nextPage());  

}

/* trata pino busy para avançar musicas */
void trataPinBusy(void)
{
  if((modoOperacao == MUSICAS) && (estadoAtual == PLAY))
  {
    if(digitalRead(BUSY_PIN)){
      if(faixa<MAX_MUSICAS+1) faixa++;
      if(faixa == MAX_MUSICAS +1 )faixa = 1;

      myDFPlayer.playFolder(modoOperacao,faixa);
      EEPROM.write(END_ULTIMA_MUSICA, faixa); 
      delay(500);
    }
  }
  else if ((modoOperacao == DORMIR) && (estadoAtual == PLAY) )
  {
    if(digitalRead(BUSY_PIN)){
      if(faixa<MAX_NINAR+1) {
        faixa++;
        if(faixa == MAX_NINAR +1 )
        {
          myDFPlayer.pause();
          faixa = 1;
          estadoAtual = STOP;
          delay(500);
        }
        else
        {
          myDFPlayer.playFolder(modoOperacao,faixa);
          delay(500);
        }
      }
    }
  }
  
}

void loop()
{
  ajustaVolume();
  trataBtModo();
  trataBtNext();
  trataBtPrev();
  trataBtStart();
  atualizaDisplay();
  trataPinBusy();
}

