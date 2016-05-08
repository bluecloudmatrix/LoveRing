/********************************************************************
* LCD1602
* ADXL345
* STC89C52RC
* RING
* Qiushan
***********************************************************************/
#include<reg52.h>
#include<intrins.h>
#define uchar unsigned char
#define uint  unsigned int

sbit		P10=P1^0;

sbit	  SCL=P1^4;      //IIC时钟引脚定义
sbit 	  SDA=P1^5;      //IIC数据引脚定义

sbit E=P2^2;		//1602使能
sbit RW=P2^1;		//1602读写
sbit RS=P2^0;		//1602输入指令或数据

#define	SlaveAddress   0xA6	  //定义器件在IIC总线中的从地址,根据ALT  ADDRESS地址引脚不同修改
                              //ALT  ADDRESS引脚接地时地址为0xA6，接电源时地址为0x3A
															//所选ADXL345模块ALT ADDRESS引脚默认接地
typedef unsigned char  BYTE;
typedef unsigned short WORD;

uchar ge,shi,bai,qian,wan;           //显示变量

BYTE BUF[8];   											 // 从ADXL345中读取数据缓冲区
int  dis_data;                       //变量

float preX = 0.00;
float preY = 0.00;
float cmpX = 0.00;
float cmpY = 0.00;
float threshold = -0.30;

uint step = 0;

int recordX[3]={0,0,0};
float recordXData[3];
int traceX=0;
int recordY[3]={0,0,0};
float recordYData[3];
int traceY=0;

void Delay5us();
void ADXL345_Start();
void ADXL345_Stop();
void ADXL345_SendACK(bit ack);
bit  ADXL345_RecvACK();
void ADXL345_SendByte(BYTE dat);
BYTE ADXL345_RecvByte();
void Init_ADXL345(void);             //初始化ADXL345
void  Single_Write_ADXL345(uchar REG_Address,uchar REG_data);   //单个写入数据

void  Multiple_Read_ADXL345();                                  //连续的读取内部寄存器数据

void Data_Process();

void show_steps();

/********************************************************************
*  belongs to ADXL345
*  
*  
*  
***********************************************************************/
void conversion(uint temp_data)  
{  
    wan=temp_data/10000+0x30 ;
    temp_data=temp_data%10000;   //取余运算
		qian=temp_data/1000+0x30 ;
    temp_data=temp_data%1000;    //取余运算
    bai=temp_data/100+0x30   ;
    temp_data=temp_data%100;     //取余运算
    shi=temp_data/10+0x30    ;
    temp_data=temp_data%10;      //取余运算
    ge=temp_data+0x30; 	
}

/********************************************************************
*  belongs to LCD1602
*  
*  
*  
***********************************************************************/
void delay()
{
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	_nop_();
}

/**************************************
* belongs to ADXL345
* 延时5毫秒(STC90C52RC@12M)
* 不同的工作环境,需要调整此函数
* 当改用1T的MCU时,请调整此延时函数
**************************************/
void Delay5ms()
{
    WORD n = 560;

    while (n--);
}

/********************************************************************
*  belongs to LCD1602
*  
*  
*  
***********************************************************************/
void Delay(uint i)
{
	uint x,j;
	for(j=0;j<i;j++)
	for(x=0;x<=148;x++);	
}

/********************************************************************
*  belongs to LCD1602
*  
*  
*  
***********************************************************************/
bit Busy(void)
{
	bit busy_flag = 0;
	RS = 0;
	RW = 1;
	E = 1;
	delay();
	busy_flag = (bit)(P0 & 0x80);
	E = 0;
	return busy_flag;
}

/********************************************************************
*  belongs to LCD1602
*  
*  
*  
***********************************************************************/
void wcmd(uchar del)
{
	while(Busy());
	RS = 0;
	RW = 0;
	E = 0;
	delay();
	P0 = del;
	delay();
	E = 1;
	delay();
	E = 0;
}

