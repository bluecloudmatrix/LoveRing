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

sbit	  SCL=P1^4;      //IICʱ�����Ŷ���
sbit 	  SDA=P1^5;      //IIC�������Ŷ���

sbit E=P2^2;		//1602ʹ��
sbit RW=P2^1;		//1602��д
sbit RS=P2^0;		//1602����ָ�������

#define	SlaveAddress   0xA6	  //����������IIC�����еĴӵ�ַ,����ALT  ADDRESS��ַ���Ų�ͬ�޸�
                              //ALT  ADDRESS���Žӵ�ʱ��ַΪ0xA6���ӵ�Դʱ��ַΪ0x3A
															//��ѡADXL345ģ��ALT ADDRESS����Ĭ�Ͻӵ�
typedef unsigned char  BYTE;
typedef unsigned short WORD;

uchar ge,shi,bai,qian,wan;           //��ʾ����

BYTE BUF[8];   											 // ��ADXL345�ж�ȡ���ݻ�����
int  dis_data;                       //����

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
void Init_ADXL345(void);             //��ʼ��ADXL345
void  Single_Write_ADXL345(uchar REG_Address,uchar REG_data);   //����д������

void  Multiple_Read_ADXL345();                                  //�����Ķ�ȡ�ڲ��Ĵ�������

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
    temp_data=temp_data%10000;   //ȡ������
		qian=temp_data/1000+0x30 ;
    temp_data=temp_data%1000;    //ȡ������
    bai=temp_data/100+0x30   ;
    temp_data=temp_data%100;     //ȡ������
    shi=temp_data/10+0x30    ;
    temp_data=temp_data%10;      //ȡ������
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
* ��ʱ5����(STC90C52RC@12M)
* ��ͬ�Ĺ�������,��Ҫ�����˺���
* ������1T��MCUʱ,���������ʱ����
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
��ʱ5΢��(STC90C52RC@12M)
��ͬ�Ĺ�������,��Ҫ�����˺�����ע��ʱ�ӹ���ʱ��Ҫ�޸�
������1T��MCUʱ,���������ʱ����
**************************************/
void Delay5us()
{
    _nop_();_nop_();_nop_();_nop_();
    _nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();
}


/**************************************
* ADXL345
��ʼ�ź�
**************************************/
void ADXL345_Start()
{
    SDA = 1;                    //����������
    SCL = 1;                    //����ʱ����
    Delay5us();                 //��ʱ
    SDA = 0;                    //�����½���
    Delay5us();                 //��ʱ
    SCL = 0;                    //����ʱ����
}

/**************************************
* ADXL345
ֹͣ�ź�
**************************************/
void ADXL345_Stop()
{
    SDA = 0;                    //����������
    SCL = 1;                    //����ʱ����
    Delay5us();                 //��ʱ
    SDA = 1;                    //����������
    Delay5us();                 //��ʱ
}

/**************************************
* ADXL345
����Ӧ���ź�
��ڲ���:ack (0:ACK 1:NAK)
**************************************/
void ADXL345_SendACK(bit ack)
{
    SDA = ack;                  //дӦ���ź�
    SCL = 1;                    //����ʱ����
    Delay5us();                 //��ʱ
    SCL = 0;                    //����ʱ����
    Delay5us();                 //��ʱ
}

/**************************************
* ADXL345
����Ӧ���ź�
**************************************/
bit ADXL345_RecvACK()
{
    SCL = 1;                    //����ʱ����
    Delay5us();                 //��ʱ
    CY = SDA;                   //��Ӧ���ź�
    SCL = 0;                    //����ʱ����
    Delay5us();                 //��ʱ

    return CY;
}

/**************************************
* ADXL345
��IIC���߷���һ���ֽ�����
**************************************/
void ADXL345_SendByte(BYTE dat)
{
    BYTE i;

    for (i=0; i<8; i++)         //8λ������
    {
        dat <<= 1;              //�Ƴ����ݵ����λ
        SDA = CY;               //�����ݿ�
        SCL = 1;                //����ʱ����
        Delay5us();             //��ʱ
        SCL = 0;                //����ʱ����
        Delay5us();             //��ʱ
    }
    ADXL345_RecvACK();
}

