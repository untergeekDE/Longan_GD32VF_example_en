/**************************************************************************
 * LCD Display library functions										*
 * Code CC-BY untergeekDE
 * 
 * Based on the LCD display demo code by Sipeed under Apache 2.0 License
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Reuses parts of Deloarts OLED library. 
 * 
 * OK team, what have we got here? 
 * This is a backlit 160x80px LC color display. 
 * Individual pixels are 16-bit RGB values (0brrrrrggggggbbbbb - 
 * see the color constants in the lcd.h file).
 * Data is written en bloc, by sending the display a range of pixels to fill,
 * defined by x and y offset, width, and height,  
 * and then banging the 16-bit values for the pixels to the SPI bus. 
 * Data is written line by line, in descending order. 
 * (0,0) is in the upper left corner, 
 * (159,79) in the lower right corner.
 * 
 * The original Sipeed code is plain and simple:
 * - Init for the communication 
 * - Routines to write data to the SPI bus: single pixel, range...
 * - Crude but fast routines to write lines and circles, filled and empty rectangles.
 * - A routine to print 8x16px ASCII characters to any position.
 * - A routine to print 16x16 or 32x32 Chinese characters to any position
 * - A logo display routine.  
 * - Routines to print character strings, and numbers. 
 * 
 * So this is what I took from my old code: 
 * - A routine to print 8x8 characters from ye olde CP437/MSDOS charset, 
 *   in fontsize=1 (8x8), 2 (16x16), 3 (24x24), using the Scale2x and Scale3x algo
 * - A sorta BITBLT routine
 * - I will make the string and numbers routines use my character print routine. 
 * 
 * Just for the record: 
 * - The LCD_showChar routine by Sipeed gives you 5 lines of 20 characters.
 * - My LCD_printChar routine gives you 10 lines of 20 very small characters,
 *   5x10 middle-sized ones, or 3 lines of 6 *large* characters.
 * 
 * *******************************************************************************/

#include "lcd/lcd.h"
#include <lcd/oledfont.h>
#include <lcd/8x8_vertikal_LSB_2.h>		//The CP437 font I ripped
#include <lcd/bmp.h>
u16 BACK_COLOR;   //Background color


/******************************************************************************
       Function description: LCD serial data write function
       Entry data: one byte to be written via serial 
       Return value: None
******************************************************************************/
void LCD_Writ_Bus(u8 dat) 
{
#if SPI0_CFG == 1
	OLED_CS_Clr();

	while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));
        spi_i2s_data_transmit(SPI0, dat);
	while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));
        spi_i2s_data_receive(SPI0);

	OLED_CS_Set();
#elif SPI0_CFG == 2
	spi_dma_enable(SPI0, SPI_DMA_TRANSMIT);
#else
	u8 i;
	OLED_CS_Clr();
	for(i=0;i<8;i++)
	{			  
		OLED_SCLK_Clr();
		if(dat&0x80)
		   OLED_SDIN_Set();
		else
		   OLED_SDIN_Clr();
		OLED_SCLK_Set();
		dat<<=1;
	}	
  OLED_CS_Set();	
#endif
}


/******************************************************************************
       Function description: LCD write data
       Entry data: unsigned byte value to write
       Return value: None
******************************************************************************/
void LCD_WR_DATA8(u8 dat)
{
	OLED_DC_Set();		// Set to "Write data"
	LCD_Writ_Bus(dat);	// Serial write the byte
}


/******************************************************************************
Function description: LCD write data
       Entry data: unsigned integer word to write
       Return value: None
******************************************************************************/
void LCD_WR_DATA(u16 dat)
{
	OLED_DC_Set();			//Set to "Write data"
	LCD_Writ_Bus(dat>>8);	//Serial write High byte
	LCD_Writ_Bus(dat);		//Serial write Low byte
}


/******************************************************************************
       Function description: LCD write command
       Entry data: unsigned command byte
       Return value: None
******************************************************************************/
void LCD_WR_REG(u8 dat)
{
	OLED_DC_Clr();		//Set to "Write command"
	LCD_Writ_Bus(dat);	// Send command byte via serial
}