/********************************************************************
*  belongs to LCD1602
*  
*  
*  
***********************************************************************/
void wdata(uchar del)
{
	while(Busy());
	RS = 1;
	RW = 0;
	E = 0;
	delay();
	P0 = del;
    delay();
	E = 1;
	delay();
	E = 0;
}

/********************************************************************
*  belongs to LCD1602
*  
*  
*  
***********************************************************************/
void L1602_init(void)
{
	wcmd(0x38);
	Delay(5);
	wcmd(0x38);
	Delay(5);
	wcmd(0x38);
	Delay(5);
	wcmd(0x38);
	wcmd(0x08);	
	wcmd(0x0c);
	wcmd(0x04);
	wcmd(0x01);
}

/********************************************************************
*  belongs to LCD1602
*  
*  
*  
***********************************************************************/
void L1602_char(uchar hang,uchar lie,char sign)
{
	uchar a;
	if(hang == 1) a = 0x80;
	if(hang == 2) a = 0xc0;
	a = a + lie - 1;
	wcmd(a);
	wdata(sign);
}

/********************************************************************
*  belongs to LCD1602
*  
*  
*  
***********************************************************************/
void L1602_string(uchar hang,uchar lie,uchar *p)
{
	uchar a,b=0;
	if(hang == 1) a = 0x80;
	if(hang == 2) a = 0xc0;
	a = a + lie - 1;
	while(1)
	{
		wcmd(a++);			
		if((*p == '\0')||(b==16)) break;
		b++;
		wdata(*p);
		p++;
	}
}

/**************************************
* ADXL345
延时5微秒(STC90C52RC@12M)
不同的工作环境,需要调整此函数，注意时钟过快时需要修改
当改用1T的MCU时,请调整此延时函数
**************************************/
void Delay5us()
{
    _nop_();_nop_();_nop_();_nop_();
    _nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
}


/**************************************
* ADXL345
起始信号
**************************************/
void ADXL345_Start()
{
    SDA = 1;                    //拉高数据线
    SCL = 1;                    //拉高时钟线
    Delay5us();                 //延时
    SDA = 0;                    //产生下降沿
    Delay5us();                 //延时
    SCL = 0;                    //拉低时钟线
}

/**************************************
* ADXL345
停止信号
**************************************/
void ADXL345_Stop()
{
    SDA = 0;                    //拉低数据线
    SCL = 1;                    //拉高时钟线
    Delay5us();                 //延时
    SDA = 1;                    //产生上升沿
    Delay5us();                 //延时
}

/**************************************
* ADXL345
发送应答信号
入口参数:ack (0:ACK 1:NAK)
**************************************/
void ADXL345_SendACK(bit ack)
{
    SDA = ack;                  //写应答信号
    SCL = 1;                    //拉高时钟线
    Delay5us();                 //延时
    SCL = 0;                    //拉低时钟线
    Delay5us();                 //延时
}

/**************************************
* ADXL345
接收应答信号
**************************************/
bit ADXL345_RecvACK()
{
    SCL = 1;                    //拉高时钟线
    Delay5us();                 //延时
    CY = SDA;                   //读应答信号
    SCL = 0;                    //拉低时钟线
    Delay5us();                 //延时

    return CY;
}

/**************************************
* ADXL345
向IIC总线发送一个字节数据
**************************************/
void ADXL345_SendByte(BYTE dat)
{
    BYTE i;

    for (i=0; i<8; i++)         //8位计数器
    {
        dat <<= 1;              //移出数据的最高位
        SDA = CY;               //送数据口
        SCL = 1;                //拉高时钟线
        Delay5us();             //延时
        SCL = 0;                //拉低时钟线
        Delay5us();             //延时
    }
    ADXL345_RecvACK();
}

/**************************************
* ADXL345
从IIC总线接收一个字节数据
**************************************/
BYTE ADXL345_RecvByte()
{
    BYTE i;
    BYTE dat = 0;

    SDA = 1;                    //使能内部上拉,准备读取数据,
    for (i=0; i<8; i++)         //8位计数器
    {
        dat <<= 1;
        SCL = 1;                //拉高时钟线
        Delay5us();             //延时
        dat |= SDA;             //读数据               
        SCL = 0;                //拉低时钟线
        Delay5us();             //延时
    }
    return dat;
}

