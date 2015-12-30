/*********************************************************************************
12/2~
  スペクトルアナライザ
使用マイコン:Atmega328
クロック:8MHz

制御対象:マトリクスLED(アノードコモン)
シリアルポート:spi通信 シフトレジスタ制御(ソースドライバ制御)
ioポート(D):シンクドライバ制御
LED抵抗 : 200Ω
********************************************************************************/
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SS  0b00000100 //シフトレジスタ起動
#define RESET 0b00000010 //MSGEQ7 RESET
#define L_STROBE 0b00000001 //MSGEQ7 STROBE Lch
#define R_STROBE 0b10000000 //Rch
#define STROBE_SIG 18//STROBE_SIGNAL 待機時間
#define RESET_SIG  1//RESET_SIGNAL 待機時間
#define is_SET(x,y) ((x) & (1<<(y)))//AD変換用定義

const uint8_t log_level[8]={30,35,43,55,73,101,150,230};//レベル変換関数logスケール変換用
const uint8_t x_led[8]={1,2,4,8,16,32,64,128};//位置指定(横)

const uint8_t y_led[8]={127,63,31,15,7,3,1,0};//レベル表示(縦)

uint8_t x_posi=0;//X_position 
volatile uint8_t y_posi[16]={"0"};//Y_posision
volatile uint8_t buf_y_posi[16]={"0"};//前のy_posiを保管しておく
volatile uint8_t interrupt_cnt =0;//割り込み関数,割り込みカウント
 
uint8_t level_convert(uint8_t in_level); //予め定義

/*初期設定*/

void spi_ini(){//spi通信設定
  //CSはPD2ピン
  SPCR|=_BV(SPE)|_BV(MSTR);
  /*  SPIE    : SPI割り込み許可
	 SPE     : SPI許可(SPI操作を許可するために必須)
	 DORD    : データ順選択,1:LSBから 0:MSBから
	 MSTR    : 1:主装置動作 0:従装置動作
	 CPOL    : SCK極性選択
	 CPHA    :SCK位相選択
	 SPR1,SPR0 : 00:SCK周波数=fosc/4
  */
  /*SPI状態レジスタ SPSR
    SPIF    : SPI割り込み要求フラグ 転送完了時1
    WCOL    :上書き発生フラグ
  */
  /*SPIデータレジスタSPDR
    8bit
    7 6 5 4 3 2 1 0
    (MSB)       (LSB)
  */

  //SPI通信用関数
}
void spi_send(uint8_t spi_data){//SPI送信用関数
  uint8_t dummy = 0;
  dummy = SPDR;
  SPDR = spi_data;
  while(!(SPSR&(1<<SPIF)));//転送完了まで待機
  dummy = SPDR;
}

uint8_t spi_get(void){//SPI受信用関数
  uint8_t dummy = 0;
  SPDR = dummy;
  while(!(SPSR&(1<<SPIF)));//転送完了まで待機
  return SPDR;
    
}
//ここまでSPI通信用関数

void pin_ini(void){
  DDRD=0b11111111;//PD0~7:LEDsource
  DDRB=0b10101111;//PB5:SCK,PB3:MOSI,PB2:SS(sift register) PB1:reset,PB0:strobe(MS L),PB7:strobe(MS R)
  PORTD=0b00000000;
  PORTB=0b00000000;
}

void timer_ini(void){
  
  TCCR0A|=_BV(WGM01);//比較一致タイマCTC TOP値:OCR0A
  TCCR0B|=_BV(CS02);//前置分周 256分周 = N

  /*割り込み関係*/
  TIMSK0|=_BV(OCIE0A);//比較Aタイマ割り込み 許可
  OCR0A=255;//77Hz程度の頻度で割り込み:199
  /*********************************************************************************
   *f_interrput = (f_io)/(2*N*(OCR0A+1)) : 割り込み時間計算式
   ********************************************************************************/
}


uint8_t level_convert(uint8_t in_level){
  uint8_t out_level=0;
  // buf = in_level/level_div;
  if(in_level>=0 && in_level<=log_level[0]){
    out_level = 0;
  }else if(in_level>log_level[0] && in_level<=log_level[1]){
    out_level = 1;
  }else if(in_level>log_level[1] && in_level<=log_level[2]){
    out_level = 2;
  }else if(in_level>log_level[2] && in_level<=log_level[3]){
    out_level = 3;
  }else if(in_level>log_level[3] && in_level<=log_level[4]){
    out_level = 4;
  }else if(in_level>log_level[4] && in_level<=log_level[5]){
    out_level = 5;
  }else if(in_level>log_level[5] && in_level<=log_level[6]){
    out_level = 6;
  }else if(in_level>log_level[6] && in_level<=log_level[7]){
    out_level = 7;
  }else if(in_level>log_level[7]){
    out_level = 7;
  }
  return out_level;
}


