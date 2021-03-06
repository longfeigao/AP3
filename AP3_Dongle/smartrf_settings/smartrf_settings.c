

//*********************************************************************************
// Generated by SmartRF Studio version 2.6.0 (build #81)
// Tested for SimpleLink SDK version: CC13x0 SDK 1.0.xx
// Device: CC1350 Rev. 2.1
//
//*********************************************************************************


#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_ble_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_cmd.h)
#include <ti/devices/cc26x0r2\inc\hw_rfc_dbell.h>
#include <ti/drivers/rf/RF.h>
#include "smartrf_settings.h"
#include "apply_genfsk_mce_patch.h"
#include "apply_genfsk_rfe_patch.h"
#include "rf_patch_cpe_prop.h"

#define COMPAT_E30_E31

#ifdef COMPAT_E30_E31
// TI-RTOS RF Mode Object
RF_Mode RF_prop =
{
    .rfMode = RF_MODE_PROPRIETARY_2_4,
    .cpePatchFxn = &rf_patch_cpe_prop,///////////////////////////////////////////////////////////////////////////////////////
    .mcePatchFxn = &enterGenfskMcePatch,
    .rfePatchFxn = &enterGenfskRfePatch,
};
#else
RF_Mode RF_prop =
{
    .rfMode = RF_MODE_PROPRIETARY_2_4,
    .cpePatchFxn = &rf_patch_cpe_prop,  // Shape 2FSK => GFSK with BT 2,
    .mcePatchFxn = 0,
    .rfePatchFxn = 0,
};
#endif

#ifdef COMPAT_E30_E31
// Overrides for CMD_PROP_RADIO_SETUP
uint32_t pOverrides500[] =
{
    MCE_RFE_OVERRIDE(1,0,0,1,0,0), 
     HW_REG_OVERRIDE(0x6088,0x3F1F),          //PA trim ramping and AGC ref
     //HW_REG_OVERRIDE(0x608C,0x8213),          // PA IB ramping and AGC rssicount/wait, 50kbps-0x8213, 150kbps-0x0213, 200kbps-0x0113
     HW_REG_OVERRIDE(0x608C,0x0A13),          
     (uint32_t)0x000484A3,                    // Synth: Set FREF = 6 MHz (24 MHz / 4)
     (uint32_t)0x02010403,                    //Fixes phase error discard
     HW32_ARRAY_OVERRIDE(0x2004, 1),          // Configure new CRC16 polynom
     (uint32_t) 0x10210000,                   // The CRC16 polynome: CRC-16-CCITT normal form, 0x1021 is x^16 + x^15 + x^5 + 1
     (uint32_t) 0xC0040051,                   // Configure new CRC init value
     (uint32_t) 0x1D0F0000,                   // Change CRC init value to 0x1D0F

     (uint32_t) 0x00018063,                   //cache used GPRAM,add
     HW_REG_OVERRIDE(0x51F8, 0x0570),

     (uint32_t) 0x008F88B3,
     HW_REG_OVERRIDE(0x1110,  RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO1 | RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO1),

     HW_REG_OVERRIDE(0x6084, 0x35F1),         //This should make sure that the RSSI is not beeing updated after sync is found.
     (uint32_t)0xFFFFFFFF,
};

