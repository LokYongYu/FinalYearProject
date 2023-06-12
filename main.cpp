/**
@file main.cpp
@author Arnaud Maye, 4DSP   
@brief FMC15x reference application
*************************************************************************/

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <iostream>

#if defined WIN32

// Include declarations for _aligned_malloc and _aligned_free 
#include <malloc.h>
#include <windows.h>
#else
#include <unistd.h>
#ifndef __int64
#define __int64 long long
#endif
#ifndef ULONG
#define ULONG unsigned long
#endif
#define INVALID_HANDLE_VALUE -1

static void *_aligned_malloc(size_t size, size_t alignment)  
{
	void *p;
	if (posix_memalign(&p, alignment, size))
		return NULL;
	return p; 
}

static void _aligned_free(void *p)
{
	free(p);
}

static void DeleteFile(const char *filename)
{
	unlink(filename);
}

#define Sleep(x)	(usleep((unsigned long long)(5 * x * 1000)))

#endif

// project includes
#include "sipif.h"

#include "cid.h"	
#include "sxdxrouter.h"
#include "i2cmaster.h"
#include "fmc15x.h"
#include "ctgen.h"
#include "memfifo.h"


#define ROUTER_S3D1_ID		0x14			/*!< router star ID as per the firmware source code */
#define ROUTER_S1D3_ID		0x12			/*!< router star ID as per the firmware source code */
#define ROUTER_S5D1_ID		0x15			/*!< router star ID as per the firmware source code */
#define ROUTER_S1D5_ID		0x13			/*!< router star ID as per the firmware source code */
#define FMC150_ID			0x26			/*!< FMC150 star ID as per the firmware source code */
#define FMC151_ID           0xE0			/*!< FMC151 star ID as per the firmware source code */
#define FMC150_SEC_ID		0xC6			/*!< FMC150 secondary star ID as per the firmware source code */
#define FMC151_SEC_ID		0xF2			/*!< FMC151 secondary star ID as per the firmware source code */
#define I2C_MASTER_ID		0x05			/*!< I2C master star ID as per the firmware source code */
#define I2C_MASTER_OE_ID	0x42			/*!< I2C master oe star ID as per the firmware source code */
#define ZC706_STATIC_DDR3	0x116			/*!< I2C master oe star ID as per the firmware source code */


#define CT_GEN_ID			0x43			/*!< fmc_ct_gen star ID as per the firmware source code */
#define SINE_WAVE	0						/*!< GenerateWaveform() generates sine wave */
#define SAW_WAVE	1						/*!< GenerateWaveform() generates saw wave */
#define DC_WAVE		2						/*!< GenerateWaveform() generates dc wave */
#define PULSES		3						/*!< GenerateWaveform() generates  pulses */
#define SQUARE_WAVE 4

#define SYNTH_M			250					/*!< Reference value for M on the synthesizer frequency (f = M/N) */
#define SYNTH_N			2					/*!< Reference value for N on the synthesizer frequency (f = M/N) */
#define ASCII			0					/*!< Save16BitArrayToFile() saves the samples as ASCII */
#define BINARY			1					/*!< Save16BitArrayToFile() saves the samples as binary */
#define TIMEOUTDMA		2000				/*!< DMA tiemout is 2 seconds (2000 ms) */

#define FPGATYPE_160T	0					/*!< FPGA type when 160t is 0 */
#define FPGATYPE_410T	1					/*!< FPGA type when 410t is 1 */

/**
*  Generate a 16 bit waveform into a previously allocated memory buffer. This function can generate several data types and both
*  signal period as well as amplitude are configurable.
*
*  @param buffer	pointer to a buffer about to receive the waveform data. This point to a previously allocated memory as big as
*					numbersamples*2 ( byte size ) or numbersamples*1 ( sample size ).
*  @param numbersamples	number of samples to be written on the buffer where one sample is as big as 2 bytes.
*  @param period	period of the signal to generate.
*  @param amplitude	amplitude of the signal to generate.
*  @param datatype	decide what kind of data the function generates :
*						- SINE_WAVE
*						- SAW_WAVE
*						- DC_WAVE
*  @return 
*						- -1 ( Unexpected NULL argument )
*						- 0 ( Success )
*/

int32_t GenerateWaveform16(uint16_t *buffer, uint32_t numbersamples, uint32_t period, uint32_t frequency, uint32_t amplitude, uint8_t datatype)
{
	int32_t ampl				= 0;
	double pi				= 3.1415926535;
	int32_t tmp2				= 0x0;
	double x, y;

	
	
	
	// set our buffer with known value ( 0 ). Note the MUL(2) because memset takes a byte size
	memset(buffer,0, numbersamples*2);

	// make sure we are not going to hit the wall
	if(!buffer) {
		printf("GenerateWaveform() cannot receive a NULL first argument...\n");
		return -1;
	}

	// proceed with the data generation
	switch(datatype)
	{
	case SINE_WAVE:
		for (uint32_t i = 0; i < numbersamples / 2; i++)
		{
			ampl = amplitude / 2 - 1;
			// calculate Mcycles based on the frequency and numbersamples
			double freqof1cycle =245e6/numbersamples;
			//int Mcycle = (frequency/freqof1cycle);
			// 100 = 6687, 70 = 4681, 50 = 3344, 60 = 4012
			double Mcycle_50 = 6687;
			double cwfreq50 = 245e6 / (numbersamples/Mcycle_50);
			double Mcycle_70 = 4681;
			double cwfreq70 = 245e6 / (numbersamples / Mcycle_70);
			double Mcycle_3 = 4012;
			double cwfreq3 = 245e6 / (numbersamples / Mcycle_3);
			double freq_start = 50e6;
			double freq_stop = 70e6;
			double freq_step = (freq_stop - freq_start) / (numbersamples / 2);
            //frequency = freq_start + (i  * freq_step);
			frequency = cwfreq50;
			x = (2 * pi * frequency) * (i * 2 + 0) / 245e6;
			y = sin(x);
			buffer[2 * i + 0] = ((uint16_t)(y * ampl));
			x = (2 * pi * frequency) * (i * 2 + 1) / 245e6;
			y = sin(x);
			buffer[2 * i + 1] = ((uint16_t)(y * ampl));
		}
		break;
	case SAW_WAVE:
		for(uint32_t i=0; i < numbersamples/2; i++)
		{
			tmp2 =(0xffff)& ((2*i)%period*(amplitude-1)/period*2);
			buffer[2*i+0] = (uint16_t)(tmp2);
			tmp2 = (0xffff)& ((2*i)%period*(amplitude-1)/period*2);
			buffer[2*i+1] = (uint16_t)(tmp2);
		}
		break;
	case PULSES:
		for (uint32_t i = 0; i < numbersamples / 2; i++)
		{
			ampl = amplitude / 2 - 1;
			if (i % period == 0) {
				buffer[2 * i + 0] = (uint16_t)(ampl);
				buffer[2 * i + 1] = (uint16_t)(ampl);
			}
			else {
				buffer[2 * i + 0] = (uint16_t)(0);
				buffer[2 * i + 1] = (uint16_t)(0);
			}
		}
		break;

	case DC_WAVE:
	default:
		for(uint32_t i=0; i < numbersamples/2; i++)
		{
			tmp2 =(0xffff) & (0x8000);
			buffer[2*i+0] = (uint16_t)(tmp2);
			buffer[2*i+1] = (uint16_t)(tmp2);
		}
		break;
	case SQUARE_WAVE:
		for (uint32_t i = 0; i < numbersamples / 2; i++)
		{
			if (i % (period/2 ) < period / 4) {
				buffer[2 * i + 0] = (uint16_t)(amplitude / 2 - 1);
				buffer[2 * i + 1] = (uint16_t)(amplitude / 2 - 1);
			}
			else {
				buffer[2 * i + 0] = (uint16_t)(0);
				buffer[2 * i + 1] = (uint16_t)(0);
			}
		}
		break;



	}
	return 0;
}