//******单字节写入*******************************************
void Single_Write_ADXL345(uchar REG_Address,uchar REG_data)
{
    ADXL345_Start();                  //起始信号
    ADXL345_SendByte(SlaveAddress);   //发送设备地址+写信号
    ADXL345_SendByte(REG_Address);    //内部寄存器地址，请参考中文pdf22页 
    ADXL345_SendByte(REG_data);       //内部寄存器数据，请参考中文pdf22页 
    ADXL345_Stop();                   //发送停止信号
}

//********单字节读取*****************************************
uchar Single_Read_ADXL345(uchar REG_Address)
{  uchar REG_data;
    ADXL345_Start();                          //起始信号
    ADXL345_SendByte(SlaveAddress);           //发送设备地址+写信号
    ADXL345_SendByte(REG_Address);            //发送存储单元地址，从0开始	
    ADXL345_Start();                          //起始信号
    ADXL345_SendByte(SlaveAddress+1);         //发送设备地址+读信号
    REG_data=ADXL345_RecvByte();              //读出寄存器数据
	ADXL345_SendACK(1);   
	ADXL345_Stop();                           //停止信号
    return REG_data; 
}

//*********************************************************
//
//连续读出ADXL345内部加速度数据，地址范围0x32~0x37
//
//*********************************************************
void Multiple_read_ADXL345(void)
{   uchar i;
    ADXL345_Start();                          //起始信号
    ADXL345_SendByte(SlaveAddress);           //发送设备地址+写信号
    ADXL345_SendByte(0x32);                   //发送存储单元地址，从0x32开始	
    ADXL345_Start();                          //起始信号
    ADXL345_SendByte(SlaveAddress+1);         //发送设备地址+读信号
	 for (i=0; i<6; i++)                      //连续读取6个地址数据，存储中BUF
    {
        BUF[i] = ADXL345_RecvByte();          //BUF[0]存储0x32地址中的数据
        if (i == 5)
        {
           ADXL345_SendACK(1);                //最后一个数据需要回NOACK
        }
        else
        {
          ADXL345_SendACK(0);                //回应ACK
       }
   }
    ADXL345_Stop();                          //停止信号
    Delay5ms();
}


//*****************************************************************

//初始化ADXL345，根据需要请参考pdf进行修改************************
void Init_ADXL345()
{
   Single_Write_ADXL345(0x31,0x0B);   //测量范围,正负16g，13位模式
   Single_Write_ADXL345(0x2C,0x08);   //速率设定为12.5 参考pdf13页
   Single_Write_ADXL345(0x2D,0x08);   //选择电源模式   参考pdf24页
   Single_Write_ADXL345(0x2E,0x80);   //使能 DATA_READY 中断
   Single_Write_ADXL345(0x1E,0x00);   //X 偏移量 根据测试传感器的状态写入pdf29页
   Single_Write_ADXL345(0x1F,0x00);   //Y 偏移量 根据测试传感器的状态写入pdf29页
   Single_Write_ADXL345(0x20,0x05);   //Z 偏移量 根据测试传感器的状态写入pdf29页
}
//***********************************************************************