/************************************************* *****************************
       Function description: Set start and end addresses for write


       Entry data: x1, x2 set the start and end column
                 y1, y2 set the start and end line
       Return value: None
************************************************** ****************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
	if(USE_HORIZONTAL==0)
	{
		LCD_WR_REG(0x2a);		//Column address settings
		LCD_WR_DATA(x1+26);		//x1 - x of 
		LCD_WR_DATA(x2+26);
		LCD_WR_REG(0x2b);		//Row address setting
		LCD_WR_DATA(y1+1);
		LCD_WR_DATA(y2+1);
		LCD_WR_REG(0x2c);		//Memory Write
	}
	else if(USE_HORIZONTAL==1)
	{
		LCD_WR_REG(0x2a);		//Column address settings
		LCD_WR_DATA(x1+26);
		LCD_WR_DATA(x2+26);
		LCD_WR_REG(0x2b);		//Row address setting
		LCD_WR_DATA(y1+1);
		LCD_WR_DATA(y2+1);
		LCD_WR_REG(0x2c);		//Memory write
	}
	else if(USE_HORIZONTAL==2)	//IT'S MEEEE!!! THIS IS WHAT WE ARE USING!!!!
	{
		LCD_WR_REG(0x2a);		//Column address settings
		LCD_WR_DATA(x1+1);		
		LCD_WR_DATA(x2+1);
		LCD_WR_REG(0x2b);		//Row address setting
		LCD_WR_DATA(y1+26);
		LCD_WR_DATA(y2+26);
		LCD_WR_REG(0x2c);		//Memory write
	}
	else
	{
		LCD_WR_REG(0x2a);		//Column address settings
		LCD_WR_DATA(x1+1);
		LCD_WR_DATA(x2+1);
		LCD_WR_REG(0x2b);		//Row address setting
		LCD_WR_DATA(y1+26);
		LCD_WR_DATA(y2+26);
		LCD_WR_REG(0x2c);		//Memory write
	}
}

/************************************************* *****************************
       Function description: Configure the DMA or SPI peripheral
       Entry data: none
       Return value: None
************************************************** ****************************/
#if SPI0_CFG == 2
void dma_config(void)
{
	dma_parameter_struct dma_init_struct;

    /* SPI0 transmit dma config:DMA0,DMA_CH2 */
    dma_deinit(DMA0, DMA_CH2);
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.periph_addr  = (uint32_t)&SPI_DATA(SPI0);
    dma_init_struct.memory_addr  = (uint32_t)image;
    dma_init_struct.direction    = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority     = DMA_PRIORITY_LOW;
    dma_init_struct.number       = FRAME_SIZE;
    dma_init_struct.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
    dma_init(DMA0, DMA_CH2, &dma_init_struct);
    /* configure DMA mode */
    dma_circulation_disable(DMA0, DMA_CH2);
    dma_memory_to_memory_disable(DMA0, DMA_CH2);
}
#endif

#if SPI0_CFG == 1
/************************************************* *****************************
       Function description: Configure the DMA or SPI peripheral
       Entry data: none
       Return value: None
************************************************** ****************************/
void spi_config(void)
{
    spi_parameter_struct spi_init_struct;
    /* deinitilize SPI and the parameters */
    OLED_CS_Set();
    spi_struct_para_init(&spi_init_struct);

    /* SPI0 parameter config */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_8;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(SPI0, &spi_init_struct);

	spi_crc_polynomial_set(SPI0,7);
	spi_enable(SPI0);
}
#endif

/******************************************************************************
      LCD initialisation. Gets nothing, returns nothing. 
******************************************************************************/
void Lcd_Init(void)
{
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);


#if SPI0_CFG == 1
 	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_clock_enable(RCU_SPI0);
	/* SPI0 GPIO config: NSS/PA4, SCK/PA5, MOSI/PA7 */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 |GPIO_PIN_6| GPIO_PIN_7);
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);

	spi_config();