#ifndef API_ENUM_DISPLAY
#define API_ENUM_DISPLAY 1
#endif

// Save a buffer to a file.
#ifndef Save16BitArrayToFile
/**
*  Save a 16 bit sample array to file.
*
*  @param buf	pointer to a buffer about to receive the waveform data. This point to a previously allocated memory as big as
*					bufsize*2 ( byte size ) or bufsize*1 ( sample size ).
*  @param bufsize	number of samples to be written on the buffer where one sample is as big as 2 bytes.
*  @param filename	pointer to a string representing the filename/path
*  @param mode	decide if the function writes in ASCII or binary representation:
*				- BINARY
*				- ASCII
*  @return
*						- -1, -2 ( Unexpected NULL argument )
*						- 0 ( Success )
*/
static uint32_t Save16BitArrayToFile(void *buf, int32_t bufsize, const char *filename, int32_t mode)
{
	int32_t i;
	FILE *fOutFile;
	char sOpenMode[55];

	// these pointers cannot be NULL
	if(!buf) {
		printf("Save16BitArrayToFile() -> first argument cannot be NULL\n");
		return -1;
	}

	if(!filename) {
		printf("Save16BitArrayToFile() -> third argument cannot be NULL\n");
		return -2;
	}

	// cast our stamp less pointer to a int16_t
	int16_t *buf16 = (int16_t *)buf;

	// open the file given as argument
	if(mode==ASCII)
		sprintf(sOpenMode, "a");
	else
		sprintf(sOpenMode, "ab");

	fOutFile = fopen(filename, sOpenMode);
	if(fOutFile==NULL) {
		printf("Save16BitArrayToFile() -> Cannot open file '%s' with write access\n", filename);
		return -3;
	}

	// write to file either as ASCII or BINARY
	if(mode == BINARY)
		fwrite(buf, 2, bufsize, fOutFile);
	else // -> ASCII
	{
		// Here we don't take risk. We pass a int16_t casted as an int32_t,
		// and we use the normalized int32_t -> int16_t format converter (%hi)
		for(i = 0; i < bufsize; i++)
			fprintf(fOutFile, "%hi\n", (int32_t)buf16[i]);
	}

	fclose(fOutFile);
	return 0;
}

#endif

/**
*  Verifies Ramp Pattern (Card Specific)
*
*  @param pInData  8-bit wide stream of buffer from the ADC
*  @param burstSize  BurstSize, or number of samples in the buffer in 16 bit width (Fmc15x)
*
*  @return 
*						- -1 ( pattern check fails )
*						- 0 ( Success )
*/
int32_t verify_ramp_pattern(char *pInData, int32_t burstSize)
{
	// verify the patterned data
	// pattern data on FMC15x bit is 14 resolution, 16 bit aligned.

	uint16_t *pattern_data = (uint16_t *)pInData;

	uint16_t check_data = (pattern_data[0] >> 2) + 1;

	uint32_t error_count = 0;
	uint16_t mask = 0x3fff;

	// mask off the overrange bit in the data received from the ADC using the appropriate mask
	check_data = check_data & mask;
	for (int32_t j = 1; j < burstSize; j++) {
		if (check_data != (pattern_data[j] >> 2))
		{
			error_count++;
		}
		check_data++;
		check_data &= mask;
	}

	if (error_count) {
		printf ("Pattern Check Failed with %d pattern errors\n", error_count);
		return -1;
	}
	else {
		printf ("Pattern Check Passed!\n");
		return 0;
	}
}