ISR(TIMER0_COMPA_vect){
  if(interrupt_cnt>15){
    interrupt_cnt =0;
  }
  uint8_t adc_data =0 ;//AD変換値受け取り変数
  /*MSGEQ7の処理*/
    if(interrupt_cnt<=7){//Lの処理
    ADMUX|=_BV(ADLAR);
    ADMUX&=~_BV(MUX0);//PC0に切り替え*/
    PORTB|=L_STROBE;
    _delay_us(STROBE_SIG);//STROBE信号用
    PORTB&=~L_STROBE;
    _delay_us(72);//Strobe to Storbe ICとしては72usほどでいい
   
    }else if(interrupt_cnt>7&&interrupt_cnt<=15){
    ADMUX|=_BV(ADLAR)|_BV(MUX0);//PC1に切り替え
    PORTB|=R_STROBE;
    _delay_us(STROBE_SIG);//STROBE信号用
    PORTB&=~R_STROBE;
    _delay_us(72);//Strobe to Storbe ICとしては72usほどでいい
    }
  /*MSGEQ7から送られてきたデータの処理*/
  ADCSRA|=_BV(ADSC);//AD変換開始
  while(is_SET(ADCSRA,ADIF)==0);//変換終了まで待機
  adc_data=ADCH;
  y_posi[interrupt_cnt]=level_convert(adc_data);
  
  /*なめらか処理*/
  if(buf_y_posi[interrupt_cnt]>y_posi[interrupt_cnt]){//入力データが前のデータより小さい時
    buf_y_posi[interrupt_cnt]=buf_y_posi[interrupt_cnt]-1;//レベルを一つさげる
    y_posi[interrupt_cnt]=buf_y_posi[interrupt_cnt];
  }else if(buf_y_posi[interrupt_cnt]<=y_posi[interrupt_cnt]){//入力データが前のデータより大きい時
    buf_y_posi[interrupt_cnt]=y_posi[interrupt_cnt];//バッファの値を更新する
  }
  
  interrupt_cnt++;
  
  if(interrupt_cnt==7||interrupt_cnt==15){
      y_posi[interrupt_cnt]=0;
	 interrupt_cnt++;
      }
}

void adc_ini(){//AD変換初期設定
  ADMUX |=_BV(ADLAR);
  /*REFS1 REFS0 : 01 AVCCピンの基準電圧
   *MUX3 MUX2 MUX1 MUX0: 0000ADC0ピン
   */
    
  ADCSRA|= _BV(ADEN);
  /*ADEN :1 A/D許可
   *ADPS2 ADPS1 ADPS0 :000 CK/2 ~= 4MHz(変換クロック)
   *_BV(ADSC)によって起動 単独変換動
   */
    
  /*ADCSRA|=_BV(ADEN)|_BV(ADATE);
   *ADCSRB|=_BV(ADTS2)|_BV(ADTS1)|_BV(ADTS0);
   *自動変換タイマ/カウンタ1捕獲*/
  DIDR0 |=_BV(ADC0D)|_BV(ADC1D);
  /*デジタル入力禁止 ADC0: PC0,PC1*/
}

int main(void){
  /*関数宣言*/
  adc_ini();
  pin_ini();
  spi_ini();
  timer_ini();

  uint8_t position=0;
  uint8_t main_cnt=0;

  /*MSGEQ7 SET*/
  PORTB|=RESET;
  _delay_us(1);
  PORTB&=~RESET;

  /*シフトレジスタ SET*/
  PORTB|=SS;
  

  ADCSRA|=_BV(ADSC);//初期
  sei();//割り込み許可


  for(;;){
    if(main_cnt>15){
	 main_cnt=0;
    }
    position = y_posi[main_cnt];
        _delay_us(200);//残像防止
    
    PORTB&=~SS;//シフトレジスタ選択,起動
    //ソースドライバ側操作
    if(main_cnt<=7){
	
	 spi_send(~y_led[position]);//データ転送(8bit)
	  spi_send(0x00);
    }else if(main_cnt>7&&main_cnt<=15){
	  spi_send(0x00);
	 spi_send(~y_led[position]);//~は上下反転用

    }
    PORTB|=SS;//シフトレジスタ選択解除,反映
    
    /*
        PORTB&=~SS;//シフトレジスタ選択,起動
    //ソースドライバ側操作
    if(main_cnt<=7){
	 spi_send(x_led[main_cnt]);//データ転送(8bit)
	 spi_send(0x00);
    }else if(main_cnt>7&&main_cnt<=15){
	 spi_send(0x00);
	 spi_send(x_led[main_cnt-8]);
    }
    PORTB|=SS;//シフトレジスタ選択解除,反映
    */

    /*発光動作*/
    //    PORTD=y_led[y_posi[main_cnt]];
    //シンクドライバ側操作
       
    if(main_cnt<=7){
	 //	 PORTD=0;//(1<<3);
	 	 PORTD=x_led[main_cnt];
    }else if(main_cnt>7&&main_cnt<=15){
	 // PORTD=0;//(1<<5);
	  PORTD=x_led[main_cnt-8];
	 }
    main_cnt++;
    }
  
  return 0;
}