#elif SPI0_CFG == 2
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_SPI0);

	/* SPI0 GPIO config: NSS/PA4, SCK/PA5, MOSI/PA7 */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7);
    /* SPI0 GPIO config: MISO/PA6 */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6);

	dma_config();

	dma_channel_enable(DMA0,DMA_CH2);
#elif SPI0_CFG == 3
	gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_7);
	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);

	gpio_bit_reset(GPIOA, GPIO_PIN_5 | GPIO_PIN_7);
	gpio_bit_reset(GPIOB, GPIO_PIN_2);
#endif

	gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1);
	gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1);

	OLED_RST_Clr();
	delay_1ms(200);
	OLED_RST_Set();
	delay_1ms(20);
	OLED_BLK_Set();

	LCD_WR_REG(0x11);	// turn off sleep mode
	delay_1ms(100);

	LCD_WR_REG(0x21);	// display inversion mode

	LCD_WR_REG(0xB1);	// Set the frame frequency of the full colors normal mode
						// Frame rate=fosc/((RTNA x 2 + 40) x (LINE + FPA + BPA +2))
						// fosc = 850kHz
	LCD_WR_DATA8(0x05);	// RTNA
	LCD_WR_DATA8(0x3A);	// FPA
	LCD_WR_DATA8(0x3A);	// BPA

	LCD_WR_REG(0xB2);	// Set the frame frequency of the Idle mode
						// Frame rate=fosc/((RTNB x 2 + 40) x (LINE + FPB + BPB +2))
						// fosc = 850kHz
	LCD_WR_DATA8(0x05);	// RTNB
	LCD_WR_DATA8(0x3A);	// FPB
	LCD_WR_DATA8(0x3A);	// BPB

	LCD_WR_REG(0xB3);	// Set the frame frequency of the Partial mode/ full colors
	LCD_WR_DATA8(0x05);  
	LCD_WR_DATA8(0x3A);
	LCD_WR_DATA8(0x3A);
	LCD_WR_DATA8(0x05);
	LCD_WR_DATA8(0x3A);
	LCD_WR_DATA8(0x3A);

	LCD_WR_REG(0xB4);
	LCD_WR_DATA8(0x03);

	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x62);
	LCD_WR_DATA8(0x02);
	LCD_WR_DATA8(0x04);

	LCD_WR_REG(0xC1);
	LCD_WR_DATA8(0xC0);

	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x00);

	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x8D);
	LCD_WR_DATA8(0x6A);   

	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x8D); 
	LCD_WR_DATA8(0xEE); 

	LCD_WR_REG(0xC5);  /*VCOM*/
	LCD_WR_DATA8(0x0E);    

	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0x10);
	LCD_WR_DATA8(0x0E);
	LCD_WR_DATA8(0x02);
	LCD_WR_DATA8(0x03);
	LCD_WR_DATA8(0x0E);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x02);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x0A);
	LCD_WR_DATA8(0x12);
	LCD_WR_DATA8(0x27);
	LCD_WR_DATA8(0x37);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x0E);
	LCD_WR_DATA8(0x10);

	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0x10);
	LCD_WR_DATA8(0x0E);
	LCD_WR_DATA8(0x03);
	LCD_WR_DATA8(0x03);
	LCD_WR_DATA8(0x0F);
	LCD_WR_DATA8(0x06);
	LCD_WR_DATA8(0x02);
	LCD_WR_DATA8(0x08);
	LCD_WR_DATA8(0x0A);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x26);
	LCD_WR_DATA8(0x36);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x0E);
	LCD_WR_DATA8(0x10);

	LCD_WR_REG(0x3A);	// define the format of RGB picture data
	LCD_WR_DATA8(0x05);	// 16-bit/pixel

	LCD_WR_REG(0x36);
	if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x08);
	else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC8);
	else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x78);
	else LCD_WR_DATA8(0xA8);

	LCD_WR_REG(0x29);	// Display On
}