/**
*  \brief FMC15x Reference application (main).
*
*  This function demonstrates how to configure both digital to analog peripherals, upload waveforms to digital
*  to analog convert chips, display FMC15x diagnostic, display FMC15x clock tree and grab data from the ADC 0
*  and ADC 1.
*
*  Description of the software sequence :
*	- Check and convert arguments passed to the application.
*	- Open a FMC15x over ethernet or PCI using sipif_init().
*	- Read the constellation information from the ML605+FMC15x using cid_init().
*	- Compute start offset of all FMC15x peripheral in the main constallation address space using cid_getstaroffset().
*	- Configure the data routers with some defautl settings using sxdx_configurerouter().
*	- Display FMC15x diagnostics using fmc15x_getdiagnostics().
*	- Init all the FMC15x peripherals using fmc15x_init().
*	- Display all the freqencies part of the frequency tree using fmc15x_freqcnt_getfrequency().
*	- Configure burst size and burst number ( common for both ADC and DAC chips ) using fmc15x_ctrl_configure_burst().
*	- Generate a waveform and upload waveform to DAC0 using GenerateWaveform16(), sxdx_configurerouter(), fmc15x_ctrl_prepare_wfm_load() and WriteBlock() part of ethapi.
*	- Generate a waveform and upload waveform to DAC1 using GenerateWaveform16(), sxdx_configurerouter(), fmc15x_ctrl_prepare_wfm_load() and WriteBlock() part of ethapi.
*	- Grab a burst from ADC0 using 	sxdx_configurerouter(), fmc15x_ctrl_enable_channel(), fmc15x_ctrl_arm_dac(), fmc15x_ctrl_sw_trigger() and Save16BitArrayToFile().
*	- Grab a burst from ADC1 using 	sxdx_configurerouter(), fmc15x_ctrl_enable_channel(), fmc15x_ctrl_arm_dac(), fmc15x_ctrl_sw_trigger() and Save16BitArrayToFile().
*
*  @param argc the command line
*  @param argv the number of options in the command line.
*  @return 0 ( success ) or any other error code.
*/
int32_t main(int32_t argc, char* argv[])
{

	int32_t devIdx;
	int32_t modeClock;
	int32_t ifType;
	const char *devType;
	char fpga_device_type[32];
	uint8_t tapiod_clk;
	uint8_t tapiod_data;
	uint16_t constellation_id;
	int32_t numFmcCards;
	uint32_t odelay_tap;
	uint8_t fpgatype;
	int32_t auto_training;

	// Parse the application arguments
	if(argc!=6) {
		printf("Usage: FMCxxxApp.exe {interface type} {device type} {device index} {clock mode} {auto training}\n\n");
		printf(" {interface type} can be either 0 (PCI) or 1 (Ethernet) or 2 (TCPIP)\n");
		printf(" {device type} is a string defining the target hardware (VP680, ML605, ...)\n");
		printf(" {device type} is an ip address when using TCPIP interface\n");
		printf(" {device index} is a PCI index or an Ethernet interface index or a TCPIP port when using TCPIP interface\n");		
		printf(" {clock mode} can be either:\n");
		printf("   0 Internal Clock with Internal Reference\n");
		printf("   1 External Clock\n");
		printf("   2 Internal Clock with External Reference\n");
		printf(" {auto training} can be either:\n");
		printf("    0 Auto training disabled\n");
		printf("    1 Auto training enabled\n");
		printf("\n");
		printf("\n");
		printf(" List of NDIS interfaces found in the system {device index}:\n");
		printf(" -----------------------------------------------------------\n");
		if(sipif_getdeviceenumeration(API_ENUM_DISPLAY)!=SIPIF_ERR_OK) {
			printf("Could not obtain NDIS(Ethernet) device enumeration...\n Check if the 4dspnet driver installed or if the service started?\n");
			printf("You can discard this error if you do not have any Ethernet based product in use.");
		}
		sipif_free();
		return -1;
	} else {
		// Convert arguments
		ifType = atoi(argv[1]);
		devType = (const char *)argv[2];
		devIdx = atoi(argv[3]);
		modeClock = atoi(argv[4]);
		auto_training = atoi(argv[5]);

		// translate interface type to the sipif values
		if(ifType==0)
			ifType = SIPIF_4FM;
		else if(ifType==1)
			ifType = SIPIF_ETHAPI;	
		else
			ifType = SIPIF_TCPIP_V4;	
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Open one of the device from a given device ID argument	
	if(sipif_init(ifType, devType, devIdx, TIMEOUTDMA, SYNTH_M, SYNTH_N, fpga_device_type) != SIPIF_ERR_OK) {
		printf("Could not open device %d\n", devIdx);
		sipif_free();
		return -2;
	}

	printf ("\n\nConnected FPGA Device Type: %s\n\n", fpga_device_type);


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Figure out if the device is a 160t or 410t	
	if(!strcmp(fpga_device_type, "XC7K160T-2FFG676C")) {
		fpgatype = FPGATYPE_160T;
	} else
		fpgatype = FPGATYPE_410T;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Get the reference frequency (always 125MHz except 200MHz for VP680 GEN2 PCIe)
	float fReference;
	if(	sipif_getsipcmdfreq(&fReference) != SIPIF_ERR_OK) {
		printf("Could not get reference frequency %d\n", devIdx);
		sipif_free();
		return -2;
	}

	printf("Start of program (Ref Freq. = %1.1fMHz)\n", fReference);
	printf("--------------------------------------\n");


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Obtain and display the sip_cid informations to the console. This function also check that the constellation ID
	// obtained by the firmware match the value passed as argument
	int32_t rc  = cid_init(0);
	if(rc<1) {
		printf("Could obtain sipcid table (error %x), exiting\n", rc);
		sipif_free();
		return -3;
	}

	constellation_id = cid_getconstellationid();
	printf("Constellation ID : %d\n", constellation_id);
	printf("Number of Stars  : %d\n", cid_getnbrstar());
	printf("Software Build   : 0x%8.8X\n", cid_getswbuildcode());
	printf("Firmware Build   : 0x%8.8X\n", cid_getfwbuildcode());
	printf("Firmware Version : %d.%d\n", cid_getfirmwareversion()>>16, cid_getfirmwareversion()&0xFFFF);
	printf("--------------------------------------\n");
	printf("\n");

	// Default number of cards is 1 per board - exception is for PC720 which could have two
	numFmcCards = 1;
	odelay_tap = 0;
	switch (constellation_id)
	{
	case CONSTELLATION_ID_ML605:
	case CONSTELLATION_ID_FMC151_ML605:
		printf("Found ML605 hardware\n\n");
		tapiod_clk = 0x00; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_KC705:
		printf("Found KC705 hardware\n\n");
		tapiod_clk = 0x00; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_FMC151_KC705:
		printf("Found KC705 hardware\n\n");
		tapiod_clk = 0x00; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_VC707:
		printf("Found VC707 hardware\n\n");
		tapiod_clk = 0x00; tapiod_data = 28;
		break;

	case CONSTELLATION_ID_FMC151_VC707_HPC1:
		printf("Found VC707 hardware\n\n");
		tapiod_clk = 0x07; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_FMC151_VC707_HPC2:
		printf("Found VC707 hardware\n\n");
		tapiod_clk = 10; tapiod_data = 0;
		break;

	case CONSTELLATION_ID_SP601:
		printf("Found SP601 hardware\n\n");
		tapiod_clk = 0x00; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_SP605:
		printf("Found SP605 hardware\n\n");
		tapiod_clk = 0x00; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_FM680:
		printf("Found FM680 hardware\n\n");
		tapiod_clk = 0x00; tapiod_data = 20;
		break;

	case CONSTELLATION_ID_VP680:
		printf("Found VP680 hardware\n\n");
		tapiod_clk = 0x00; tapiod_data = 20;
		break;

	case CONSTELLATION_ID_ZC702:
		printf("Found ZC702 hardware\n\n");
		tapiod_clk = 10; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_ZC706:
		printf("Found ZC706 hardware\n\n");
		tapiod_clk = 17; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_FMC151_ZC706:
		printf("Found ZC706 hardware\n\n");
		tapiod_clk = 17; tapiod_data = 0;
		break;

	case CONSTELLATION_ID_FMC151_ZC706_DDR3:
		printf("Found ZC706 hardware\n\n");
		tapiod_clk = 5; tapiod_data = 0;
		break;

	case CONSTELLATION_ID_ZEDB:
		printf("Found Zedboard hardware\n\n");
		tapiod_clk = 10; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_ML605_PCIe:
		printf("Found ML605 hardware with PCIe\n\n");
		tapiod_clk = 0x00; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_PC720_PRIMARY:
		printf("Found PC720 hardware with PCIe on Primary FMC\n\n");
		tapiod_clk = 15; tapiod_data = 0x00;	// optimized for 160t device. For 325t, tap values of clk=0 and data=10 may be required. 
		break;
	case CONSTELLATION_ID_FMC151_PC720_PRIMARY:
		printf("Found PC720 hardware with PCIe on Primary FMC\n\n");
		if(fpgatype == FPGATYPE_160T) {       
			tapiod_clk = 16; tapiod_data = 0x00;
		} else {
			tapiod_clk = 16; tapiod_data = 0x00;
		}	 
		break;

	case CONSTELLATION_ID_PC720_SECONDARY:
		printf("Found PC720 hardware with PCIe on Secondary FMC\n\n");
		tapiod_clk = 6; tapiod_data = 0x00;
		odelay_tap = 2;
		break;

	case CONSTELLATION_ID_sFMC720:
		printf("Found sFMC720 hardware\n\n");
		tapiod_clk = 0; tapiod_data = 6; // optimized for 325t device. data OK with tapiod_data = 0 to 0xD
		break;

	case CONSTELLATION_ID_FMC151_PC720_SECONDARY:
		printf("Found PC720 hardware with PCIe on Secondary FMC\n\n");
		if(fpgatype == FPGATYPE_160T) {
			tapiod_clk = 10; tapiod_data = 0x00;
			odelay_tap = 2;
		} else {
			tapiod_clk = 0; tapiod_data = 0x06;
			odelay_tap = 2;
		}
		break;

	case CONSTELLATION_ID_PC720_BOTH:
		printf("Found PC720 hardware with PCIe on Two FMCs\n\n");
		tapiod_clk = 12; tapiod_data = 0x00;		// Tap values for primary FMC150
		odelay_tap = 5;							// Only secondary FMC150 has odelay implementations.
		numFmcCards = 2;
		break;

	case CONSTELLATION_ID_FMC151_PC720_BOTH:
		printf("Found PC720 hardware with PCIe on Two FMCs\n\n");					
		if(fpgatype == FPGATYPE_160T) {
			tapiod_clk = 16; tapiod_data = 0x00;	// Tap values for primary FMC151
			odelay_tap = 2; // Only secondary FMC151 has odelay implementations.
		} else {
			tapiod_clk = 4; tapiod_data = 0x00; // Tap values for primary FMC151
			odelay_tap = 2; // Only secondary FMC151 has odelay implementations.
		}
		numFmcCards = 2;
		break;

	case CONSTELLATION_ID_KC705_PCIe:
		printf("Found KC705 hardware with PCIe interface\n\n");
		tapiod_clk = 0x00; tapiod_data = 0x00;
		break;

	case CONSTELLATION_ID_FC6301:
		printf("Found FC6301 hardware\n\n");
		tapiod_clk = 10; tapiod_data = 0x00;
		break;
	case CONSTELLATION_ID_FM780:
		printf("Found FM780 hardware\n\n");
		tapiod_clk = 10; tapiod_data = 0x00;
		odelay_tap = 0;
		break;

	default:
		printf("Constellation ID not supported by this software, exiting...\n");
		sipif_free();
		return -3;
		break;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Read Star Offsets and compute sub mapping for stars
	uint32_t size = 1;
	uint32_t AddrSipRouterS1D3, AddrSipRouterS3D1, AddrSipFMC150, AddrSipFMC150SEC, AddrSipI2cMaster, AddrSipMemoryFIFO;

	if (constellation_id != CONSTELLATION_ID_PC720_BOTH && constellation_id != CONSTELLATION_ID_FMC151_PC720_BOTH) {
		if(cid_getstaroffset(ROUTER_S1D3_ID, &AddrSipRouterS1D3, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", ROUTER_S1D3_ID);
			sipif_free();
			return -4;
		}
		if(cid_getstaroffset(ROUTER_S3D1_ID, &AddrSipRouterS3D1, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", ROUTER_S3D1_ID);
			sipif_free();
			return -5;
		}
	}
	else {
		if(cid_getstaroffset(ROUTER_S1D5_ID, &AddrSipRouterS1D3, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", ROUTER_S1D5_ID);
			sipif_free();
			return -4;
		}
		if(cid_getstaroffset(ROUTER_S5D1_ID, &AddrSipRouterS3D1, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", ROUTER_S5D1_ID);
			sipif_free();
			return -5;
		}
	}

	if( constellation_id == CONSTELLATION_ID_PC720_SECONDARY) {
		if(cid_getstaroffset(FMC150_SEC_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC150_SEC_ID);
			sipif_free();
			return -6;
		}
	} else if( constellation_id == CONSTELLATION_ID_FMC151_PC720_SECONDARY) {
		if(cid_getstaroffset(FMC151_SEC_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_SEC_ID);
			sipif_free();
			return -6;
		}
	} else if ( constellation_id == CONSTELLATION_ID_PC720_BOTH) {
		if(cid_getstaroffset(FMC150_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC150_ID);
			sipif_free();
			return -6;
		}
		if(cid_getstaroffset(FMC150_SEC_ID, &AddrSipFMC150SEC, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC150_SEC_ID);
			sipif_free();
			return -6;
		}
	} else if ( constellation_id == CONSTELLATION_ID_FMC151_PC720_BOTH) {
		if(cid_getstaroffset(FMC151_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_ID);
			sipif_free();
			return -6;
		}
		if(cid_getstaroffset(FMC151_SEC_ID, &AddrSipFMC150SEC, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_SEC_ID);
			sipif_free();
			return -6;
		}
	} else if (constellation_id == CONSTELLATION_ID_FMC151_ML605) {
		if(cid_getstaroffset(FMC151_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_ID);
			sipif_free();
			return -6;
		}
	}
	else if (constellation_id == CONSTELLATION_ID_FMC151_VC707_HPC1) {
		if(cid_getstaroffset(FMC151_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_ID);
			sipif_free();
			return -6;
		}
	}
	else if ((constellation_id == CONSTELLATION_ID_FMC151_VC707_HPC2)||(constellation_id == CONSTELLATION_ID_FMC151_PC720_PRIMARY)) {
		if(cid_getstaroffset(FMC151_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_ID);
			sipif_free();
			return -6;
		}
	}
	else if ((constellation_id == CONSTELLATION_ID_FMC151_KC705)||(constellation_id == CONSTELLATION_ID_FMC151_VC707_HPC2)) {
		if(cid_getstaroffset(FMC151_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_ID);
			sipif_free();
			return -6;
		}
	}
	else if (constellation_id == CONSTELLATION_ID_FMC151_ZC706) {
		if(cid_getstaroffset(FMC151_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_ID);
			sipif_free();
			return -6;
		}
	}
	else if (constellation_id == CONSTELLATION_ID_FMC151_ZC706_DDR3) {
		if(cid_getstaroffset(FMC151_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC151_ID);
			sipif_free();
			return -6;
		}
		if(cid_getstaroffset(ZC706_STATIC_DDR3, &AddrSipMemoryFIFO, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", ZC706_STATIC_DDR3);
			sipif_free();
			return -7;
		} 
	}
	else {
		if(cid_getstaroffset(FMC150_ID, &AddrSipFMC150, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", FMC150_ID);
			sipif_free();
			return -6;
		}
	}

	if( constellation_id == CONSTELLATION_ID_PC720_PRIMARY  || constellation_id == CONSTELLATION_ID_PC720_SECONDARY ||
		constellation_id == CONSTELLATION_ID_PC720_BOTH || constellation_id == CONSTELLATION_ID_FC6301 ||
		constellation_id == CONSTELLATION_ID_FMC151_PC720_SECONDARY || constellation_id == CONSTELLATION_ID_FMC151_PC720_BOTH ||
		constellation_id == CONSTELLATION_ID_FMC151_PC720_PRIMARY || constellation_id == CONSTELLATION_ID_sFMC720) {
			if(cid_getstaroffset(I2C_MASTER_OE_ID, &AddrSipI2cMaster, &size)!=SIP_CID_ERR_OK) {
				printf("Could not obtain address for star type %d, exiting\n", I2C_MASTER_OE_ID);
				sipif_free();
				return -7;
			}
	}
	else {
		if(cid_getstaroffset(I2C_MASTER_ID, &AddrSipI2cMaster, &size)!=SIP_CID_ERR_OK) {
			printf("Could not obtain address for star type %d, exiting\n", I2C_MASTER_ID);
			sipif_free();
			return -7;
		}
	}

	// if we are dealing with a ML605, KC705 or a VC707 constellation
	if((constellation_id == CONSTELLATION_ID_ML605) || (constellation_id == CONSTELLATION_ID_FMC151_KC705) ||(constellation_id == CONSTELLATION_ID_KC705 ) || 
		(constellation_id == CONSTELLATION_ID_KC705_PCIe )) {
			uint32_t AddrCtGen;
			// Search for fmc_ct_gen star in the constellation

			if(cid_getstaroffset(CT_GEN_ID, &AddrCtGen, &size)!=SIP_CID_ERR_OK) {
				printf("Could not obtain address for star type %d, exiting\n", I2C_MASTER_ID);
				sipif_free();
				return -8;
			}
			// If we are in external clock mode, we configure fmc_ct_gen star with the correct output frequency
			// otherwise we disable the output
			if(modeClock==1) {
				if(ctgen_configure(AddrCtGen, OUT_2GBPS_500MHZ)!=CTGEN_ERR_OK) {
					printf("Could not configure the clock/trigger generator star, exiting\n");
					sipif_free();
					return -9;
				}
			} else {
				if(ctgen_configure(AddrCtGen, OUT_XGBPS_DISABLED)!=CTGEN_ERR_OK) {
					printf("Could not configure the clock/trigger generator star, exiting\n");
					sipif_free();
					return -9;
				}
			}
	}

	for (int32_t currentCard = 0; currentCard < numFmcCards; currentCard++) {
		uint32_t AddrSipFMC150Ctrl;
		uint32_t AddrSipFMC150AdcPhy;
		uint32_t AddrSipFMC150DacPhy;
		uint32_t AddrSipFMC150AdcSpi;
		uint32_t AddrSipFMC150DacSpi;
		uint32_t AddrSipFMC150ClkSpi;
		uint32_t AddrSipFMC150FreqCnt;
		uint32_t AddrSipFMC150Monitor;

		if (constellation_id != CONSTELLATION_ID_PC720_BOTH && constellation_id != CONSTELLATION_ID_FMC151_PC720_BOTH) {
			// Calculate BAR of every peripheral mapped (sub mapping) to the FMC150 star's memory. This uses fixed offsets given by the FMC150 
			// For all constellation except one with two FMC cards
			AddrSipFMC150Ctrl    = AddrSipFMC150 + 0x000;
			AddrSipFMC150AdcPhy  = AddrSipFMC150 + 0x010;
			AddrSipFMC150DacPhy  = AddrSipFMC150 + 0x020;
			AddrSipFMC150AdcSpi  = AddrSipFMC150 + 0x100;
			AddrSipFMC150DacSpi  = AddrSipFMC150 + 0x300;
			AddrSipFMC150ClkSpi  = AddrSipFMC150 + 0x400;
			AddrSipFMC150FreqCnt = AddrSipFMC150 + 0x600;
			AddrSipFMC150Monitor = AddrSipFMC150 + 0x700;
		}
		else {

			if (currentCard == 0) {
				// first Card on the PC720
				AddrSipFMC150Ctrl    = AddrSipFMC150 + 0x000;
				AddrSipFMC150AdcPhy  = AddrSipFMC150 + 0x010;
				AddrSipFMC150DacPhy  = AddrSipFMC150 + 0x020;
				AddrSipFMC150AdcSpi  = AddrSipFMC150 + 0x100;
				AddrSipFMC150DacSpi  = AddrSipFMC150 + 0x300;
				AddrSipFMC150ClkSpi  = AddrSipFMC150 + 0x400;
				AddrSipFMC150FreqCnt = AddrSipFMC150 + 0x600;
				AddrSipFMC150Monitor = AddrSipFMC150 + 0x700;

				sipif_writesipreg(AddrSipI2cMaster+0x7000, 0x01);	// I2C switch set to primary FMC
				Sleep(10);
			}
			else {
				// second Card on the PC720
				AddrSipFMC150Ctrl    = AddrSipFMC150SEC + 0x000;
				AddrSipFMC150AdcPhy  = AddrSipFMC150SEC+ 0x010;
				AddrSipFMC150DacPhy  = AddrSipFMC150SEC + 0x020;
				AddrSipFMC150AdcSpi  = AddrSipFMC150SEC + 0x100;
				AddrSipFMC150DacSpi  = AddrSipFMC150SEC + 0x300;
				AddrSipFMC150ClkSpi  = AddrSipFMC150SEC + 0x400;
				AddrSipFMC150FreqCnt = AddrSipFMC150SEC + 0x600;
				AddrSipFMC150Monitor = AddrSipFMC150SEC + 0x700;

				tapiod_clk = 0;	tapiod_data = 0;					// Tap values for secondary FMC150
				sipif_writesipreg(AddrSipI2cMaster+0x7000, 0x02);	// I2C switch set to secondary FMC
				Sleep(10);
			}
		}

		// Configure I2C switch
		if( constellation_id == CONSTELLATION_ID_KC705) {
			sipif_writesipreg(AddrSipI2cMaster+0x7400, 0x02); // Switch set to LPC		
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_KC705_PCIe) {
			sipif_writesipreg(AddrSipI2cMaster+0x7400, 0x02); // Switch set to LPC		
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_VC707) {
			sipif_writesipreg(AddrSipI2cMaster+0x7400, 0x01); // Switch set to HPC_1	
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_FMC151_VC707_HPC1) {
			sipif_writesipreg(AddrSipI2cMaster+0x7400, 0x02); // Switch set to HPC_1	
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_FMC151_VC707_HPC2) {
			sipif_writesipreg(AddrSipI2cMaster+0x7400, 0x04); // Switch set to HPC_2	
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_PC720_PRIMARY) {
			sipif_writesipreg(AddrSipI2cMaster+0x7000, 0x01);	// I2C switch set to primary FMC
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_PC720_SECONDARY) {
			sipif_writesipreg(AddrSipI2cMaster+0x7000, 0x02);	// I2C switch set to secondary FMC
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_FMC151_PC720_PRIMARY) {
			sipif_writesipreg(AddrSipI2cMaster+0x7000, 0x01);	// I2C switch set to primary FMC
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_FMC151_PC720_SECONDARY) {
			sipif_writesipreg(AddrSipI2cMaster+0x7000, 0x02);	// I2C switch set to secondary FMC
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_FMC151_KC705) {
			sipif_writesipreg(AddrSipI2cMaster+0x7400, 0x04); // Switch set to LPC		
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_FMC151_ZC706) {
			sipif_writesipreg(AddrSipI2cMaster+0x7400, 0x40); // Switch set to LPC		
			Sleep(10);	
		}
		if( constellation_id == CONSTELLATION_ID_FMC151_ZC706_DDR3) {
			sipif_writesipreg(AddrSipI2cMaster+0x7400, 0x40); // Switch set to LPC		
			Sleep(10);	
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Configure the routers
		// Setup 1-to-3 Router
#ifdef WIN32
		if(sxdx_configurerouter(AddrSipRouterS1D3, 0xFFFFFFFFFFFFFF00)!=SXDXROUTER_ERR_OK) {
			printf("Could not configure S1D3 router, exiting\n");
			sipif_free();
			return -8;
		}

		// Setup 3-to-1 Router
		if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF00)!=SXDXROUTER_ERR_OK) {
			printf("Could not configure S3D1 router, exiting\n");
			sipif_free();
			return -9;
		}
#else
		if(sxdx_configurerouter(AddrSipRouterS1D3, 0xFFFFFFFFFFFFFF00LLU)!=SXDXROUTER_ERR_OK) {
			printf("Could not configure S1D3 router, exiting\n");
			sipif_free();
			return -8;
		}

		// Setup 3-to-1 Router
		if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF00LLU)!=SXDXROUTER_ERR_OK) {
			printf("Could not configure S3D1 router, exiting\n");
			sipif_free();
			return -9;
		}
#endif
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Detect FMC Presence
		if(fmc15x_ctrl_probefmc(AddrSipFMC150Ctrl)!=FMC15x_ERR_OK) {
			printf("Could not detect FMC150 hardware, exiting\n");
			sipif_free();
			return -11;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Temperature/Voltages monitoring
		printf("---  Measuring on-board voltages   ---\n");
		if(fmc15x_monitor_getdiags(AddrSipFMC150Monitor)!=FMC15x_MON_ERR_OK) {
			printf("An error occurred in the FMC150 diagnostics function.\n");
			printf("An error occurred in the FMC150 diagnostics function, exiting\n");
			sipif_free();
			return -10;
		}
		printf("--------------------------------------\n\n");

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Determine the VCXO type
		// The firmware default power up divider on the DAC reference clock (frequency 6) is two;
		// - When a 737.28MHz VCXO is assembled the frequency readout will be around 368.64MHz
		// - When a 491.52MHz VCXO is assembled the frequency readout will be around 245.76MHz
		// - When a 542.40MHz VCXO is assembled the frequency readout will be around 271.20MHz
		// - When a 800.00MHz VCXO is assembled the frequency readout will be around 400.00MHz
		// - When a 480.00MHz VCXO is assembled the frequency readout will be around 240.00MHz
		float freq ;
		
		
		int32_t vcxoType = FMC150_VCXO_737_28;
		if(fmc15x_freqcnt_getfrequency(AddrSipFMC150FreqCnt, 6, &freq, FMC15x_FREQCNT_NO_DISPLAY_CONSOLE, 0, fReference)!=FMC15x_FREQCNT_ERR_OK) {
			printf("Could not obtain frequency id%d from FMC15x.FREQCNT\n", 6);
			sipif_free();
			return -12;
		}

		// this check MUST happen before internal/external clock check
		if ( freq > 395 && freq < 405 )
			vcxoType = FMC150_VCXO_800_00;

		if (freq < 300 || modeClock == 1)
			vcxoType = FMC150_VCXO_542_40;

		if (freq < 260 || modeClock == 1)
			vcxoType = FMC150_VCXO_491_52;

		if (freq < 245 || modeClock == 1)
			vcxoType = FMC150_VCXO_480_00 ;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Init FMC150
		if(fmc15x_init(AddrSipFMC150ClkSpi, AddrSipFMC150DacSpi, AddrSipFMC150DacPhy, AddrSipFMC150AdcSpi, AddrSipFMC150AdcPhy,
			AddrSipFMC150Monitor, modeClock, vcxoType, tapiod_clk, tapiod_data, odelay_tap, constellation_id, auto_training)!=FMC15x_ERR_OK) {
				printf("Could not initialize FMC150\n");
				sipif_free();
				return -11;
		}
		printf("\n");

		/////////////////////////////////////////////////////////////////////////////////////////////
		// Measure and display all available frequencies in a loop.
		// Note that the first frequencies (ADC clocks) are going to display erroneous values if no
		// FMC is actually attached.
		printf("--------------------------------------\n\n");
		printf("\n--- Measuring on-board frequencies ---\n");
		for(int32_t i = 0; i < 7; i++) {
			if(fmc15x_freqcnt_getfrequency(AddrSipFMC150FreqCnt, i, NULL, FMC15x_FREQCNT_DISPLAY_CONSOLE, vcxoType, fReference)!=FMC15x_FREQCNT_ERR_OK) {
				printf("Could not obtain frequency id%d from FMC15x.FREQCNT\n", i);
				sipif_free();
				return -12;
			}
		}
		printf("--------------------------------------\n\n");

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Configure burst size and burst number
		int32_t BurstSize = 1024; // samples
		switch( cid_getconstellationid() )
		{
		case CONSTELLATION_ID_FMC151_ML605:
		case CONSTELLATION_ID_ML605:
		case CONSTELLATION_ID_ML605_PCIe:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_KC705_PCIe:
		case CONSTELLATION_ID_KC705:
		case CONSTELLATION_ID_FMC151_KC705:
			BurstSize = 16 * 1024;
			break;
		case CONSTELLATION_ID_FMC151_VC707_HPC1:
		case CONSTELLATION_ID_FMC151_VC707_HPC2:
		case CONSTELLATION_ID_VC707:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_FMC151_ZC706:
		case CONSTELLATION_ID_FMC151_ZC706_DDR3:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_SP601:
			BurstSize = 1024;
			break;
		case CONSTELLATION_ID_SP605:
			BurstSize = 1024;
			break;
		case CONSTELLATION_ID_FM680:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_VP680:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_PC720_PRIMARY:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_PC720_SECONDARY:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_sFMC720:
			BurstSize = 16*1024;
			break;	
		case CONSTELLATION_ID_FMC151_PC720_PRIMARY:
			BurstSize = 4*1024;
			break;
		case CONSTELLATION_ID_FMC151_PC720_SECONDARY:
			BurstSize = 4*1024;
			break;
		case CONSTELLATION_ID_FM780:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_PC720_BOTH:
			BurstSize = 16*1024;
			break;
		case CONSTELLATION_ID_FMC151_PC720_BOTH:
			BurstSize = 4*1024;
			break;
		default:
			BurstSize = 1024;
			break;
		}
		const int32_t DacNbPeriod0	= BurstSize/16;	// number of DAC periods per burst
		const int32_t DacNbPeriod1	= BurstSize/16;	// number of DAC periods per burst
		uint8_t *pOutData = (uint8_t *)_aligned_malloc(2*BurstSize, 4096);	// out buffer
		uint8_t *pInData	= (uint8_t *)_aligned_malloc(2*BurstSize, 4096);		// in buffer
		if(fmc15x_ctrl_configure_burst(AddrSipFMC150Ctrl, 1, BurstSize)!=FMC15x_CTRL_ERR_OK) {
			printf("Could not configure burst size/length in FMC15x.CTRL\n ");
			sipif_free();
			_aligned_free(pOutData);
			_aligned_free(pInData);
			return -13;
		}

		// For FMC151, configure DC offset to mid point
		if ( (constellation_id == CONSTELLATION_ID_FMC151_ML605) || (constellation_id == CONSTELLATION_ID_FMC151_VC707_HPC1) 
			|| (constellation_id == CONSTELLATION_ID_FMC151_VC707_HPC2) || (constellation_id == CONSTELLATION_ID_FMC151_PC720_PRIMARY) 
			|| (constellation_id == CONSTELLATION_ID_FMC151_PC720_SECONDARY) || (constellation_id == CONSTELLATION_ID_FMC151_PC720_BOTH) 
			|| (constellation_id == CONSTELLATION_ID_FMC151_KC705) || (constellation_id == CONSTELLATION_ID_FMC151_ZC706)
			|| (constellation_id == CONSTELLATION_ID_FMC151_ZC706_DDR3))
		{
			// 0x7fff is the mid point.   For larger values, it is between 0x0 and 0x7ffe, for lower values
			// it is between 0x8000 and 0xffff
			int slaveaddress = 0x1000;
			if(constellation_id == CONSTELLATION_ID_FMC151_ML605) {
				slaveaddress = 0x2200;
			}
			if (fmc151_configure_dc_offset(AddrSipI2cMaster, 0x7fff, 0x7fff, 0x7fff, 0x7fff, slaveaddress) != 0) {
				printf ("Could not configure DC offset.\n");
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				return -13;
			}

		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Load DAC0
		if(GenerateWaveform16((uint16_t *)pOutData, BurstSize, (BurstSize / DacNbPeriod0), (100e6),(uint32_t)pow(2.0f,15.8f), SINE_WAVE)!=0) {
			printf("Could not generate waveform\n");
		}

		if ((constellation_id != CONSTELLATION_ID_PC720_BOTH) && (constellation_id != CONSTELLATION_ID_FMC151_PC720_BOTH)) {
			DeleteFile("dac0.bin"); DeleteFile("dac0.txt");
			Save16BitArrayToFile(pOutData, BurstSize, "dac0.txt", ASCII);
			Save16BitArrayToFile(pOutData, BurstSize, "dac0.bin", BINARY);
		}
		else {
			if (currentCard == 0) {
				DeleteFile("dac0_primary.bin"); DeleteFile("dac0_primary.txt");
				Save16BitArrayToFile(pOutData, BurstSize, "dac0_primary.txt", ASCII);
				Save16BitArrayToFile(pOutData, BurstSize, "dac0_primary.bin", BINARY);
			}
			else
			{
				DeleteFile("dac0_secondary.bin"); DeleteFile("dac0_secondary.txt");
				Save16BitArrayToFile(pOutData, BurstSize, "dac0_secondary.txt", ASCII);
				Save16BitArrayToFile(pOutData, BurstSize, "dac0_secondary.bin", BINARY);
			}
		}
		// configure the router ( route data to DAC0's wave form memory )
		uint64_t routerSetting;
		routerSetting = 0xff;
		routerSetting = routerSetting << (currentCard * 16);
		routerSetting = ~routerSetting;

		if(sxdx_configurerouter(AddrSipRouterS1D3, routerSetting)!=SXDXROUTER_ERR_OK) {
			printf("Could not configure S1D3 router, exiting\n");
			sipif_free();
			_aligned_free(pOutData);
			_aligned_free(pInData);
			return -14;
		}
		// prepare the firmware to receive waveform data
		if(fmc15x_ctrl_prepare_wfm_load(AddrSipFMC150Ctrl, DAC0)!=FMC15x_CTRL_ERR_OK) {
			printf("Could not prepare waveform upload, exiting\n");
			sipif_free();
			_aligned_free(pOutData);
			_aligned_free(pInData);
			return -15;
		}

		// send the data to the waveform memory
		if(sipif_writedata(pOutData,  2*BurstSize)!=SIPIF_ERR_OK) {
			printf("Could not communicate with device %d.\n", devIdx);
			sipif_free();
			_aligned_free(pOutData);
			_aligned_free(pInData);
			return -16;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Load DAC1
		if(GenerateWaveform16((uint16_t *)pOutData, BurstSize, 16,(100e6), (uint32_t)pow(2.0f,15.8f), SQUARE_WAVE)!=0) {
			printf("Could not generate waveform\n");
		}

		if ((constellation_id != CONSTELLATION_ID_PC720_BOTH) && (constellation_id != CONSTELLATION_ID_FMC151_PC720_BOTH)) {
			// clean the files previously saved
			DeleteFile("dac1.bin");
			DeleteFile("dac1.txt");
			// write to file
			Save16BitArrayToFile(pOutData, BurstSize, "dac1.txt", ASCII);
			Save16BitArrayToFile(pOutData, BurstSize, "dac1.bin", BINARY);
		}
		else {
			if (currentCard == 0) {
				// clean the files previously saved
				DeleteFile("dac1_primary.bin");
				DeleteFile("dac1_primary.txt");
				// write to file
				Save16BitArrayToFile(pOutData, BurstSize, "dac1_primary.txt", ASCII);
				Save16BitArrayToFile(pOutData, BurstSize, "dac1_primary.bin", BINARY);
			}
			else {
				// clean the files previously saved
				DeleteFile("dac1_secondary.bin");
				DeleteFile("dac1_secondary.txt");
				// write to file
				Save16BitArrayToFile(pOutData, BurstSize, "dac1_secondary.txt", ASCII);
				Save16BitArrayToFile(pOutData, BurstSize, "dac1_secondary.bin", BINARY);
			}
		}
		// configure the router ( route data to DAC1's wave form memory )
		routerSetting = 0xff00;
		routerSetting = routerSetting << (currentCard * 16);
		routerSetting = ~routerSetting;

		if(sxdx_configurerouter(AddrSipRouterS1D3, routerSetting)!=SXDXROUTER_ERR_OK) {
			printf("Could not configure S1D3 router, exiting\n");
			sipif_free();
			_aligned_free(pOutData);
			_aligned_free(pInData);
			return -14;
		}
		// prepare the firmware to receive waveform data
		if(fmc15x_ctrl_prepare_wfm_load(AddrSipFMC150Ctrl, DAC1)!=FMC15x_CTRL_ERR_OK) {
			printf("Could not prepare waveform upload, exiting\n");
			sipif_free();
			_aligned_free(pOutData);
			_aligned_free(pInData);
			return -18;
		}

		// send the data to the waveform memory
		if(sipif_writedata(pOutData,  2*BurstSize)!=SIPIF_ERR_OK) {
			printf("Could not communicate with device %d.\n", devIdx);
			sipif_free();
			_aligned_free(pOutData);
			_aligned_free(pInData);
			return -19;
		}


		bool pattern_check_passed = false;
		for (;;) {

			if (pattern_check_passed == false) {
				printf ("Running ramp pattern check on card %d......\n", currentCard);
				if (fmc15x_adc_pattern_check(AddrSipFMC150AdcSpi, true) != FMC15x_ADC_ERR_OK)
				{
					printf ("Could not enabled pattern check\n");
					sipif_free();
					_aligned_free(pInData);
					return -13;
				}
			}
			else {
				printf ("Acquiring %d samples on card %d\n", BurstSize, currentCard);
				if (fmc15x_adc_pattern_check(AddrSipFMC150AdcSpi, false) != FMC15x_ADC_ERR_OK)
				{
					printf ("Could not enabled pattern check\n");
					sipif_free();
					_aligned_free(pInData);
					return -13;

				}
			}


			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Read a burst from ADC0 and save to file
			// route data from ADC0's FIFO
#ifdef WIN32
			if(currentCard == 0) {
				if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF00)!=SXDXROUTER_ERR_OK) {
					printf("Could not configure S3D1 router, exiting\n");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);
					return -20;
				}

			} else if(currentCard == 1) {			
				if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF02)!=SXDXROUTER_ERR_OK) {
					printf("Could not configure S3D1 router, exiting\n");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);
					return -20;
				}
			}
#else
			if (currentCard == 0) {
				if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF00LLU)!=SXDXROUTER_ERR_OK) {
					printf("Could not configure S3D1 router, exiting\n");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);
					return -20;
				}
			}
			else if (currentCard == 1) {
				if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF02LLU)!=SXDXROUTER_ERR_OK) {
					printf("Could not configure S3D1 router, exiting\n");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);
					return -20;
				}
			}
