/*
  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

  Rotary Encoder code from boolrules @ http://forum.arduino.cc/index.php?topic=242356.15

  Char array number addition: https://www.reddit.com/r/cpp_questions/comments/45ggz7/adding_digits_from_a_char_array/czxqhep/

*/

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#undef U8X8_HAVE_HW_SPI
#undef _SPI_H_INCLUDED

// Physical Pin Defines
const int menuPin=27;
const int selectPin=30;
const int pttPin=9;
const int encDTPin = 4;
const int encCLKPin = 5;

//Encoder
#define DIR_NONE 0x00           // No complete step yet.
#define DIR_CW   0x10           // Clockwise step.
#define DIR_CCW  0x20           // Anti-clockwise step.
/*
 * The below state table has, for each state (row), the new state
 * to set based on the next encoder output. From left to right in,
 * the table, the encoder outputs are 00, 01, 10, 11, and the value
 * in that position is the new state to set.
 */

// State definitions state table (emits a code at 00 only)
// states are: (NAB) N = 0: clockwise;  N = 1: counterclockwiswe
#define R_START     0x3
#define R_CW_BEGIN  0x1
#define R_CW_NEXT   0x0
#define R_CW_FINAL  0x2
#define R_CCW_BEGIN 0x6
#define R_CCW_NEXT  0x4
#define R_CCW_FINAL 0x5
//
/*
  U8glib Example Overview:
    Frame Buffer Examples: clearBuffer/sendBuffer. Fast, but may not work with all Arduino boards because of RAM consumption
    Page Buffer Examples: firstPage/nextPage. Less RAM usage, should work with all Arduino boards.
    U8x8 Text Only Example: No RAM usage, direct communication with display controller. No graphics, 8x8 Text only.
    
*/

// U8g2 Contructor List (Frame Buffer)
// The complete list is available here: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
// Please update the pin numbers according to your setup. Use U8X8_PIN_NONE if the reset pin is not connected

U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);


// Variables

uint8_t start_state = 1;
//char pwrlvl= "L";
//int pttState=0;
volatile int txOn=0;
int pttState=0;
int cursorPos=26;
int initPos=0;
int initPos1=0;

// Menu Variables
int repoff=0;
int repshiftm=0;
int enterMenu=0;
int bwm=0;
int fqs=0;
int fsm=0;
int hlfm=0;
int fss=0;
int pde=0;
int pdem=0;
int fltrm=0;

//TopBar Variables and their defaults
char pwrlvlbar[6]="PWR:L";
char shiftbar[8]="SHIFT:";
char repshift[3]="+";
char bwbar[5]="BW:";
char bw[2]="W";
char chan[6]="CH:1";
char rxfreq[10]="147.090";
char txfreq[10]="147.090";

int tracker=0;
char offset[10]="000.600";
char offset1[10]="000.000";
int offnum=0;

int selectState=0;
int lastSelectState=0;

int menuState=0;
int lastMenuState=0;

// CTCSS Codes
const char ctcss_array[39][3]={
  {0000,0},
  {'0001',67.0},
  {'0002',71.9},
  {'0003',74.4},
  {'0004',77.0},
  {'0005',79.7},
  {'0006',82.5},
  {'0007',85.4},
  {'0008',88.5},
  {'0009',91.5},
  {'0010',94.8},
  {'0011',97.4},
  {'0012',100.0},
  {'0013',103.5},
  {'0014',107.2},
  {'0015',110.9},
  {'0016',114.8},
  {'0017',118.8},
  {'0018',123.0},
  {'0019',127.3},
  {'0020',131.8},
  {'0021',136.4},
  {'0022',141.3},
  {'0023',146.2},
  {'0024',151.4},
  {'0025',156.7},
  {'0026',162.2},
  {'0027',167.9},
  {'0028',173.8},
  {'0029',179.9},
  {'0030',186.2},
  {'0031',192.8},
  {'0032',203.5},
  {'0033',210.7},
  {'0034',218.1},
  {'0035',225.7},
  {'0036',233.6},
  {'0037',241.8},
  {'0038',250.3}
};