/******************************************************************************
 * 		Erase to 16-bit Color value, pixel by pixel. (Duh, slow, that.)
******************************************************************************/
void LCD_Clear(u16 Color)
{
	u16 i,j;  	
	LCD_Address_Set(0,0,LCD_W-1,LCD_H-1);
    for(i=0;i<LCD_W;i++)
	  {
			for (j=0;j<LCD_H;j++)
				{
					LCD_WR_DATA(Color);
				}
	  }
}



/******************************************************************************
Function: Draw a colored pixel
	Needs: x and y position, 16-bit color
******************************************************************************/
void LCD_DrawPoint(u16 x,u16 y,u16 color)
{
	LCD_Address_Set(x,y,x,y);	//Frame to write to 
	LCD_WR_DATA(color);
} 


/******************************************************************************
Function description: Draw a 3x3px block in color 
       Entry data: x, y starting coordinates
       Return value: None
******************************************************************************/
void LCD_DrawPoint_big(u16 x,u16 y,u16 color)
{
	LCD_Fill(x-1,y-1,x+1,y+1,color);
} 


/******************************************************************************
Function description: fill rectangle with specified color
       Entry data: xsta, ysta starting coordinates
                 xend, yend termination coordinates
       Return value: None
******************************************************************************/
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color)
{          
	u16 i,j; 
	LCD_Address_Set(xsta,ysta,xend,yend);      // Frame to fill 
	for(i=ysta;i<=yend;i++)
	{													   	 	
		for(j=xsta;j<=xend;j++)LCD_WR_DATA(color);//Send Data 	    
	} 					  	    
}


/******************************************************************************
Function description: draw line in color color
       Entry data: x1, y1 starting coordinates
                 x2, y2 end coordinates
       Return value: None
******************************************************************************/
void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //Number of horizontal and vertical steps
	delta_y=y2-y1;
	uRow=x1;//Start position
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向 
	else if (delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线 
	else {incy=-1;delta_y=-delta_x;}
	if(delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		LCD_DrawPoint(uRow,uCol,color);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}


/******************************************************************************
 * 	Draw an empty rectangle
******************************************************************************/
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2,u16 color)
{
	LCD_DrawLine(x1,y1,x2,y1,color);
	LCD_DrawLine(x1,y1,x1,y2,color);
	LCD_DrawLine(x1,y2,x2,y2,color);
	LCD_DrawLine(x2,y1,x2,y2,color);
}

/******************************************************************************
Function description: fast circle draw
       Entry data: x0, y0 center coordinates
                 r radius
       Return value: None
******************************************************************************/
void Draw_Circle(u16 x0,u16 y0,u8 r,u16 color)
{
	int a,b;
	// int di;
	a=0;b=r;	
	// Write eight segments of circle at once  
	while(a<=b)
	{
		LCD_DrawPoint(x0-b,y0-a,color);             //3           
		LCD_DrawPoint(x0+b,y0-a,color);             //0           
		LCD_DrawPoint(x0-a,y0+b,color);             //1                
		LCD_DrawPoint(x0-a,y0-b,color);             //2             
		LCD_DrawPoint(x0+b,y0+a,color);             //4               
		LCD_DrawPoint(x0+a,y0-b,color);             //5
		LCD_DrawPoint(x0+a,y0+b,color);             //6 
		LCD_DrawPoint(x0-b,y0+a,color);             //7
		a++;										// Increase x offset
		if((a*a+b*b)>(r*r))							// If radius reached, decrease y offset
		{
			b--;
		}
	}
}




/******************************************************************************
Function description: display string (no wrapping)
       Entry data: x, y starting point coordinates
                 * p string start address
       Return value: None
******************************************************************************/
void LCD_ShowString(u16 x,u16 y,const u8 *p,u16 color)
{         
	LCD_ShowStringX(x,y,p,color,SMALL);
}

void LCD_ShowStringX(u16 x,u16 y,const u8 *p,u16 color, u8 fontsize)
{         
	while(*p!='\0')
    {       
        LCD_printChar(x,y,*p,color,fontsize);
        x+=8;
		if (fontsize>1) x+=8;
		if (fontsize>3) x+=8;
		if (x > LCD_W)
		{
			x -= LCD_W;
			y += 8+(fontsize*8);
		}
        p++;
    }  
}