//***********************************************************************
//显示x轴
void display_x()
{   
	int fuhao=0; // 0为正
	float temp, tt;
  dis_data=(BUF[1]<<8)+BUF[0];  //合成数据   
	if(dis_data<0){
		fuhao=1;
		dis_data=-dis_data;
    //L1602_char(1,3,'-');      //显示正负符号位
	}
	//else L1602_char(1,3,' '); //显示空格

  temp=(float)dis_data*3.9;  //计算数据和显示,查考ADXL345快速入门第4页
  conversion(temp);          //转换出显示需要的数据
/*
	L1602_char(1,1,'X');
	L1602_char(1,2,':'); 
  L1602_char(1,4,qian); 
	L1602_char(1,5,'.'); 
  L1602_char(1,6,bai); 
  L1602_char(1,7,shi); 
	L1602_char(1,8,' '); 
*/

	recordX[traceX]=fuhao;
	if(fuhao==1)
		recordXData[traceX]=-(qian+0.1*bai+0.01*shi);
	else
		recordXData[traceX]=qian+0.1*bai+0.01*shi;
	traceX++;
	if(traceX>2)
		traceX=0;
	
	/*
	if(fuhao==1)
	{
	  tt=-(qian+0.1*bai+0.01*shi)-preX;
		preX=-(qian+0.1*bai+0.01*shi);
	} else
	{
		tt=qian+0.1*bai+0.01*shi-preX;
		preX=qian+0.1*bai+0.01*shi;
	}
	
	if(tt<0)
		cmpX = -tt;
	else
		cmpX = tt;
	*/
	
	
	
	/*
	L1602_char(1,1,'X');
	L1602_char(1,2,':'); 
  L1602_char(1,4,'0'); 
	L1602_char(1,5,'.'); 
  L1602_char(1,6,'0'); 
  L1602_char(1,7,'0'); 
	L1602_char(1,8,' '); 
	*/
}
//***********************************************************************
//显示y轴
void display_y()
{     
	int fuhao=0;  // 0为正
	float temp, tt;
  dis_data=(BUF[3]<<8)+BUF[2];  //合成数据   
	if(dis_data<0){
		dis_data=-dis_data;
		fuhao=1;
    //L1602_char(1,11,'-');      //显示正负符号位
	}
	//else L1602_char(1,11,' '); //显示空格

  temp=(float)dis_data*3.9;  //计算数据和显示,查考ADXL345快速入门第4页
  conversion(temp);          //转换出显示需要的数据
/*
	L1602_char(1,9,'Y');   //第1行，第0列 显示y
  L1602_char(1,10,':'); 
  L1602_char(1,12,qian); 
	L1602_char(1,13,'.'); 
  L1602_char(1,14,bai); 
  L1602_char(1,15,shi);  
	L1602_char(1,16,' ');  
*/

	recordY[traceY]=fuhao;
	if(fuhao==1)
		recordYData[traceY]=-(qian+0.1*bai+0.01*shi);
	else
		recordYData[traceY]=qian+0.1*bai+0.01*shi;
	traceY++;
	if(traceY>2)
		traceY=0;

/*	if(fuhao==1)
	{
	  tt=-(qian+0.1*bai+0.01*shi)-preY;
		preY=-(qian+0.1*bai+0.01*shi);
	} else
	{
		tt=qian+0.1*bai+0.01*shi-preY;
		preY=qian+0.1*bai+0.01*shi;
	}
	
	if(tt<0)
		cmpY = -tt;
	else
		cmpY = tt;
	*/
	/*
	L1602_char(1,9,'Y');   
  L1602_char(1,10,':'); 
  L1602_char(1,12,'0'); 
	L1602_char(1,13,'.'); 
  L1602_char(1,14,'0'); 
  L1602_char(1,15,'0');  
	L1602_char(1,16,' ');  
	*/
}

//***********************************************************************
//显示z轴
void display_z()
{
	
  float temp;
  dis_data=(BUF[5]<<8)+BUF[4];    //合成数据   
	if(dis_data<0){
		dis_data=-dis_data;
    //L1602_char(2,3,'-');       //显示负符号位
	}
	//else L1602_char(2,3,' ');  //显示空格

  temp=(float)dis_data*3.9;  //计算数据和显示,查考ADXL345快速入门第4页
  conversion(temp);          //转换出显示需要的数据
/*	
	L1602_char(2,1,'Z');  //第0行，第10列 显示Z
  L1602_char(2,2,':'); 
  L1602_char(2,4,qian); 
	L1602_char(2,5,'.'); 
  L1602_char(2,6,bai); 
  L1602_char(2,7,shi); 
	L1602_char(2,8,' ');  
*/
	/*
	L1602_char(2,1,'Z'); 
  L1602_char(2,2,':'); 
  L1602_char(2,4,'0'); 
	L1602_char(2,5,'.'); 
  L1602_char(2,6,'0'); 
  L1602_char(2,7,'0'); 
	L1602_char(2,8,' ');  
	*/
}