//CDCSS Codes
const char cdcss_array[118][1]={
  '006','007','015','017','021','023','025','026','031','032','036','043','047','050','051','053','054','065','071','072','073','074','114','115','116','122',
  '125','131','132','134','141','143','145','152','155','156','162','165','172','174','205','212','214','223','225','226','243','244','245','246','251','252',
  '255','261','263','265','266','271','274','306','311','315','325','311','322','343','346','351','356','364','365','371','411','412','413','423','431','432',
  '445','446','452','454','455','462','464','465','466','503','506','516','523','526','532','546','565','606','612','624','627','631','632','654','662','664',
  '703','712','723','731','732','734','743','754'
};
char cdcss_style='N';


//Encoder Variables
unsigned int state;
int count = 0;         // count each indent
int old_count = 0;     // check for count changed
const unsigned char ttable[8][4] = {
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},                // R_CW_NEXT
    {R_CW_NEXT,  R_CW_BEGIN,  R_CW_BEGIN,  R_START},                // R_CW_BEGIN
    {R_CW_NEXT,  R_CW_FINAL,  R_CW_FINAL,  R_START | DIR_CW},       // R_CW_FINAL
    {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},                // R_START
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},                // R_CCW_NEXT
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_FINAL, R_START | DIR_CCW},      // R_CCW_FINAL
    {R_CCW_NEXT, R_CCW_BEGIN, R_CCW_BEGIN, R_START},                // R_CCW_BEGIN
    {R_START,    R_START,     R_START,     R_START}                 // ILLEGAL
};
//

void u8g2_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setFontMode(1);
  u8g2.setDrawColor(2);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}



void drawSplash(void) {
  /*if (start_state==0) {
    u8g2.drawRFrame(19,29,95,11,5);
    u8g2.drawStr(22,30,"xxxxxx 2M Radio");  
  }*/
    //digitalWrite(13,HIGH);
    u8g2.firstPage();
    do {
      u8g2.drawRFrame(19,29,95,11,5);
      u8g2.drawStr(22,30,"xxxxxx 2M Radio"); 
    } while (u8g2.nextPage());
    delay(2000);
    start_state=1; 
}

void topbar(void) {
    //u8g2.setDrawColor(2);
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawFrame(0,0,128,11);
    
    // Draw Power Level
    u8g2.drawStr(5,0,pwrlvlbar);
    //
    
    // Draw vertical line
    u8g2.drawBox(38,0,1,11);
    //

    // Draw Shift Indicator
    u8g2.drawStr(43,0,shiftbar);
    //

    // Draw vertical line
    u8g2.drawBox(87,0,1,11);
    //
    
    // Draw Bandwidth 
    u8g2.drawStr(91,0,bwbar);
    //
}

void applyshift(void) {
  memcpy(offset1,offset,sizeof(offset));
  if (repshift[0]==43){
    for (int i=6;i>=0;i--) {
      txfreq[i]=rxfreq[i]+offset1[i]-'0';
      if (txfreq[i]>'9') {
        txfreq[i]-=10;
        if (i!=4) {
          offset1[(i-1)]+=1;
        } else if (i==4) {
          offset1[(i-2)]+=1;
        }
      }
      if (i==4) {
        // Increment before we're supposed to for the period.. no need to add the period.
        i--;
      }    
    }
  } else if (repshift[0]==45){
    for (int i=6;i>=0;i--) {
      txfreq[i]=(rxfreq[i]-offset1[i])+'0';
      if (txfreq[i]<'0') {
        txfreq[i]+=10;
        if (i!=4) {
          offset1[(i-1)]+=1;
        } else if (i==4) {
          offset1[(i-2)]+=1;
        }
      }
      if (i==4) {
        // Increment before we're supposed to for the period.. no need to add the period.
        i--;
      }  
    }
  }
}

void AB_isr( ) {
    // Grab state of input pins.
    unsigned char pinstate = (digitalRead( encDTPin ) << 1) | digitalRead( encCLKPin );

    // Determine new state from the pins and state table.
    state = ttable[state & 0x07][pinstate];

    if( state & DIR_CW )    count++;
    if( state & DIR_CCW )   count--;
}