/******************************************************************************
Function description: nth power of m
       Entry data: base m, n exponent
       Return value: m^n
******************************************************************************/
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}


/******************************************************************************
Function description: display numbers
       Entry data: x, y starting point coordinates
                 num unsigned int number to display
                 len number of digits to display
       Return value: None
******************************************************************************/
void LCD_ShowNum(u16 x,u16 y,u16 num,u8 len,u16 color)
{         	
	LCD_ShowNumX(x,y,num,len,color,SMALL);
} 

void LCD_ShowNumX(u16 x,u16 y,u16 num,u8 len,u16 color, u8 fontsize)
{         	
	u8 t,temp;
	u8 enshow=0;
	fontsize++;
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				LCD_printChar(x+8*fontsize*t,y,' ',color,fontsize);
				continue;
			}else enshow=1; 
		 	 
		}
	 	LCD_ShowChar(x+8*fontsize*t,y,temp+48,0,color); 
	}
} 


/******************************************************************************
Function description: display decimal
       Entry data: x, y starting point coordinates
                 num floating point decimal to display
                 len number of digits to display
       Return value: None
******************************************************************************/

void LCD_ShowNum1(u16 x,u16 y,float num,u8 len,u16 color)
{
	LCD_ShowNum1X(x,y,num,len,color,SMALL);
}

void LCD_ShowNum1X(u16 x,u16 y,float num,u8 len,u16 color,u8 fontsize)
{         	
	u8 t,temp;
	// u8 enshow=0;
	u16 num1;
	num1=num*100;
	for(t=0;t<len;t++)
	{
		temp=(num1/mypow(10,len-t-1))%10;
		if(t==(len-2))
		{
			LCD_printChar(x+8*fontsize*(len-2),y,'.',color,fontsize);
			t++;
			len+=1;
		}
	 	LCD_printChar(x+8*fontsize*t,y,temp+48,color,fontsize);
	}
}


/******************************************************************************
Function description: display 40x40 picture
       Entry data: x, y starting point coordinates
       Return value: None
******************************************************************************/
void LCD_ShowPicture(u16 x1,u16 y1,u16 x2,u16 y2)
{
	int i;
	LCD_Address_Set(x1,y1,x2,y2);
	for(i=0;i<12800;i++)
	{ 	
		// LCD_WR_DATA8(image[i*2+1]);
		LCD_WR_DATA8(image[i]);
	}			
}

void LCD_ShowLogo(void)
{
	int i;
	LCD_Address_Set(0,0,159,75);
	for(i=0;i<25600;i++)
	{
		LCD_WR_DATA8(logo_bmp[i]);
	}			
}



/* #####################################################################################################################
   ##### DRAW BITMAP ###################################################################################################
   #####################################################################################################################
* Copies a 16bit-per-pixel bitmap. 
* Careful, Laddie: This routine does not check for bounds. It just takes a pointer and starts on copying
* as long as X and Y have not been reached; don't use it on memory that does not belong to you. Srsly. */

void LCD_drawBitmap(u16 *b, u8 x1, u8 y1, u8 Width, u8 Height)
{
	u8 x2 = x1+Width-1;
	u8 y2 = y1+Height-1;
	u16 i;
	if (x2 >= LCD_W) return; // Do nothing if out of bounds.
	if (y2 >= LCD_H) return; // Do nothing if out of bounds.	 
	// TODO: Write a clipping routine. 

	LCD_Address_Set(x1,y1,x2,y2); 
	for(i=0;i<Width*Height;i++)
	{						 
		LCD_WR_DATA(*b++); 	    
	} 	
}