#endif
			
			// Configure the DDR3 FIFO
			if( constellation_id == CONSTELLATION_ID_FMC151_ZC706_DDR3) {
				// Configure and arm the FIFO
				if(memfifo_configure(AddrSipMemoryFIFO, 0, NBRBURST_UNLIMITED, 2*BurstSize, 0, 0, FIFO_ARMED)!=MEMFIFO_ERR_OK) {
					printf("Could not configure the memory FIFO\n ");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);
					return -20;
				}
			}

			// enable ADC0 + DAC0 + DAC1
			if(fmc15x_ctrl_enable_channel(AddrSipFMC150Ctrl, ENABLED, DISABLED, ENABLED, ENABLED)!=FMC15x_CTRL_ERR_OK) {
				printf("Could not enable, exiting\n");
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				return -21;
			}

			// arm the DAC
			if(fmc15x_ctrl_arm_dac(AddrSipFMC150Ctrl)!=FMC15x_CTRL_ERR_OK) {
				printf("Could not arm DAC0, exiting\n");
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				return -22;
			}

			// send a software trigger to the ADC block
			if(fmc15x_ctrl_sw_trigger(AddrSipFMC150Ctrl)!=FMC15x_CTRL_ERR_OK) {
				printf("Could not send software trigger to ADC0, exiting\n");
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				
				return -23;
			}

			// Read data from the pipe
			if (pattern_check_passed)
				printf("Retrieve %d samples from ADC0\n", BurstSize);
			if(sipif_readdata  (pInData,  2*BurstSize)!=SIPIF_ERR_OK) {
				printf("Could not communicate with device %d.\n", devIdx);
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				return -24;
			}

			if (pattern_check_passed == false) {
				rc = verify_ramp_pattern((char *)pInData, BurstSize);
			}
			else {
				if ((constellation_id != CONSTELLATION_ID_PC720_BOTH) && (constellation_id != CONSTELLATION_ID_FMC151_PC720_BOTH)) {
					// clean the files previously saved
					DeleteFile("adc0.bin");
					DeleteFile("adc0.txt");

					// write to file
					Save16BitArrayToFile(pInData, BurstSize, "adc0.txt", ASCII);
					Save16BitArrayToFile(pInData, BurstSize, "adc0.bin", BINARY);
				}
				else {
					if (currentCard == 0) {
						// clean the files previously saved
						DeleteFile("adc0_primary.bin");
						DeleteFile("adc0_primary.txt");

						// write to file
						Save16BitArrayToFile(pInData, BurstSize, "adc0_primary.txt", ASCII);
						Save16BitArrayToFile(pInData, BurstSize, "adc0_primary.bin", BINARY);
					}
					else {
						// clean the files previously saved
						DeleteFile("adc0_secondary.bin");
						DeleteFile("adc0_secondary.txt");

						// write to file
						Save16BitArrayToFile(pInData, BurstSize, "adc0_secondary.txt", ASCII);
						Save16BitArrayToFile(pInData, BurstSize, "adc0_secondary.bin", BINARY);
					}
				}
			}
			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// Read a burst from ADC1 and save to file 
			// route data from ADC1's FIFO