void ptt_ISR() {
  // Get pin
  if (digitalRead(pttPin)==1) {
    txOn=1;
  } else if (digitalRead(pttPin)==0) {
    txOn=0;
  }
  
}

void setup(void) {
  //Encoder     
  pinMode( encDTPin, INPUT_PULLUP );
  pinMode( encCLKPin, INPUT_PULLUP );
  attachInterrupt( digitalPinToInterrupt(encDTPin), AB_isr, CHANGE );
  attachInterrupt( digitalPinToInterrupt(encCLKPin), AB_isr, CHANGE );
  // Other Pins
  pinMode(selectPin, INPUT_PULLUP );
  pinMode(menuPin,INPUT_PULLUP);
  // PTT
  pinMode(pttPin,INPUT_PULLUP);
  attachInterrupt( digitalPinToInterrupt(pttPin), ptt_ISR, CHANGE);
  // Setup Display
  u8g2.begin();

  state = (digitalRead( encDTPin ) << 1) | digitalRead( encCLKPin );
  old_count = 0;
  shiftbar[6]=repshift[0];
  bwbar[3]=bw[0];
  applyshift();
}

void loop(void) {  
  // picture loop
  u8g2_prepare();  
  
  // Draw Splash Screen if we're just starting up
  if (start_state==0) {
    drawSplash();
    digitalWrite(13,LOW);
  }

  // Menu
  if (digitalRead(menuPin)==LOW&&txOn==0){
      initPos=count;
      cursorPos=26;
      enterMenu=1;   
  }

  if (enterMenu>0) {
          if (txOn==HIGH){
        // Draw box in about the middle of the screen, inverting text to visually indicate transmit
        u8g2.drawBox(0,13,128,35);
      }
    selectState=digitalRead(selectPin);
    lastSelectState=selectState;
    tracker=count;
    // Start Menu1    
    do {
      u8g2.firstPage();
      selectState=digitalRead(selectPin); 
      do {
        // Set font to 10pixel - u8g2_font_9x15_tf X11
        u8g2.setFont(u8g2_font_7x13_tf);
        if (tracker!=count) {
          if (tracker<count){
            cursorPos+=13;
            if (cursorPos>52) {
              cursorPos=26;
              enterMenu=2;
            }
            tracker=count;
          } else if (tracker>count){
            cursorPos-=13;
            if (cursorPos<26) {
              cursorPos=26;
            }
            tracker=count;
          }
        }
        if (txOn==HIGH){
        // Draw box in about the middle of the screen, inverting text to visually indicate transmit
          u8g2.drawBox(0,13,128,35);
        }
        u8g2.drawStr(43,13,"Menu 1");
        u8g2.drawGlyph(0,cursorPos,0x003e);
        u8g2.drawStr(7,26,"Repeater Offset");
        u8g2.drawStr(7,39,"Repeater Shift");
        u8g2.drawStr(7,52,"Bandwidth");
        if (selectState!=lastSelectState) {
          if (selectState==LOW) {
            initPos1=initPos;
            // Repeater Offset Menu
            if (cursorPos==26) {
              repoff=1;
              tracker=count;
              // Start cursor @ 32 (first digit)
              cursorPos=32;
              do {
                // Going into a submenu, Reset the select button
                lastSelectState=selectState;
                u8g2.firstPage();
                
                do {
                  selectState=digitalRead(selectPin); 
                  //Repeater Offset Adjustment display
                  u8g2.setFont(u8g2_font_7x13_tf);
                  u8g2.drawStr(11,13,"Repeater Offset");
                  u8g2.drawStr(50,52,"Exit");
                  u8g2.setFont(u8g2_font_9x15B_tf);
                  u8g2.drawStr(32,26,offset);

                  // Figure out what position we're on, and adjust which offset number we then operate on
                  switch (cursorPos) {
                    case 26:
                      // This is only for when we cycle through the last time?
                      break;
                    case 32:                
                      offnum=0;
                      break;
                    case 41:
                      offnum=1;
                      break;
                    case 50:
                      offnum=2;
                      break;
                    case 59:
                      offnum=3;
                      break;
                    case 68:
                      offnum=4;
                      break;
                    case 77:
                      offnum=5;
                      break;
                    case 86:
                      offnum=6;
                      break;
                    default:
                      cursorPos=5052;
                      break;
                  }
                    // Figure out if we moved the dial, then operate on it.
                    // 5052 = exit position
                    if (tracker!=count&&cursorPos!=5052) {
                      if (tracker>count){
                        offset[offnum]--;
                        if (offset[offnum]<48) {
                          offset[offnum]=57;
                        }
                        tracker=count;
                      } else if (tracker<count){
                        offset[offnum]++;
                        if (offset[offnum]>57) {
                          offset[offnum]=48;
                        }
                        tracker=count;
  
                      }
                    }
                    // Operate on the select button
                    if (selectState!=lastSelectState) {
                      if (selectState==LOW) {
                        if (cursorPos==5052) {
                          // Exit selected. Add offset to the RX and store in txfreq, Exit out of this menu, and clear the screen
                          applyshift();
                          repoff=0;
                          // Return cursorPos to position on Menu1
                          cursorPos=26;
                          u8g2.clearBuffer();
                          
                        } else {
                          // If we're not exiting, move to the next digit.
                          cursorPos=cursorPos+9;
                          if (cursorPos==59){
                            // If we're on the period, skip
                            cursorPos=cursorPos+9;
                          } 
                        }
                          
                      }
                    }
                    //}
                    lastSelectState=selectState;

                  // If the position cursor is on exit, Display >, otherwise underline the selected digit
                  if (cursorPos==5052) {
                    u8g2.setFont(u8g2_font_7x13_tf);
                    u8g2.drawGlyph(42,52,0x003e);
                  } else {
                    u8g2.drawGlyph(cursorPos,28,0x005f);
                  }
                } while ( u8g2.nextPage());
              // End Repeater Offset Do/While
              } while (repoff==1);
            // End Repeater Offset Menu If
            // Start of Repeater Offset Menu If
            } 
            else if (cursorPos==39) {
              repshiftm=1;
              tracker=count;
              // Start cursor @ 59 (center posistion)
              cursorPos=59;
              do {
                // Going into a submenu, Reset the select button
                lastSelectState=selectState;
                u8g2.firstPage();
                
                do {
                  selectState=digitalRead(selectPin); 
                  //Repeater shift Adjustment display
                  u8g2.setFont(u8g2_font_7x13_tf);
                  u8g2.drawStr(11,13,"Repeater Shift");
                  u8g2.drawStr(50,52,"Exit");
                  u8g2.setFont(u8g2_font_9x15B_tf);
                  u8g2.drawStr(59,26,repshift);

                    // Figure out if we moved the dial, then operate on it.
                    // 5052 = exit position
                    if (tracker!=count&&cursorPos!=5052) {
                      if (tracker>count){
                        repshift[0]-=2;
                        if (repshift[0]<43) {
                          repshift[0]=45;
                        }
                        tracker=count;
                      } else if (tracker<count){
                        repshift[0]+=2;
                        if (repshift[0]>45) {
                          repshift[0]=43;
                        }
                        tracker=count;
                      }
                    }
                    // Operate on the select button
                    if (selectState!=lastSelectState) {
                      if (selectState==LOW) {
                        if (cursorPos==5052) {
                          // Exit selected. Operate on offset to the RX and store in txfreq, Exit out of this menu, and clear the screen
                          // Copy the offset, so we can increase the next digit if we roll past 9, without effecting what it's actually set to
                          applyshift();
                          // Update the top bar info
                          shiftbar[6]=repshift[0];
                          repshiftm=0;
                          u8g2.clearBuffer();
                          //return cursor to position on Menu1
                          cursorPos=39;
                        } else {
                          // If button is pressed, Move to Exit (Only one toggling)
                          cursorPos=5052;
                        }
                      }
                    }
                    lastSelectState=selectState;

                  // If the position cursor is on exit, Display >, otherwise underline the selected digit
                  if (cursorPos==5052) {
                    u8g2.setFont(u8g2_font_7x13_tf);
                    u8g2.drawGlyph(42,52,0x003e);
                  } else {
                    u8g2.drawGlyph(cursorPos,28,0x005f);
                  }
                  


                } while ( u8g2.nextPage());





              } while (repshiftm==1);

            // End Repeater Offset Menu If
            // Start Bandwidth W/N Menu
            } 
            else if (cursorPos==52) {
              bwm=1;
              tracker=count;
              // Start cursor @ 46 (first option)
              cursorPos=26;
              do {
                // Going into a submenu, Reset the select button
                lastSelectState=selectState;
                u8g2.firstPage();
                
                do {
                  selectState=digitalRead(selectPin); 
                  //Bandiwdth display
                  u8g2.setFont(u8g2_font_7x13_tf);
                  u8g2.drawStr(32,13,"Bandwidth");
                  u8g2.drawStr(50,52,"Exit");
                  u8g2.drawStr(50,26,"Wide");
                  u8g2.drawStr(43,39,"Narrow");

                  // Figure out if we moved the dial, then operate on it.
                  // 5052 = exit position
                  if (tracker!=count&&cursorPos!=5052) {
                      if (bw[0]==87) {
                        bw[0]=78;
                      } else if (bw[0]==78) {
                        bw[0]=87;
                      }
                      tracker=count;
                  }

                  // Operate on the select button
                  if (selectState!=lastSelectState) {
                    if (selectState==LOW) {
                      if (cursorPos==5052) {
                        // Exit selected. Operate on offset to the RX and store in txfreq, Exit out of this menu, and clear the screen
                        // Copy the offset, so we can increase the next digit if we roll past 9, without effecting what it's actually set to
                        // Update the top bar info
                        bwbar[3]=bw[0];
                        bwm=0;
                        u8g2.clearBuffer();
                        // Return cursorPos to position on Menu1
                        cursorPos=52;
                      } else {
                        // If button is pressed, Move to Exit (Only one toggling)
                        cursorPos=5052;
                      }
                    }
                  }
                  lastSelectState=selectState;

                  // If the position cursor is on exit, Display >, otherwise highlight the selected BW
                  if (cursorPos==5052) {
                    u8g2.setFont(u8g2_font_7x13_tf);
                    u8g2.drawGlyph(42,52,0x003e);
                  } else {
                    if (bw[0]==87) {
                      u8g2.drawBox(39,25,48,13);
                    } else if (bw[0]==78) {
                      u8g2.drawBox(39,38,48,13);
                    }
                  }
                  


                } while ( u8g2.nextPage());





              } while (bwm==1);

            // End Bandwidth Menu If
            }
            
          // End Menu Button Selection
          }
          lastSelectState=selectState;
        }

        
      //} 
    
      // End Menu1 Do/While Display Loop
      } while (u8g2.nextPage());
    //End Menu1 Do/While Loop
    } while (enterMenu==1 );

    // Start Menu2
    do {
      u8g2.firstPage();
      selectState=digitalRead(selectPin); 
      do {
        // Set font to 10pixel - u8g2_font_9x15_tf X11
        u8g2.setFont(u8g2_font_7x13_tf);
        if (tracker!=count) {
          if (tracker<count){
            cursorPos+=13;
            if (cursorPos>52) {
              cursorPos=52;
              //enterMenu=2;
            }
            tracker=count;
          } else if (tracker>count){
            cursorPos-=13;
            if (cursorPos<26) {
              cursorPos=52;
              enterMenu=1;
            }
            tracker=count;
          }
        }
        u8g2.drawStr(43,13,"Menu 2");
        u8g2.drawGlyph(0,cursorPos,0x003e);
        u8g2.drawStr(7,26,"Frequency Steps");
        u8g2.drawStr(7,39,"Filter");
        u8g2.drawStr(7,52,"Exit");
        if (selectState!=lastSelectState) {
          if (selectState==LOW) {
            // Exit Menu
            if (cursorPos==52) {
              enterMenu=0;
            } 
            // Frequency Steps Menu
            else if (cursorPos==26) {
              fsm=1;
              tracker=count;
              // Start cursor @ 46 (first digit)
              cursorPos=46;
              do {
                // Going into a submenu, Reset the select button
                lastSelectState=selectState;
                u8g2.firstPage();
                
                do {
                  selectState=digitalRead(selectPin); 
                  //Bandiwdth display
                  u8g2.setFont(u8g2_font_7x13_tf);
                  u8g2.drawStr(12,13,"Frequency Steps");
                  u8g2.drawStr(50,52,"Exit");
                  u8g2.drawStr(36,26,"12.5 Khz");
                  u8g2.drawStr(36,39,"25.0 Khz");

                  // Figure out if we moved the dial, then operate on it.
                  // 5052 = exit position
                  if (tracker!=count&&cursorPos!=5052) {
                      if (fqs==1) {
                        fqs=0;
                      } else if (fqs==0) {
                        fqs=1;
                      }
                      tracker=count;
                  }

                  // Operate on the select button
                  if (selectState!=lastSelectState) {
                    if (selectState==LOW) {
                      if (cursorPos==5052) {
                        // Exit selected. Operate on offset to the RX and store in txfreq, Exit out of this menu, and clear the screen
                        // Copy the offset, so we can increase the next digit if we roll past 9, without effecting what it's actually set to
                        // Update the top bar info
                        fsm=0;
                        u8g2.clearBuffer();
                        // Return cursorPos to position on Menu1
                        cursorPos=26;
                      } else {
                        // If button is pressed, Move to Exit (Only one toggling)
                        cursorPos=5052;
                      }
                    }
                  }
                  lastSelectState=selectState;

                  // If the position cursor is on exit, Display >, otherwise highlight the selected BW
                  if (cursorPos==5052) {
                    u8g2.setFont(u8g2_font_7x13_tf);
                    u8g2.drawGlyph(42,52,0x003e);
                  } else {
                    if (fqs==0) {
                      u8g2.drawBox(31,25,65,13);
                    } else if (fqs==1) {
                      u8g2.drawBox(31,38,65,13);
                    }
                  }
                  
                } while ( u8g2.nextPage());

              } while (fsm==1);

            // End Frequency Steps If
            
            } 
            // Begin filter menu if
            else if (cursorPos==39) {
////////////////////////////////////////////////////////////////////
              // Start Filter Menu
              fltrm=1;
              tracker=count;
              cursorPos=13;
              do {
                u8g2.firstPage();
                selectState=digitalRead(selectPin); 
                do {
                  // Set font to 10pixel - u8g2_font_9x15_tf X11
                  u8g2.setFont(u8g2_font_7x13_tf);
                  if (tracker!=count) {
                    if (tracker<count){ //right
                      cursorPos+=13;
                      if (cursorPos>52) {
                        cursorPos=13;
                        //enterMenu=2;
                      }
                      tracker=count;
                    } else if (tracker>count){ //left
                      cursorPos-=13;
                      if (cursorPos<13) {
                        cursorPos=52;
                        //enterMenu=1;
                      }
                      tracker=count;
                    }
                  }
                  u8g2.drawStr(25,0,"Filter Menu");
                  u8g2.drawGlyph(0,cursorPos,0x003e);
                  u8g2.drawStr(32,13,"Emphasis");
                  u8g2.drawStr(32,26,"High Pass");
                  u8g2.drawStr(32,39,"Low Pass");
                  u8g2.drawStr(50,52,"Exit");
                  if (selectState!=lastSelectState) {
                    if (selectState==LOW) {
                      // Exit Menu
                      if (cursorPos==52) {
                        fltrm=0;
                        cursorPos=39;
                      } 
                      else if (cursorPos==13) {
                        // Pre/De-Emphasis Enable/Disable
                        pdem=1;
                        tracker=count;
                        // Start cursor @ 46 (first digit)
                        cursorPos=46;
                        do {
                          // Going into a submenu, Reset the select button
                          lastSelectState=selectState;
                          u8g2.firstPage();
                          ///////////
                          do {
                            selectState=digitalRead(selectPin); 
                            //Bandiwdth display
                            u8g2.setFont(u8g2_font_7x13_tf);
                            u8g2.drawStr(11,0,"Pre/De-Emphasis");
                            u8g2.drawStr(50,52,"Exit");
                            u8g2.drawStr(50,13,"Enable");
                            u8g2.drawStr(43,26,"Disable");
          
                            // Figure out if we moved the dial, then operate on it.
                            // 5052 = exit position
                            if (tracker!=count&&cursorPos!=5052) {
                                if (pde==1) {
                                  pde=0;
                                } else if (pde==9) {
                                  pde=1;
                                }
                                tracker=count;
                            }

                            // Operate on the select button
                            if (selectState!=lastSelectState) {
                              if (selectState==LOW) {
                                if (cursorPos==5052) {
                                  // Exit selected. Operate on offset to the RX and store in txfreq, Exit out of this menu, and clear the screen
                                  // Copy the offset, so we can increase the next digit if we roll past 9, without effecting what it's actually set to
                                  // Update the top bar info
                                  pdem=0;
                                  u8g2.clearBuffer();
                                  // Return cursorPos to position on Filter Menu
                                  cursorPos=13;
                                } else {
                                  // If button is pressed, Move to Exit (Only one toggling)
                                  cursorPos=5052;
                                }
                              }
                            }
                            lastSelectState=selectState;
          
                            // If the position cursor is on exit, Display >, otherwise highlight the selected BW
                            if (cursorPos==5052) {
                              u8g2.setFont(u8g2_font_7x13_tf);
                              u8g2.drawGlyph(42,52,0x003e);
                            } else {
                              if (pde==1) {
                                u8g2.drawBox(48,27,50,13);
                              } else if (pde==0) {
                                u8g2.drawBox(48,40,50,13);
                              }
                            }
                          } while ( u8g2.nextPage());
                          ////////
          
                        } while (pdem==1);
          
                      // End Emphasis Enable/Disable
                      
                      } 
                      // Begin filter menu if
                      else if (cursorPos==26) {
                        // Filter Menu
                        hlfm=1;
                        tracker=count;
                        // Start cursor @ 46 (first digit)
                        cursorPos=39;
                        do {
                          // Going into a submenu, Reset the select button
                          lastSelectState=selectState;
                          u8g2.firstPage();
                          
                          do {
                            selectState=digitalRead(selectPin); 
                            //Filter display
                            u8g2.setFont(u8g2_font_7x13_tf);
                            u8g2.drawStr(39,0,"Filters");
                            u8g2.drawStr(50,52,"Exit");
                            u8g2.drawStr(36,13,"Emphasis");
                            u8g2.drawStr(36,26,"High Pass");
                            u8g2.drawStr(36,39,"Low Pass");
          
                            // Figure out if we moved the dial, then operate on it.
                            // 5052 = exit position
                            if (tracker!=count&&cursorPos!=5052) {
                                if (fss==2) {
                                  if (tracker>count){
                                    fss=1;
                                    
                                  }
                                  
                                } else if (fss==1) {
                                  fss=0;
                                  } else if (fss==0) {
                                    fss=2;
                                }
                                  tracker=count;
                            }
          
                            // Operate on the select button
                            if (selectState!=lastSelectState) {
                              if (selectState==LOW) {
                                if (cursorPos==5052) {
                                  // Exit selected. Operate on offset to the RX and store in txfreq, Exit out of this menu, and clear the screen
                                  // Copy the offset, so we can increase the next digit if we roll past 9, without effecting what it's actually set to
                                  // Update the top bar info
                                  hlfm=0;
                                  u8g2.clearBuffer();
                                  // Return cursorPos to position on Menu1
                                  cursorPos=26;
                                } else {
                                  // If button is pressed, Move to Exit (Only one toggling)
                                  cursorPos=5052;
                                }
                              }
                            }
                            lastSelectState=selectState;
          
                            // If the position cursor is on exit, Display >, otherwise highlight the selected BW
                            if (cursorPos==5052) {
                              u8g2.setFont(u8g2_font_7x13_tf);
                              u8g2.drawGlyph(42,52,0x003e);
                            } else {
                              if (fss==0) {
                                u8g2.drawGlyph(0,cursorPos,0x003e);
                                //u8g2.drawBox(31,13,71,13);
                              } else if (fss==1) {
                                u8g2.drawGlyph(0,cursorPos,0x003e);
                                //u8g2.drawBox(31,26,71,13);
                              } else if (fss==2) {
                                u8g2.drawGlyph(0,cursorPos,0x003e);
                                //u8g2.drawBox(31,39,71,13);
                              }
                            }
                            
                          } while ( u8g2.nextPage());
          
                        } while (hlfm==1);
                      } // End Filter Menu
                    }
                  }
                  lastSelectState=selectState;
                // Power Saving (Physical pin)
                // RF Power Selection (Physical Pin)
                // Handshake - startup?
                // Freuqncy Scanning??
                // Group Setting?
                // Volume
                // LP/HP Filter
                // Tail Tone? (Bad Ham)
                // RSSI?
                } while (u8g2.nextPage());
              // End Menu2 Do/While Loop  
              } while (fltrm==1);
    /////////////////////////////////////////////////////////////////
            } // End Filter Menu
          }
        }
        lastSelectState=selectState;
      // Power Saving (Physical pin)
      // RF Power Selection (Physical Pin)
      // Handshake - startup?
      // Freuqncy Scanning??
      // Group Setting?
      // Volume
      // LP/HP Filter
      // Tail Tone? (Bad Ham)
      // RSSI?
      } while (u8g2.nextPage());
    // End Menu2 Do/While Loop  
    } while (enterMenu==2);
    /* Menu debug code
      String str2=String(cursorPos);
      char str3[10];
      str2.toCharArray(str3,10);
      u8g2.drawStr(0,64,str3);
      String str4=String(count);
      char str5[10];
      str4.toCharArray(str5,10);
      u8g2.drawStr(20,64,str5);
      String str6=String(tracker);
      char str7[10];
      str6.toCharArray(str7,10);
      u8g2.drawStr(40,64,str7);
      */
  } else {

    // Draw main display
    u8g2.firstPage();
    do {
      // See if we're transmitting
      if (txOn==HIGH){
        // Draw box in about the middle of the screen, inverting text to visually indicate transmit
        u8g2.drawBox(0,13,128,35);
      }
      // Draw the top bar
      topbar();
      // Set font to 10pixel - u8g2_font_9x15_tf X11
      u8g2.setFont(u8g2_font_9x15_tf);
      // Indicate mode (Channel or VFO)
      u8g2.drawStr(0,13,chan);
  
      // Set font to 20pixel - u8g2_font_fub20_tf Free Universal
      u8g2.setFont(u8g2_font_fub20_tf);
      // Display the Frequency we're currently receiving, else display TX Freq
      if (txOn==0) {
        u8g2.drawStr(0,26,rxfreq);
      } else if (txOn==1) {
        u8g2.drawStr(0,26,txfreq);
      }
      // Set font back to 10pixel - u8g2_font_9x15_tf X11
      u8g2.setFont(u8g2_font_9x15_tf);
      u8g2.drawStr(102,35,"Mhz");
      // Display Encoder
      String str1=String(count);
      char countd[10];
      str1.toCharArray(countd,10);
      //Count.toCharArray(countd,5);
      u8g2.drawStr(0,51,countd);
  
    } while (u8g2.nextPage() );
    
  }  

  // Get PTT Button State
 /* if (digitalRead(pttPin)==HIGH){
    txOn=1;
  } else {
    txOn=0;
  }*/

 u8g2.clearBuffer();
 

  // deley between each page
  delay(100);

}