/******************************************************************************
Function description: display characters (8x16px ASCII font)
       Entry data: x, y starting point coordinates
                 num characters to display
                 mode: 1 superimpose 0 overwrite bg 
       Return value: None
******************************************************************************/
void LCD_ShowChar(u16 x,u16 y,u8 num,u8 mode,u16 color)
{
    u8 temp;
    u8 pos,t;
	  u16 x0=x;    
    if(x>LCD_W-16||y>LCD_H-16)return;	    // Setting window: Out-of-bounds chars ignored	   
	num=num-' ';							// ASCII offset: Don't print anything below 32 
	LCD_Address_Set(x,y,x+8-1,y+16-1);      // Set cursor position
	if(!mode) 								// Overwrite
	{
		for(pos=0;pos<16;pos++)
		{ 
			temp=asc2_1608[(u16)num*16+pos];		 //Copy from 1608 font
			for(t=0;t<8;t++)
		    {                 
		        if(temp&0x01)LCD_WR_DATA(color);
				else LCD_WR_DATA(BACK_COLOR);
				temp>>=1;
				x++;
		    }
			x=x0;
			y++;
		}	
	}else											// Transparent
	{
		for(pos=0;pos<16;pos++)
		{
		    temp=asc2_1608[(u16)num*16+pos];		 //Call 1608 font
			for(t=0;t<8;t++)
		    {                 
		        if(temp&0x01)LCD_DrawPoint(x+t,y+pos,color);//Plot pixel in color    
		        temp>>=1; 
		    }
		}
	}   	   	 	  
}


// My repossessed OLED_I2C_128x64 print routine.
// Modular rewrite. As everything is pixels here anyway, 
// we might just as well use pixel arrays and copy those. 

/********************************************************
 * Function: u16 dampenColor(color)
 * Calculates a color value with half the brightness. 
 * more of a macro
 * 
 * Colors are 16-bit values of the form 
 * RRRRRGGGGGGBBBBBb
 ********************************************************/
u16 dampenColor(u16 color)
{
	return ((color & 0xF800) >> 1) | ((color & 0x07E0) >> 1) | ((color & 0x001F) >> 1);
}

/********************************************************
 * Function: scale2x
 * Input: pointer to an array of unsigned integers, x and y dimensions
 * Needs a buffer at least 2x the size
 * The image to double 
 * Returns the array filled with a bitmap of the doubled size
 * Uses a modified version of the scale2x pixel doubler 
*********************************************************/
void scale2x(u16 b[X_BUF][Y_BUF], u8 x, u8 y)
{	
	// If the bitmap array is smaller than the dimensions, exit
	if (X_BUF < x*2 || Y_BUF < y*2) return;
	// Start filling from lower right corner, 
	// which is empty right now, and then start  
	// 1101 -> 10100010
	for (int yy=y-1;yy>=0;yy--)
	{
		for(int xx=x-1;xx>=0;xx--)
		{
			// scale2x magic borrowed from scale2x.it
			// Pixel names in source matrix: 
			//  A B C
			//  D E F
			//  G H I 
			//
			u16 B = (yy>0) ? b[xx][yy-1] : 0; //if out of bounds, set 0
			u16 H = (yy<y-1) ? b[xx][yy+1] : 0;
			u16 D = (xx>0) ? b[xx-1][yy] : 0;
			u16 F = (xx<x-1) ? b[xx+1][yy] : 0;
			
			if (B != H && D != F) {
				b[2*xx][2*yy] = D == B ? D : b[xx][yy];
				b[2*xx+1][2*yy] = B == F ? F : b[xx][yy];
				b[2*xx][2*yy+1] = D == H ? D : b[xx][yy];
				b[2*xx+1][2*yy+1] = H == F ? F : b[xx][yy];
			} else {
				u16 E = b[xx][yy];
				b[2*xx][2*yy] = E;
				b[2*xx][2*yy+1] = E;
				b[2*xx+1][2*yy] = E;
				b[2*xx+1][2*yy+1] = E;
			}	
		}
	}
}