/**************************************
* ADXL345
��IIC���߽���һ���ֽ�����
**************************************/
BYTE ADXL345_RecvByte()
{
    BYTE i;
    BYTE dat = 0;

    SDA = 1;                    //ʹ���ڲ�����,׼����ȡ����,
    for (i=0; i<8; i++)         //8λ������
    {
        dat <<= 1;
        SCL = 1;                //����ʱ����
        Delay5us();             //��ʱ
        dat |= SDA;             //������               
        SCL = 0;                //����ʱ����
        Delay5us();             //��ʱ
    }
    return dat;
}

//******���ֽ�д��*******************************************
void Single_Write_ADXL345(uchar REG_Address,uchar REG_data)
{
    ADXL345_Start();                  //��ʼ�ź�
    ADXL345_SendByte(SlaveAddress);   //�����豸��ַ+д�ź�
    ADXL345_SendByte(REG_Address);    //�ڲ��Ĵ�����ַ����ο�����pdf22ҳ 
    ADXL345_SendByte(REG_data);       //�ڲ��Ĵ������ݣ���ο�����pdf22ҳ 
    ADXL345_Stop();                   //����ֹͣ�ź�
}

//********���ֽڶ�ȡ*****************************************
uchar Single_Read_ADXL345(uchar REG_Address)
{  uchar REG_data;
    ADXL345_Start();                          //��ʼ�ź�
    ADXL345_SendByte(SlaveAddress);           //�����豸��ַ+д�ź�
    ADXL345_SendByte(REG_Address);            //���ʹ洢��Ԫ��ַ����0��ʼ	
    ADXL345_Start();                          //��ʼ�ź�
    ADXL345_SendByte(SlaveAddress+1);         //�����豸��ַ+���ź�
    REG_data=ADXL345_RecvByte();              //�����Ĵ�������
	ADXL345_SendACK(1);   
	ADXL345_Stop();                           //ֹͣ�ź�
    return REG_data; 
}

//*********************************************************
//
//��������ADXL345�ڲ����ٶ����ݣ���ַ��Χ0x32~0x37
//
//*********************************************************
void Multiple_read_ADXL345(void)
{   uchar i;
    ADXL345_Start();                          //��ʼ�ź�
    ADXL345_SendByte(SlaveAddress);           //�����豸��ַ+д�ź�
    ADXL345_SendByte(0x32);                   //���ʹ洢��Ԫ��ַ����0x32��ʼ	
    ADXL345_Start();                          //��ʼ�ź�
    ADXL345_SendByte(SlaveAddress+1);         //�����豸��ַ+���ź�
	 for (i=0; i<6; i++)                      //������ȡ6����ַ���ݣ��洢��BUF
    {
        BUF[i] = ADXL345_RecvByte();          //BUF[0]�洢0x32��ַ�е�����
        if (i == 5)
        {
           ADXL345_SendACK(1);                //���һ��������Ҫ��NOACK
        }
        else
        {
          ADXL345_SendACK(0);                //��ӦACK
       }
   }
    ADXL345_Stop();                          //ֹͣ�ź�
    Delay5ms();
}


//*****************************************************************

//��ʼ��ADXL345��������Ҫ��ο�pdf�����޸�************************
void Init_ADXL345()
{
   Single_Write_ADXL345(0x31,0x0B);   //������Χ,����16g��13λģʽ
   Single_Write_ADXL345(0x2C,0x08);   //�����趨Ϊ12.5 �ο�pdf13ҳ
   Single_Write_ADXL345(0x2D,0x08);   //ѡ���Դģʽ   �ο�pdf24ҳ
   Single_Write_ADXL345(0x2E,0x80);   //ʹ�� DATA_READY �ж�
   Single_Write_ADXL345(0x1E,0x00);   //X ƫ���� ���ݲ��Դ�������״̬д��pdf29ҳ
   Single_Write_ADXL345(0x1F,0x00);   //Y ƫ���� ���ݲ��Դ�������״̬д��pdf29ҳ
   Single_Write_ADXL345(0x20,0x05);   //Z ƫ���� ���ݲ��Դ�������״̬д��pdf29ҳ
}
//***********************************************************************