// Overrides for CMD_PROP_RADIO_SETUP
uint32_t pOverrides100[] =
{
     HW_REG_OVERRIDE(0x6088,0x3F1F),          //PA trim ramping and AGC ref
     HW_REG_OVERRIDE(0x608C,0x8213),          //PA IB ramping and AGC rssicount/wait, 50kbps-0x8213, 150kbps-0x0213, 200kbps-0x0113
     (uint32_t)0x000484A3,                    // Synth: Set FREF = 6 MHz (24 MHz / 4)
     (uint32_t)0x02010403,                    //Fixes phase error discard
     HW32_ARRAY_OVERRIDE(0x2004, 1),          // Configure new CRC16 polynom
     (uint32_t) 0x10210000,                   // The CRC16 polynome: CRC-16-CCITT normal form, 0x1021 is x^16 + x^15 + x^5 + 1
     (uint32_t) 0xC0040051,                   // Configure new CRC init value
     (uint32_t) 0x1D0F0000,                   // Change CRC init value to 0x1D0F

     (uint32_t) 0x00018063,                   //cache used GPRAM,add

     (uint32_t) 0x008F88B3,
     HW_REG_OVERRIDE(0x1110,  RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO1 | RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO1),

     HW_REG_OVERRIDE(0x6084, 0x35F1),         //This should make sure that the RSSI is not beeing updated after sync is found.
     (uint32_t)0xFFFFFFFF,
};
#else
// Overrides for CMD_PROP_RADIO_SETUP
uint32_t pOverrides500[] =
{
    HW_REG_OVERRIDE(0x6088,0x3F1F),          //PA trim ramping and AGC ref
    HW_REG_OVERRIDE(0x608C,0x8213),          //PA IB ramping and AGC rssicount/wait, 50kbps-0x8213, 150kbps-0x0213, 200kbps-0x0113
    (uint32_t)0x000484A3,                    // Synth: Set FREF = 6 MHz (24 MHz / 4)
    (uint32_t)0x02010403,                    //Fixes phase error discard
    HW32_ARRAY_OVERRIDE(0x2004, 1),          // Configure new CRC16 polynom
    (uint32_t) 0x10210000,                   // The CRC16 polynome: CRC-16-CCITT normal form, 0x1021 is x^16 + x^15 + x^5 + 1
    (uint32_t) 0xC0040051,                   // Configure new CRC init value
    (uint32_t) 0x1D0F0000,                   // Change CRC init value to 0x1D0F

    (uint32_t) 0x00018063,                   //cache used GPRAM,add

    (uint32_t) 0x008F88B3,
    HW_REG_OVERRIDE(0x1110,  RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO1 | RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO1),
    HW_REG_OVERRIDE(0x6084, 0x35F1),         //This should make sure that the RSSI is not beeing updated after sync is found.
    (uint32_t)0xFFFFFFFF,
};

// Overrides for CMD_PROP_RADIO_SETUP
uint32_t pOverrides100[] =
{
     HW_REG_OVERRIDE(0x6088,0x3F1F),          //PA trim ramping and AGC ref
     HW_REG_OVERRIDE(0x608C,0x8213),          //PA IB ramping and AGC rssicount/wait, 50kbps-0x8213, 150kbps-0x0213, 200kbps-0x0113
     (uint32_t)0x000484A3,                    // Synth: Set FREF = 6 MHz (24 MHz / 4)
     (uint32_t)0x02010403,                    //Fixes phase error discard
     HW32_ARRAY_OVERRIDE(0x2004, 1),          // Configure new CRC16 polynom
     (uint32_t) 0x10210000,                   // The CRC16 polynome: CRC-16-CCITT normal form, 0x1021 is x^16 + x^15 + x^5 + 1
     (uint32_t) 0xC0040051,                   // Configure new CRC init value
     (uint32_t) 0x1D0F0000,                   // Change CRC init value to 0x1D0F

     (uint32_t) 0x00018063,                   //cache used GPRAM,add

     (uint32_t) 0x008F88B3,
     HW_REG_OVERRIDE(0x1110,  RFC_DBELL_SYSGPOCTL_GPOCTL0_CPEGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL1_RATGPO0 | RFC_DBELL_SYSGPOCTL_GPOCTL2_MCEGPO1 | RFC_DBELL_SYSGPOCTL_GPOCTL3_RATGPO1),
     HW_REG_OVERRIDE(0x6084, 0x35F1),         //This should make sure that the RSSI is not beeing updated after sync is found.
     (uint32_t)0xFFFFFFFF,
};

#endif

// CMD_PROP_RADIO_SETUP
rfc_CMD_PROP_RADIO_SETUP_t RF_cmdPropRadioSetup =
{
    .commandNo = CMD_PROP_RADIO_SETUP,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .modulation.modType = 0x0,//////////////////////////////////////////////////
    .modulation.deviation = 744,////////////////////////////////////////////////
    .symbolRate.preScale = 15,
    .symbolRate.rateWord = 327680,
    .rxBw = 10,
    .preamConf.nPreamBytes = 0x4,///////////////////////////////////////////////
    .preamConf.preamMode = 0x0,
    .formatConf.nSwBits = 32,
    .formatConf.bBitReversal = 0x0,
    .formatConf.bMsbFirst = 0x1,
    .formatConf.fecMode = 0x0,
    .formatConf.whitenMode = 0x0,
    .config.frontEndMode = 0x0,
    .config.biasMode = 0x0,
    .config.analogCfgMode = 0x0,
    .config.bNoFsPowerUp = 0x0,
    .txPower = 0x0CC9, //0x3161 = 0dB
    .pRegOverride = pOverrides100,
};