void scale3x(u16 b[X_BUF][Y_BUF], u8 x, u8 y)
{	
	// If the bitmap array is smaller than the dimensions, exit
	if (X_BUF < x*3 || Y_BUF < y*3) return;
	// Start filling from lower right corner, 
	// which is empty right now, and then start  
	// 1101 -> 10100010
	for (int yy=y-1;yy>=0;yy--)
	{
		for(int xx=x-1;xx>=0;xx--)
		{
			u16 A = (yy>0 && xx> 0) ? b[xx-1][yy-1] : 0;
			u16 B = (yy>0) ? b[xx][yy-1] : 0; //if out of bounds, set 0
			u16 C = (yy>0 && xx<x-1) ? b[xx+1][yy-1] : 0 ;
			u16 D = (xx>0) ? b[xx-1][yy] : 0;
			u16 E = b[xx][yy];
			u16 F = (xx<x-1) ? b[xx+1][yy] : 0;
			u16 G = (xx>0 && yy<y-1) ? b[xx-1][yy+1] : 0;
			u16 H = (yy<y-1) ? b[xx][yy+1] : 0;
			u16 I = (xx<x-1 && yy<y-1) ? b[xx+1][yy+1] : 0;
			

			if (B != H && D != F) {
				b[xx*3][yy*3] = D == B ? D : E;
				b[xx*3+1][yy*3] = (D == B && E != C) || (B == F && E != A) ? B : E;
				b[xx*3+2][yy*3] = B == F ? F : E;
				b[xx*3][yy*3+1] = (D == B && E != G) || (D == H && E != A) ? D : E;
				b[xx*3+1][yy*3+1] = E;
				b[xx*3+2][yy*3+1] = (B == F && E != I) || (H == F && E != C) ? F : E;
				b[xx*3][yy*3+2] = D == H ? D : E;
				b[xx*3+1][yy*3+2] = (D == H && E != I) || (H == F && E != G) ? H : E;
				b[xx*3+2][yy*3+2] = H == F ? F : E;
			} else {
				b[xx*3][yy*3] = E;
				b[xx*3+1][yy*3] = E;
				b[xx*3+2][yy*3] = E;
				b[xx*3][yy*3+1] = E;
				b[xx*3+1][yy*3+1] = E;
				b[xx*3+2][yy*3+1] = E;
				b[xx*3][yy*3+2] = E;
				b[xx*3+1][yy*3+2] = E;
				b[xx*3+2][yy*3+2] = E;
			}
		}
	}
}


/***************************************************************
 * function LCD_printChar
 * Prints character at x,y location in one of 4 font sizes: 
 * 0 = 8x8 font
 * 1 = 8x16 default font
 * 2 = 8x8 font doubled to 16x16
 * 3 = 8x16 font doubled to 16x32
 * IMPLEMENT LATER
 * 4 = 8x8 font tripled to 24x24
 * 5 = 8x16 font tripled to 24x48
 * *************************************************************/