#ifdef WIN32
			if(currentCard == 0) {
				if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF01)!=SXDXROUTER_ERR_OK) {
					printf("Could not configure S3D1 router, exiting\n");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);;
					return -25;
				}
			} else if(currentCard == 1) {
				if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF03)!=SXDXROUTER_ERR_OK) {
					printf("Could not configure S3D1 router, exiting\n");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);;
					return -25;
				}
			}
#else
			if (currentCard == 0) {
				if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF01LLU)!=SXDXROUTER_ERR_OK) {
					printf("Could not configure S3D1 router, exiting\n");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);;
					return -25;
				}
			}
			else if (currentCard == 1) {
				if(sxdx_configurerouter(AddrSipRouterS3D1, 0xFFFFFFFFFFFFFF03LLU)!=SXDXROUTER_ERR_OK) {
					printf("Could not configure S3D1 router, exiting\n");
					sipif_free();
					_aligned_free(pOutData);
					_aligned_free(pInData);;
					return -25;
				}
			}
#endif
			// enable ADC1 + DAC0 + DAC1
			if(fmc15x_ctrl_enable_channel(AddrSipFMC150Ctrl, DISABLED, ENABLED, ENABLED, ENABLED)!=FMC15x_CTRL_ERR_OK) {
				printf("Could not enable, exiting\n");
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				return -26;
			}

			// arm the DAC
			if(fmc15x_ctrl_arm_dac(AddrSipFMC150Ctrl)!=FMC15x_CTRL_ERR_OK) {
				printf("Could not arm DAC1, exiting\n");
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				return -27;
			}

			// send a software trigger to the ADC block
			if(fmc15x_ctrl_sw_trigger(AddrSipFMC150Ctrl)!=FMC15x_CTRL_ERR_OK) {
				printf("Could not send software trigger to DAC1, exiting\n");
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				return -28;
			}

			// Read from the pipe
			if (pattern_check_passed)
				printf("Retrieve %d samples from ADC1\n", BurstSize);
			if(sipif_readdata(pInData,  2*BurstSize)!=SIPIF_ERR_OK) {
				printf("Could not communicate with device %d.\n", devIdx);
				sipif_free();
				_aligned_free(pOutData);
				_aligned_free(pInData);
				return -29;
			}

			if (pattern_check_passed == false) {
				rc = verify_ramp_pattern((char *)pInData, BurstSize);
				pattern_check_passed = true;
			}
			else {

				if ((constellation_id != CONSTELLATION_ID_PC720_BOTH) && (constellation_id != CONSTELLATION_ID_FMC151_PC720_BOTH)) {
					// clean the files previously saved
					DeleteFile("adc1.bin");
					DeleteFile("adc1.txt");

					// write to file
					Save16BitArrayToFile(pInData, BurstSize, "adc1.txt", ASCII);
					Save16BitArrayToFile(pInData, BurstSize, "adc1.bin", BINARY);
				}
				else {
					if (currentCard == 0) {
						// clean the files previously saved
						DeleteFile("adc1_primary.bin");
						DeleteFile("adc1_primary.txt");

						// write to file
						Save16BitArrayToFile(pInData, BurstSize, "adc1_primary.txt", ASCII);
						Save16BitArrayToFile(pInData, BurstSize, "adc1_primary.bin", BINARY);
					}
					else {
						// clean the files previously saved
						DeleteFile("adc1_secondary.bin");
						DeleteFile("adc1_secondary.txt");

						// write to file
						Save16BitArrayToFile(pInData, BurstSize, "adc1_secondary.txt", ASCII);
						Save16BitArrayToFile(pInData, BurstSize, "adc1_secondary.bin", BINARY);
					}
				}

				// exit the for (;;) loop
				break;
			}
		}
		_aligned_free(pOutData);
		_aligned_free(pInData);
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Close the device
	printf("\nEnd of program.\n\n\n");
	sipif_free();
#ifdef WIN32		
	// wait user entry before closing the application
	system("pause");
#else
	printf ("Press a key to exit the application\n");
	getchar();
#endif 
	return 0;
}