void Data_Process()
{
	/*float temp;
	if(cmpX<cmpY)
		temp=cmpY;
	else
		temp=cmpX;
	
	if(temp>threshold) //超过阈值记为一步
	{
		step++;  
	}*/
	int zhengX=0; //没有
	int fuX=0; //没有
	int fuXLoca;
	
	int zhengY=0;
	int fuY=0;
	int fuYLoca;
	
	int i;
	
	if(traceX==0&&traceY==0)
	{
		for(i=0; i<3; i++)
		{
			if(recordX[i]==0)
				zhengX=1;
			else if(recordX[i]==1)
			{
				fuX=1;
				fuXLoca=i;
			}
				
			if(recordY[i]==0)
				zhengY=1;
			else if(recordY[i]==1)
			{
				fuY=1;
				fuYLoca=i;
			}
				
		}
		/*
		if((zhengX==1&&fuX==1)||(zhengY==1&&fuY==1))
			step++;
		*/
		if(zhengX==1&&fuX==1)
		{
		  if(recordXData[fuXLoca]<=threshold)
				step++;
		}else if(zhengY==1&&fuY==1)
		{
			if(recordYData[fuYLoca]<=threshold)
				step++;
		}
	}
		
}

void show_steps()
{
	uint temp_data;
	uchar show;
	uchar qian, bai, shi, ge;
	if(step<=9)
	{
	  show = step+0x30;
		L1602_char(2,16,show); 
	} else if(step<=99)
	{
		shi=step/10;
		ge=step%10;
		show = shi+0x30;
		L1602_char(2,15,show);
		show = ge+0x30;
		L1602_char(2,16,show);
	} else if(step<=999)
	{
 
    bai=step/100+0x30   ;
		L1602_char(2,14,bai);
    temp_data=step%100;     //取余运算
    shi=temp_data/10+0x30    ;
		L1602_char(2,15,shi);
    temp_data=temp_data%10;      //取余运算
    ge=temp_data+0x30; 	
		L1602_char(2,16,ge);
	} else if(step<=9999)
	{
		qian=step/1000+0x30 ;
		L1602_char(2,13,qian);
    temp_data=temp_data%1000;    //取余运算
    bai=temp_data/100+0x30;
		L1602_char(2,14,bai);
    temp_data=temp_data%100;     //取余运算
    shi=temp_data/10+0x30    ;
		L1602_char(2,15,shi);
    temp_data=temp_data%10;      //取余运算
    ge=temp_data+0x30; 	
		L1602_char(2,16,ge);
	} else
		step=0;
}

/********************************************************************
*  main函数
*  
***********************************************************************/
void Main()
{
	uchar devid;
	uchar flag = 0;
	
	P10=0;
	
	Delay(30);	 
	L1602_init();
	
	//L1602_string(2,1,"  The MCU World ");
  //L1602_char(1,1,'*');
	//L1602_char(1,16,'*');

	// ADXL345
	Init_ADXL345();	
	devid=Single_Read_ADXL345(0X00);	//读出的数据为0XE5,表示正确
	/*L1602_char(2,8,'Q');
	L1602_char(2,9,'I');
	L1602_char(2,10,'A'); 
  L1602_char(2,11,'N'); 
	L1602_char(2,12,'Y'); 
  L1602_char(2,13,'U'); */
	L1602_string(1,1,"     STEPS      ");
	
	while(1)
	{
		flag++;
		Multiple_Read_ADXL345();       	//连续读出数据，存储在BUF中
		display_x();                   	//---------显示X轴
		display_y();                   	//---------显示Y轴
		//display_z();                   	//---------显示Z轴
		Data_Process();
		Delay(200);
		
		if(flag==5)
		{
			flag=0;
			show_steps();
		}
	}
}