void LCD_printChar(u16 x,u16 y,u8 c,u16 color,u8 fontsize)
{
	u16 buf[X_BUF][Y_BUF];
	u8 xx,yy;
	// Maximal bitmap size
	// DELAY TO DEBUG

// ###################################
//  fontsize=0 ==> Standard 8x8 chars
//  (Only do if not out of bounds)
// ###################################
if (fontsize == 0 && (x<LCD_W-7 && y<LCD_H-7))
	{	
		// Copy font to pixels
		LCD_Address_Set(x,y,x+7,y+7);
		// Step through the columns now. 
		for(yy = 0; yy < 8; yy++)
		{
			for(xx=0; xx < 8; xx++)
			{
				// LSB is on top; mask. 
				buf[xx][yy] = (((1 << yy) & BasicFont[c][xx]) != 0) ? color : BACK_COLOR;
				LCD_WR_DATA(buf[xx][yy]);
			}
		}
	}

// ###################################
//  fontsize=2 ==> Standard 8x8 chars
// scaled by scale2x
//  (Only do if not out of bounds)
// ###################################
if (fontsize == 2 && (x<LCD_W-15 && y<LCD_H-15))
	{	
		// Copy font to pixels
		// Step through the columns now. 
		for(yy = 0; yy < 8; yy++)
		{
			for(xx=0; xx < 8; xx++)
			{
				// LSB is on top; mask. 
				buf[xx][yy] = (((1 << yy) & BasicFont[c][xx]) != 0) ? color : BACK_COLOR;
			}
		}
		scale2x(buf,8,8);
		LCD_Address_Set(x,y,x+15,y+15);
		for (u8 yy=0;yy<16;yy++)
			for(u8 xx=0;xx<16;xx++)
				LCD_WR_DATA(buf[xx][yy]);
	}

// ###################################
//  fontsize=4 ==> Standard 8x8 chars
// scaled by scale3x
//  (Only do if not out of bounds)
// ###################################
if (fontsize == 4 && (x<LCD_W-23 && y<LCD_H-23))
	{	
		// Copy font to pixels
		// Step through the columns now. 
		for(yy = 0; yy < 8; yy++)
		{
			for(xx=0; xx < 8; xx++)
			{
				// LSB is on top; mask. 
				buf[xx][yy] = (((1 << yy) & BasicFont[c][xx]) != 0) ? color : BACK_COLOR;
			}
		}
		scale3x(buf,8,8);
		LCD_Address_Set(x,y,x+23,y+23);
		for (u8 yy=0;yy<24;yy++)
			for(u8 xx=0;xx<24;xx++)
				LCD_WR_DATA(buf[xx][yy]);
	}


// ###################################
//  fontsize==1 ==> use default font
//  (repossessing parts of the LCD_ShowChar routine here)
// ###################################
	if (fontsize == 1 && (x<LCD_W-7 && y<LCD_H-15))
	{
		// Copy default font to buffer. 
		c -= ' ';
		for(u16 yy=0;yy<16;yy++)
		{ 
			u16 temp=asc2_1608[(u16)c*16+yy];		 //Copy from 1608 font
			for(u16 xx=0;xx<8;xx++)
			{                 
				buf[xx][yy] = (temp&0x01) ? color : BACK_COLOR;
				temp>>=1;
			}
		}	
		//
		LCD_Address_Set(x,y,x+7,y+15);
		for (u8 yy=0;yy<16;yy++)
			for(u8 xx=0;xx<8;xx++)
				LCD_WR_DATA(buf[xx][yy]);
	} // end fontsize==1

// ###################################
//  fontsize==3 ==> use default font and scale2x it
//  (repossessing parts of the LCD_ShowChar routine here)
// ###################################
	if (fontsize == 3 && (x<LCD_W-15 && y<LCD_H-31))
	{
		// Copy default font to buffer. 
		c -= ' ';
		for(u16 yy=0;yy<16;yy++)
		{ 
			u16 temp=asc2_1608[(u16)c*16+yy];		 //Copy from 1608 font
			for(u16 xx=0;xx<8;xx++)
			{                 
				buf[xx][yy] = (temp&0x01) ? color : BACK_COLOR;
				temp>>=1;
			}
		}	
		//
		scale2x(buf,8,16);
		LCD_Address_Set(x,y,x+15,y+31);
		for (u8 yy=0;yy<32;yy++)
			for(u8 xx=0;xx<16;xx++)
				LCD_WR_DATA(buf[xx][yy]);
	} // end fontsize==3
// ###################################
//  fontsize==5 ==> use default font and scale3x it
//  (repossessing parts of the LCD_ShowChar routine here)
// ###################################
	if (fontsize == 5 && (x<LCD_W-23 && y<LCD_H-47))
	{
		// Copy default font to buffer. 
		c -= ' ';
		for(u16 yy=0;yy<16;yy++)
		{ 
			u16 temp=asc2_1608[(u16)c*16+yy];		 //Copy from 1608 font
			for(u16 xx=0;xx<8;xx++)
			{                 
				buf[xx][yy] = (temp&0x01) ? color : BACK_COLOR;
				temp>>=1;
			}
		}	
		//
		scale3x(buf,8,16);
		LCD_Address_Set(x,y,x+23,y+47);
		for (u8 yy=0;yy<48;yy++)
			for(u8 xx=0;xx<24;xx++)
				LCD_WR_DATA(buf[xx][yy]);
	} // end fontsize==5
}
