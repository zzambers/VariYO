#include <Arduino.h>
#include "Interrupt_counter.h"
#include "definitions.h"
#include "button_control.h"
#include "MS5611.h"
#include "buzzer.h"
#include "roundbuff.h"
#include <BMI160Gen.h>
#include "lib/kalmanfilter3.h"
#include "Variables.h"
#include "MEMS.h"
#include "Gauge.h"


#include <MadgwickAHRS/MadgwickAHRS.h>




Madgwick Madgwick_filter;




int zoufalepomocnapromenna = 0;
int flag_sec;

#include <NMEAGPS.h>
#include <GPSport.h>
#include <Streamers.h>

extern NMEAGPS  gps;
extern gps_fix  fix;




void counterInit() { // Set up the generic clock (GCLK4) used to clock timers
	REG_GCLK_GENDIV = GCLK_GENDIV_DIV(3) |          // Divide the 48MHz clock source by divisor 3: 48MHz/3=16MHz
	GCLK_GENDIV_ID(4);            // Select Generic Clock (GCLK) 4
	while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization


	REG_GCLK_GENCTRL = GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
	GCLK_GENCTRL_GENEN |         // Enable GCLK4
	GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
	GCLK_GENCTRL_ID(4);          // Select GCLK4
	while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization


	// Feed GCLK4 to TC4 and TC5
	REG_GCLK_CLKCTRL = GCLK_CLKCTRL_CLKEN |         // Enable GCLK4 to TC4 and TC5
	GCLK_CLKCTRL_GEN_GCLK4 |     // Select GCLK4
	GCLK_CLKCTRL_ID_TC4_TC5;     // Feed the GCLK4 to TC4 and TC5
	while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

	REG_TC4_CTRLA |= TC_CTRLA_MODE_COUNT8;           // Set the counter to 8-bit mode
	while (TC4->COUNT8.STATUS.bit.SYNCBUSY);        // Wait for synchronization

	REG_TC4_COUNT8_CC0 = 0xFF;                      // Set the TC4 CC0 register to some arbitary value
	while (TC4->COUNT8.STATUS.bit.SYNCBUSY);        // Wait for synchronization
	REG_TC4_COUNT8_CC1 = 0xFF;                      // Set the TC4 CC1 register to some arbitary value
	while (TC4->COUNT8.STATUS.bit.SYNCBUSY);        // Wait for synchronization
	REG_TC4_COUNT8_PER = 0xFF;                      // Set the PER (period) register to its maximum value
	while (TC4->COUNT8.STATUS.bit.SYNCBUSY);        // Wait for synchronization









	//NVIC_DisableIRQ(TC4_IRQn);
	//NVIC_ClearPendingIRQ(TC4_IRQn);
	NVIC_SetPriority(TC4_IRQn, 3);    // Set the Nested Vector Interrupt Controller (NVIC) priority for TC4 to 0 (highest)
	NVIC_EnableIRQ(TC4_IRQn);         // Connect TC4 to Nested Vector Interrupt Controller (NVIC)

	REG_TC4_INTFLAG |= TC_INTFLAG_MC1 | TC_INTFLAG_MC0 | TC_INTFLAG_OVF;        // Clear the interrupt flags
	REG_TC4_INTENSET = TC_INTENSET_MC1 | TC_INTENSET_MC0 | TC_INTENSET_OVF;     // Enable TC4 interrupts
	// REG_TC4_INTENCLR = TC_INTENCLR_MC1 | TC_INTENCLR_MC0 | TC_INTENCLR_OVF;     // Disable TC4 interrupts

	REG_TC4_CTRLA |= TC_CTRLA_PRESCALER_DIV1024 |     // Set prescaler to 64, 16MHz/64 = 256kHz
	TC_CTRLA_ENABLE;               // Enable TC4
	while (TC4->COUNT8.STATUS.bit.SYNCBUSY);        // Wait for synchronization





	///////////PIEZO BUZZER////////////////





}