//***********************************************************************
//��ʾx��
void display_x()
{   
	int fuhao=0; // 0Ϊ��
	float temp, tt;
  dis_data=(BUF[1]<<8)+BUF[0];  //�ϳ�����   
	if(dis_data<0){
		fuhao=1;
		dis_data=-dis_data;
    //L1602_char(1,3,'-');      //��ʾ��������λ
	}
	//else L1602_char(1,3,' '); //��ʾ�ո�

  temp=(float)dis_data*3.9;  //�������ݺ���ʾ,�鿼ADXL345�������ŵ�4ҳ
  conversion(temp);          //ת������ʾ��Ҫ������
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
//��ʾy��
void display_y()
{     
	int fuhao=0;  // 0Ϊ��
	float temp, tt;
  dis_data=(BUF[3]<<8)+BUF[2];  //�ϳ�����   
	if(dis_data<0){
		dis_data=-dis_data;
		fuhao=1;
    //L1602_char(1,11,'-');      //��ʾ��������λ
	}
	//else L1602_char(1,11,' '); //��ʾ�ո�

  temp=(float)dis_data*3.9;  //�������ݺ���ʾ,�鿼ADXL345�������ŵ�4ҳ
  conversion(temp);          //ת������ʾ��Ҫ������
/*
	L1602_char(1,9,'Y');   //��1�У���0�� ��ʾy
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
//��ʾz��
void display_z()
{
	
  float temp;
  dis_data=(BUF[5]<<8)+BUF[4];    //�ϳ�����   
	if(dis_data<0){
		dis_data=-dis_data;
    //L1602_char(2,3,'-');       //��ʾ������λ
	}
	//else L1602_char(2,3,' ');  //��ʾ�ո�

  temp=(float)dis_data*3.9;  //�������ݺ���ʾ,�鿼ADXL345�������ŵ�4ҳ
  conversion(temp);          //ת������ʾ��Ҫ������
/*	
	L1602_char(2,1,'Z');  //��0�У���10�� ��ʾZ
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
	
	if(temp>threshold) //������ֵ��Ϊһ��
	{
		step++;  
	}*/
	int zhengX=0; //û��
	int fuX=0; //û��
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
    temp_data=step%100;     //ȡ������
    shi=temp_data/10+0x30    ;
		L1602_char(2,15,shi);
    temp_data=temp_data%10;      //ȡ������
    ge=temp_data+0x30; 	
		L1602_char(2,16,ge);
	} else if(step<=9999)
	{
		qian=step/1000+0x30 ;
		L1602_char(2,13,qian);
    temp_data=temp_data%1000;    //ȡ������
    bai=temp_data/100+0x30;
		L1602_char(2,14,bai);
    temp_data=temp_data%100;     //ȡ������
    shi=temp_data/10+0x30    ;
		L1602_char(2,15,shi);
    temp_data=temp_data%10;      //ȡ������
    ge=temp_data+0x30; 	
		L1602_char(2,16,ge);
	} else
		step=0;
}

/********************************************************************
*  main����
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
	devid=Single_Read_ADXL345(0X00);	//����������Ϊ0XE5,��ʾ��ȷ
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
		Multiple_Read_ADXL345();       	//�����������ݣ��洢��BUF��
		display_x();                   	//---------��ʾX��
		display_y();                   	//---------��ʾY��
		//display_z();                   	//---------��ʾZ��
		Data_Process();
		Delay(200);
		
		if(flag==5)
		{
			flag=0;
			show_steps();
		}
	}
}