// CMD_FS
rfc_CMD_FS_t RF_cmdFs =
{
    .commandNo = 0x0803,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .frequency = 2440,
    .fractFreq = 0x0000,
    .synthConf.bTxMode = 0x0,
    .synthConf.refFreq = 0x0,
    .__dummy0 = 0x00,
    .__dummy1 = 0x00,
    .__dummy2 = 0x00,
    .__dummy3 = 0x0000,
};

// CMD_PROP_TX_ADV
rfc_CMD_PROP_TX_ADV_t RF_cmdPropTxAdv[2] ={
{
    .commandNo = CMD_PROP_TX_ADV,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .pktConf.bFsOff = 0x0,
    .pktConf.bUseCrc = 0x1,
    .pktConf.bCrcIncSw = 0x0,
    .pktConf.bCrcIncHdr = 0x0,
    .pktLen = 26, // SET APPLICATION PAYLOAD LENGTH
    .syncWord = 0x52567853,
    .pPkt = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
},
{
    .commandNo = CMD_PROP_TX_ADV,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .pktConf.bFsOff = 0x0,
    .pktConf.bUseCrc = 0x1,
    .pktConf.bCrcIncSw = 0x0,
    .pktConf.bCrcIncHdr = 0x0,
    .pktLen = 26, // SET APPLICATION PAYLOAD LENGTH
    .syncWord = 0x52567853,
    .pPkt = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
}
};

// CMD_PROP_RX_ADV
rfc_CMD_PROP_RX_ADV_t RF_cmdPropRxAdv =
{
    .commandNo = CMD_PROP_RX_ADV,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .pktConf.bRepeatOk = 0x0,
    .pktConf.bFsOff = 0x0,
    .pktConf.bRepeatNok = 1,
    .pktConf.bUseCrc = 0x1,
    .pktConf.bCrcIncSw = 0,
    .pktConf.bCrcIncHdr = 0,
    .pktConf.endType = 0x0,
    .rxConf.bAutoFlushIgnored = 0x1,
    .rxConf.bAutoFlushCrcErr = 0x1,
    .rxConf.bAppendRssi = 0x1,
    .rxConf.bAppendTimestamp = 0x0,
    .rxConf.bAppendStatus = 0x1,     // 0x1,  append a status byte to the packet in the RX queue
    .syncWord0 = 0x52567853,
    .syncWord1 = 0,
    .maxPktLen = 0x80, // MAKE SURE DATA ENTRY IS LARGE ENOUGH
    .endTrigger.triggerType = 0x1,
    .endTrigger.bEnaCmd = 0x1,
    .endTrigger.triggerNo = 0x0,
    .endTrigger.pastTrig = 0x0,
    .endTime = 0x00000000,
    .pQueue = 0, // INSERT APPLICABLE POINTER: (dataQueue_t*)&xxx
    .pOutput = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
};

// CMD_TX_TEST
rfc_CMD_TX_TEST_t RF_cmdTxTest =
{
    .commandNo = 0x0808,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .config.bUseCw = 0x1,
    .config.bFsOff = 0x1,
    .config.whitenMode = 0x2,
    .__dummy0 = 0x00,
    .txWord = 0xABCD,
    .__dummy1 = 0x00,
    .endTrigger.triggerType = 0x1,
    .endTrigger.bEnaCmd = 0x0,
    .endTrigger.triggerNo = 0x0,
    .endTrigger.pastTrig = 0x0,
    .syncWord = 0x930B51DE,
    .endTime = 0x00000000,
};

// CMD_RX_TEST
rfc_CMD_RX_TEST_t RF_cmdRxTest =
{
    .commandNo = 0x0807,
    .status = 0x0002,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger = 0x0,
    .condition = 0x1,
    .config     = 0x04,
    .endTrigger = 0x01,
    .syncWord   = 0x52567853,
    .endTime    = 0x00000000,
};