void TC4_Handler()                              // Interrupt Service Routine (ISR) for timer TC4
{
	SPI_IRQ.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0));
	if(present_devices & LPS33_PRESENT){

		baro_readPressure();
		alt_baro = getAltitude()*100;



	}


	//read_mag();
	//IMU_read();
	
	if(present_devices & BMX160_PRESENT){
		for(int i = 0; i < IMU_FifoBytesToRead(); i+=FIFO_FRAME_SIZE){
			
			//		SerialUSB.print(inFifoWaiting);
			//	SerialUSB.print(",");
			//	SerialUSB.println(i);
			
			IMU_ReadFrameFromFifo();
			//	digitalWrite(SRAM_CS, 1);

			ax_corr = (float)(ax-statVar.offsetAccelX) * statVar.gainErrorAccelX ;
			ay_corr = (float)(ay-statVar.offsetAccelY) * statVar.gainErrorAccelY;
			az_corr = (float)(az-statVar.offsetAccelZ) * statVar.gainErrorAccelZ;

			
			mx_cor = mx*1000000 - statVar.offsetMagX;
			my_cor = my*1000000 - statVar.offsetMagY;
			mz_cor = mz*1000000 - statVar.offsetMagZ;
			mx_cor /= statVar.gainErrorMagX;
			my_cor /= statVar.gainErrorMagY;
			mz_cor /= statVar.gainErrorMagZ;
			
			
			/*
			ax_avg = ((ax_avg<<5)-ax_avg+ax)>>5;
			ay_avg = ((ay_avg<<5)- ay_avg+ay)>>5;
			az_avg = ((az_avg<<5)- az_avg+az)>>5;
			gx_avg = ((gx_avg<<5)- gx_avg+gx)>>5;
			gy_avg = ((gy_avg<<5)- gy_avg +gy)>>5;
			gz_avg = ((gz_avg<<5)- gz_avg+gz)>>5;
			*/
			
			if(axmax<ax) 	axmax = ax;
			if(aymax<ay) 	aymax = ay;
			if(azmax<az) 	azmax = az;
			
			if(axmin>ax) 	axmin = ax;
			if(aymin>ay) 	axmin = ay;
			if(axmin>az) 	axmin = az;
			
			if(gxmax<gx) 	gxmax = gx;
			if(gymax<gy) 	gymax = gy;
			if(gzmax<gz) 	gzmax = gz;
			
			if(gxmin>gx) 	gxmin = gx;
			if(gymin>gy) 	gymin = gy;
			if(gxmin>gz) 	gzmin = gz;
			
			//Mahony_filter.updateIMU(gx/131.2f, gy/131.2f, gz/131.2f, ax_corr/16384.0f, ay_corr/16384.0f, az_corr/16384.0f);
			//	digitalWrite(SRAM_CS, 0);
			Madgwick_filter.MadgwickAHRSupdate(gx*(1/131.2f*DEG2RAD*4.0f), gy*(1/131.2f*DEG2RAD*4.0f), gz*(1/131.2f*DEG2RAD*4.0f), ax_corr/*/16384.0f*/, ay_corr/*/16384.0f*/, az_corr/*/16384.0f*/, (float)mx_cor, (float)my_cor, (float)mz_cor);
			//	digitalWrite(SRAM_CS, 1);
			a_vertical_imu = Madgwick_filter.getVertical(ax_corr/IMU_BIT_PER_G, ay_corr/IMU_BIT_PER_G, az_corr/IMU_BIT_PER_G);
			kalmanFilter3_update(alt_baro, (a_vertical_imu*1.0f-1.0f)*980.0f, (float)1/100.0f, &alt_filter, &vario_filter);
			vario_lowpassed_600samples = (vario_lowpassed_600samples * (599)+ vario_filter)*(1.0f/600.0f);


		}
			
		
	}
	SPI_IRQ.endTransaction();
	
	#ifdef SERIALDEBUG
	if(serialDebugVector){
		
		int moredata = 0;
		int mask = 1;
		for(int i = 0; i < 8; i++){
			if((mask << i) & serialDebugVector){
				moredata++;
			}
		}
		if(moredata > 1)
		moredata = 1;
		else
		moredata = 0;
		

		
		if(serialDebugVector & 1)	SerialUSB.print(alt_baro);
		if(moredata)				SerialUSB.print(",");
		if(serialDebugVector & 2)	SerialUSB.print(a_vertical_imu);
		if(moredata)				SerialUSB.print(",");
		if(serialDebugVector & 4)	SerialUSB.print(alt_filter);
		if(moredata)				SerialUSB.print(",");
		if(serialDebugVector & 8)	SerialUSB.print(vario_filter);
		if(moredata)				SerialUSB.print(",");
		if(serialDebugVector & 16)	SerialUSB.print(gy);
		if(moredata)				SerialUSB.print(",");
		if(serialDebugVector & 32)	SerialUSB.print(az);
		if(moredata)				SerialUSB.print(",");

		SerialUSB.print("\r\n");
	}
	

	#endif
	
	updateGauge(&statVar.varioGauge, vario_filter*0.01f);
	updateGauge(&statVar.varioAvgGauge, vario_filter*0.01f);
	if (fix.speed_kph() != 0)
	updateGauge(&statVar.glideRatioGauge, vario_filter/(fix.speed_kph() * (100.0f / 3.6f)));
	else
	updateGauge(&statVar.glideRatioGauge, NAN);

	
	/*
	SerialUSB.print(flag_sec++);
	SerialUSB.print(" ");
	SerialUSB.print(pressure_read);
	SerialUSB.print(" ");
	SerialUSB.print(altChange);
	SerialUSB.print(" ");
	SerialUSB.print((int)(a_vertical_imu*1000.0-1000.0));
	SerialUSB.print(" ");
	SerialUSB.print((int)(vyska*100));
	SerialUSB.print(" ");
	SerialUSB.println((int)alt_filter);
	*/
	


	
	//generate short beep bursts, long beep or be silent
	if(    ((statVar.ena_vector & ENA_BUZZER) &&  (statVar.ena_vector & ENA_BUZZ_WHEN_LOGGING)&& (statVar.ena_vector & ENA_TRACKLOG))    ||   ((statVar.ena_vector & ENA_BUZZER) &&  (!(statVar.ena_vector & ENA_BUZZ_WHEN_LOGGING)) )){
		buzzerAltitudeDiff((int)vario_filter);
		if (buzzer_counter < buzzer_on_preiod)
		buzzerEna(1);
		else
		buzzerEna(0);
	}
	else{
		buzzerAltitudeDiff(0);
	}
	

	
	zoufalepomocnapromenna++;
	buzzer_counter++;
	if (buzzer_counter > buzzer_period)
	buzzer_counter = 0;

	buttons.buttonUpdate();
	
	
	counter_incremented_every_ISR++;

	if(counter_incremented_every_ISR%20 == 0){
		//Mag_print_angles();
		//	print_mag();
	}


	
	//clear interrupt flags
	REG_TC4_INTFLAG = TC_INTFLAG_MC1;
	REG_TC4_INTFLAG = TC_INTFLAG_OVF;
	REG_TC4_INTFLAG = TC_INTFLAG_MC0;
}

