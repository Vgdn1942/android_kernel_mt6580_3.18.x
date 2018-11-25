#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"
#include <linux/regulator/consumer.h>

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(PFX fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)   pr_err(fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
		do {    \
		   pr_debug(PFX fmt, ##arg); \
		} while (0)
#else
#define PK_DBG(a, ...)
#define PK_ERR(a, ...)
#define PK_XLOG_INFO(fmt, args...)
#endif

#if HQ_ANTI_CAMERA_INTERACT
	UsedSubCameraType   g_CurrUsedSubCameraName = SUB_NOCAMERA;
	UsedMainCameraType  g_CurrUsedMainCameraName = MAIN_NOCAMERA;	
#endif

#if !defined(CONFIG_MTK_LEGACY)
/* GPIO Pin control*/
struct platform_device *cam_plt_dev = NULL;
struct pinctrl *camctrl = NULL;
struct pinctrl_state *cam0_pnd_h = NULL;
struct pinctrl_state *cam0_pnd_l = NULL;
struct pinctrl_state *cam0_rst_h = NULL;
struct pinctrl_state *cam0_rst_l = NULL;
struct pinctrl_state *cam1_pnd_h = NULL;
struct pinctrl_state *cam1_pnd_l = NULL;
struct pinctrl_state *cam1_rst_h = NULL;
struct pinctrl_state *cam1_rst_l = NULL;
struct pinctrl_state *cam_ldo0_h = NULL;
struct pinctrl_state *cam_ldo0_l = NULL;

int mtkcam_gpio_init(struct platform_device *pdev)
{
	int ret = 0;

	camctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(camctrl)) {
		dev_err(&pdev->dev, "Cannot find camera pinctrl!");
		ret = PTR_ERR(camctrl);
	}
	/*Cam0 Power/Rst Ping initialization */
	cam0_pnd_h = pinctrl_lookup_state(camctrl, "cam0_pnd1");
	if (IS_ERR(cam0_pnd_h)) {
		ret = PTR_ERR(cam0_pnd_h);
		pr_debug("%s : pinctrl err, cam0_pnd_h\n", __func__);
	}

	cam0_pnd_l = pinctrl_lookup_state(camctrl, "cam0_pnd0");
	if (IS_ERR(cam0_pnd_l)) {
		ret = PTR_ERR(cam0_pnd_l);
		pr_debug("%s : pinctrl err, cam0_pnd_l\n", __func__);
	}


	cam0_rst_h = pinctrl_lookup_state(camctrl, "cam0_rst1");
	if (IS_ERR(cam0_rst_h)) {
		ret = PTR_ERR(cam0_rst_h);
		pr_debug("%s : pinctrl err, cam0_rst_h\n", __func__);
	}

	cam0_rst_l = pinctrl_lookup_state(camctrl, "cam0_rst0");
	if (IS_ERR(cam0_rst_l)) {
		ret = PTR_ERR(cam0_rst_l);
		pr_debug("%s : pinctrl err, cam0_rst_l\n", __func__);
	}

	/*Cam1 Power/Rst Ping initialization */
	cam1_pnd_h = pinctrl_lookup_state(camctrl, "cam1_pnd1");
	if (IS_ERR(cam1_pnd_h)) {
		ret = PTR_ERR(cam1_pnd_h);
		pr_debug("%s : pinctrl err, cam1_pnd_h\n", __func__);
	}

	cam1_pnd_l = pinctrl_lookup_state(camctrl, "cam1_pnd0");
	if (IS_ERR(cam1_pnd_l )) {
		ret = PTR_ERR(cam1_pnd_l );
		pr_debug("%s : pinctrl err, cam1_pnd_l\n", __func__);
	}


	cam1_rst_h = pinctrl_lookup_state(camctrl, "cam1_rst1");
	if (IS_ERR(cam1_rst_h)) {
		ret = PTR_ERR(cam1_rst_h);
		pr_debug("%s : pinctrl err, cam1_rst_h\n", __func__);
	}


	cam1_rst_l = pinctrl_lookup_state(camctrl, "cam1_rst0");
	if (IS_ERR(cam1_rst_l)) {
		ret = PTR_ERR(cam1_rst_l);
		pr_debug("%s : pinctrl err, cam1_rst_l\n", __func__);
	}
	/*externel LDO enable */
	cam_ldo0_h = pinctrl_lookup_state(camctrl, "cam_ldo0_1");
	if (IS_ERR(cam_ldo0_h)) {
		ret = PTR_ERR(cam_ldo0_h);
		pr_debug("%s : pinctrl err, cam_ldo0_h\n", __func__);
	}


	cam_ldo0_l = pinctrl_lookup_state(camctrl, "cam_ldo0_0");
	if (IS_ERR(cam_ldo0_l)) {
		ret = PTR_ERR(cam_ldo0_l);
		pr_debug("%s : pinctrl err, cam_ldo0_l\n", __func__);
	}
	return ret;
}

int mtkcam_gpio_set(int PinIdx, int PwrType, int Val)
{
	int ret = 0;

	switch (PwrType) {
	case CAMRST:
		if (PinIdx == 0) {
			if (Val == 0)
				pinctrl_select_state(camctrl, cam0_rst_l);
			else
				pinctrl_select_state(camctrl, cam0_rst_h);
		} else {
			if (Val == 0)
				pinctrl_select_state(camctrl, cam1_rst_l);
			else
				pinctrl_select_state(camctrl, cam1_rst_h);
		}
		break;
	case CAMPDN:
		if (PinIdx == 0) {
			if (Val == 0)
				pinctrl_select_state(camctrl, cam0_pnd_l);
			else
				pinctrl_select_state(camctrl, cam0_pnd_h);
		} else {
			if (Val == 0)
				pinctrl_select_state(camctrl, cam1_pnd_l);
			else
				pinctrl_select_state(camctrl, cam1_pnd_h);
		}

		break;
	case CAMLDO:
		if (Val == 0)
			pinctrl_select_state(camctrl, cam_ldo0_l);
		else
			pinctrl_select_state(camctrl, cam_ldo0_h);
		break;
	default:
		PK_DBG("PwrType(%d) is invalid !!\n", PwrType);
		break;
	};

	PK_DBG("PinIdx(%d) PwrType(%d) val(%d)\n", PinIdx, PwrType, Val);

	return ret;
}


int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On,
		       char *mode_name)
{

	u32 pinSetIdx = 0;	/* default main sensor */

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4
#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3
#define VOL_2800 2800000
#define VOL_1800 1800000
#define VOL_1500 1500000
#define VOL_1200 1200000
#define VOL_1000 1000000


	u32 pinSet[3][8] = {
		/* for main sensor */
		{		/* The reset pin of main sensor uses GPIO10 of mt6306, please call mt6306 API to set */
		 CAMERA_CMRST_PIN,
		 CAMERA_CMRST_PIN_M_GPIO,	/* mode */
		 GPIO_OUT_ONE,	/* ON state */
		 GPIO_OUT_ZERO,	/* OFF state */
		 CAMERA_CMPDN_PIN,
		 CAMERA_CMPDN_PIN_M_GPIO,
		 GPIO_OUT_ONE,
		 GPIO_OUT_ZERO,
		 },
		/* for sub sensor */
		{
		 CAMERA_CMRST1_PIN,
		 CAMERA_CMRST1_PIN_M_GPIO,
		 GPIO_OUT_ONE,
		 GPIO_OUT_ZERO,
		 CAMERA_CMPDN1_PIN,
		 CAMERA_CMPDN1_PIN_M_GPIO,
		 GPIO_OUT_ONE,
		 GPIO_OUT_ZERO,
		 },
		/* for main_2 sensor */
		{
		 GPIO_CAMERA_INVALID,
		 GPIO_CAMERA_INVALID,	/* mode */
		 GPIO_OUT_ONE,	/* ON state */
		 GPIO_OUT_ZERO,	/* OFF state */
		 GPIO_CAMERA_INVALID,
		 GPIO_CAMERA_INVALID,
		 GPIO_OUT_ONE,
		 GPIO_OUT_ZERO,
		 }
	};

	if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
		pinSetIdx = 0;
	else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx)
		pinSetIdx = 1;
	else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx)
		pinSetIdx = 2;

	/* power ON */
	if (On) {

#if 0
		ISP_MCLK1_EN(1);
		ISP_MCLK2_EN(1);
		ISP_MCLK3_EN(1);
#else
/*
		if (pinSetIdx == 0)
			ISP_MCLK1_EN(1);
		else if (pinSetIdx == 1)
			ISP_MCLK2_EN(1);
*/
#endif

		printk("[PowerON]pinSetIdx:%d, currSensorName: %s\n", pinSetIdx, currSensorName);

#if HQ_ANTI_CAMERA_INTERACT
		if (pinSetIdx == 0){		
			 switch (g_CurrUsedSubCameraName){
				   case SUB_HI553:
						   printk("\n[imgsensor] found subcamera hi553\n");
						   if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMPDN])
							   mtkcam_gpio_set(1, CAMPDN,pinSet[1][IDX_PS_CMPDN + IDX_PS_OFF]);
			
						   if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMRST])
							   mtkcam_gpio_set(1, CAMRST,pinSet[1][IDX_PS_CMRST + IDX_PS_OFF]);
						   break;
			
				   case SUB_GC5005:
						   printk("\n [imgsensor] found subcamera gc5005 \n");
						   if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMPDN])
							   mtkcam_gpio_set(1, CAMPDN,pinSet[1][IDX_PS_CMPDN + IDX_PS_ON]);
						   if (GPIO_CAMERA_INVALID != pinSet[1][IDX_PS_CMRST])
							   mtkcam_gpio_set(1, CAMRST,pinSet[1][IDX_PS_CMRST + IDX_PS_OFF]); 	   
						   break;
						   
				   default:
						   printk("\n [imgsensor] no subcamera found \n");
			 }
			
		} else if (pinSetIdx == 1) {
		
				 if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMPDN])
					   	mtkcam_gpio_set(0, CAMPDN,pinSet[0][IDX_PS_CMPDN + IDX_PS_OFF]);
					   	
			   	 if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMRST])
						   mtkcam_gpio_set(0, CAMRST,pinSet[0][IDX_PS_CMRST + IDX_PS_OFF]);
		
		}
			   
#endif

/***********   HIXX series   *****************/
/****   HI843B POWER ON   ****/
		if (currSensorName  && (0 == strcmp(SENSOR_DRVNAME_HI843B_MIPI_RAW, currSensorName))) {
				/* First Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);


				mdelay(1);

				/* VCAM_IO */
				if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
					PK_DBG ("[CAMERA SENSOR] Fail to enable IO power (VCAM_IO), power id = %d\n",VCAMIO);     
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);
				

				/* VCAM_A */
				if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d\n", VCAMA);    
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);

				if (TRUE != _hwPowerOn(VCAMD, VOL_1200)) {
					PK_DBG  ("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d\n", VCAMD);    
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);

				ISP_MCLK1_EN(1);

#if 1			
				/* AF_VCC */
				if (TRUE != _hwPowerOn(VCAMAF, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable AF power (VCAM_AF), power id = %d\n", VCAMAF);		
					goto _kdCISModulePowerOn_exit_;
				}
#endif

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);
				
				/* enable active sensor */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON]);	

				
/****   HI553 POWER ON   ****/						
		} else if (currSensorName  && (0 == strcmp(SENSOR_DRVNAME_HI553_MIPI_RAW, currSensorName))) {

				
				/* First Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
				

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				


				/* VCAM_IO */
				if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
					PK_DBG("[CAMERA SENSOR]Fail to enable digital power(VCAM_IO),power id = %d\n",VCAMIO);
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);
				/* VCAM_A */
				if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A),power id = %d\n",VCAMA);
					goto _kdCISModulePowerOn_exit_;
				}
				//mdelay(1);
				
				/* VCAM_D */
				if (TRUE != _hwPowerOn(SUB_VCAMD, VOL_1200)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}
				//mdelay(1);

#if 0
				/* AF_VCC */
				if (TRUE != _hwPowerOn(VCAMAF, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF),power id = %d\n",VCAMAF);
					goto _kdCISModulePowerOn_exit_;
				}
#endif
				ISP_MCLK2_EN(1);
				mdelay(1);

				/* enable active sensor */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);
				
				//mdelay(1);

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON]);
				
/*********   OVXX series   ******************/	
/******   OV8856_HUAQUAN POWERON   *******/				
		} else if (currSensorName  && (0 == strcmp(SENSOR_DRVNAME_OV8856_HUAQUAN_MIPI_RAW, currSensorName))) {
				/* First Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
				

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				

				ISP_MCLK1_EN(1);
				mdelay(1);
				
				/* VCAM_A */
				if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A),power id = %d\n",VCAMA);
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);
				
				/* VCAM_IO */
				if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
					PK_DBG("[CAMERA SENSOR]Fail to enable digital power(VCAM_IO),power id = %d\n",VCAMIO);
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);
				/* VCAM_D */
				if (TRUE != _hwPowerOn(VCAMD, VOL_1200)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);

				/* AF_VCC */
				if (TRUE != _hwPowerOn(VCAMAF, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF),power id = %d\n",VCAMAF);
					goto _kdCISModulePowerOn_exit_;
				}
	
				/* enable active sensor */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);


				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON]);
				
				
/******   OV8856_QUNHUI POWERON   *******/				
		} else if (currSensorName  && (0 == strcmp(SENSOR_DRVNAME_OV8856_QUNHUI_MIPI_RAW, currSensorName))) {
				/* First Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
				

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				

				ISP_MCLK1_EN(1);
				mdelay(1);
				
				/* VCAM_A */
				if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A),power id = %d\n",VCAMA);
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);
				
				/* VCAM_IO */
				if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
					PK_DBG("[CAMERA SENSOR]Fail to enable digital power(VCAM_IO),power id = %d\n",VCAMIO);
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);
				/* VCAM_D */
				if (TRUE != _hwPowerOn(VCAMD, VOL_1200)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(1);

				/* AF_VCC */
				if (TRUE != _hwPowerOn(VCAMAF, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF),power id = %d\n",VCAMAF);
					goto _kdCISModulePowerOn_exit_;
				}
	
				/* enable active sensor */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);
				


				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON]);				
/******   GCXX series   *************/	
		}
		else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC5025_MIPI_RAW, currSensorName))){
                    // mt_set_gpio_mode(GPIO_SPI_MOSI_PIN,GPIO_MODE_00);
                    //mt_set_gpio_dir(GPIO_SPI_MOSI_PIN,GPIO_DIR_OUT);
                    //mt_set_gpio_out(GPIO_SPI_MOSI_PIN,GPIO_OUT_ONE);
                    //First Power Pin low and Reset Pin Low
                    //  change camera power on begin by likai

                    if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMPDN])
                        mtkcam_gpio_set(0, CAMPDN, pinSet[0][IDX_PS_CMPDN + IDX_PS_OFF]);

                    mdelay(1);

                    if (GPIO_CAMERA_INVALID != pinSet[0][IDX_PS_CMRST])
                        mtkcam_gpio_set(0, CAMRST, pinSet[0][IDX_PS_CMRST + IDX_PS_OFF]);

                    mdelay(1);
                    //  change camera power on end by likai

                    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
                        mtkcam_gpio_set(pinSetIdx, CAMPDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);

                    mdelay(1);

                    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
                        mtkcam_gpio_set(pinSetIdx, CAMRST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);

                    mdelay(1);

                    //VCAM_IO
                    if(TRUE != _hwPowerOn(VCAMIO, VOL_1800 ))
                    {
                        PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAMIO), power id = %d \n", VCAMIO);
                        goto _kdCISModulePowerOn_exit_;
                    }

                    mdelay(1);

                    //DVDD
                    if(TRUE != _hwPowerOn(SUB_VCAMD, VOL_1200 ))
                    {
                        PK_DBG("[CAMERA SENSOR] Fail to enable digital power (SUB_VCAMD), power id = %d \n", SUB_VCAMD);
                        goto _kdCISModulePowerOn_exit_;
                    }

                    mdelay(2);

                    //VCAM_A
                    if(TRUE != _hwPowerOn(VCAMA, VOL_2800))
                    {
                        PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d\n", VCAMA);
                        goto _kdCISModulePowerOn_exit_;
                    }

                    mdelay(1);

                    ISP_MCLK2_EN(1);

                    mdelay(1);

                    //enable active sensor
                    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
                        mtkcam_gpio_set(pinSetIdx, CAMPDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);

                    mdelay(1);

                    if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
                        mtkcam_gpio_set(pinSetIdx, CAMRST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON]);

                    mdelay(1);
        }	
        else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_S5K4H8_MIPI_RAW, currSensorName)))
        {
            ISP_MCLK1_EN(1);

            mdelay(2);

            //First Power Pin low and Reset Pin Low
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
                mtkcam_gpio_set(pinSetIdx, CAMPDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);

            mdelay(1);

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
                mtkcam_gpio_set(pinSetIdx, CAMRST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);

            mdelay(1);

            //VCAM_A
            if(TRUE != _hwPowerOn(VCAMA, VOL_2800))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d\n", VCAMA);
                goto _kdCISModulePowerOn_exit_;
            }

            mdelay(1);

            if(TRUE != _hwPowerOn(VCAMIO, VOL_1800 ))
            {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %d \n", VCAMIO);
                goto _kdCISModulePowerOn_exit_;
            }

            mdelay(1);

            if (TRUE != _hwPowerOn(VCAMD, VOL_1200)) {
                PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %d \n", VCAMD);
                goto _kdCISModulePowerOn_exit_;
            }

            mdelay(2);


            //AF_VCC
            if (TRUE != _hwPowerOn(VCAMAF, VOL_2800)) {
                PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %d \n", VCAMAF);
                goto _kdCISModulePowerOn_exit_;
            }

            mdelay(2);

            //enable active sensor
            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
                mtkcam_gpio_set(pinSetIdx, CAMPDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);

            mdelay(1);

            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
                mtkcam_gpio_set(pinSetIdx, CAMRST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON]);

            mdelay(1);

/******   GC5005 POWERON   *******/	
		} else if (currSensorName  && (0 == strcmp(SENSOR_DRVNAME_GC5005_MIPI_RAW, currSensorName))) {
		
					
				/* First Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);
				
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				

				/* VCAM_IO */
				if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
					PK_DBG("[CAMERA SENSOR]Fail to enable digital power(VCAM_IO),power id = %d\n",VCAMIO);
					goto _kdCISModulePowerOn_exit_;
				}
				
				//mdelay(1);

				if (TRUE != _hwPowerOn(SUB_VCAMD, VOL_1200)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}

				//mdelay(1);
				
				/* VCAM_A */
				if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A),power id = %d\n",VCAMA);
					goto _kdCISModulePowerOn_exit_;
				}

				//mdelay(1);
						
#if 0
				/* AF_VCC */
				if (TRUE != _hwPowerOn(VCAMAF, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF),power id = %d\n",VCAMAF);
					goto _kdCISModulePowerOn_exit_;
				}
#endif
				ISP_MCLK2_EN(1);
				mdelay(3);

	
				/* enable active sensor */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON]);
					
/*****  default power on  ********/						
		} else {
				/* First Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
				

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				

				/* VCAM_IO */
				if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable IO power (VCAM_IO), power id = %d\n",VCAMIO);
					goto _kdCISModulePowerOn_exit_;
				}

				/* VCAM_A */
				if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d\n",VCAMA);
					goto _kdCISModulePowerOn_exit_;
				}
				/* VCAM_D */
				
				if (TRUE != _hwPowerOn(VCAMD, VOL_1200)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
					goto _kdCISModulePowerOn_exit_;
				}

				/* AF_VCC */
				if (TRUE != _hwPowerOn(VCAMAF, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF),power id = %d\n",VCAMAF);
					goto _kdCISModulePowerOn_exit_;
				}

				mdelay(5);

				/* enable active sensor */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);
				
				mdelay(1);

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON]);
				
			}
				
				
/***********************************        POWER OFF    *****************************************/	


	} else {		/* power OFF */

		PK_DBG("[PowerOFF]pinSetIdx:%d\n", pinSetIdx);
		if (pinSetIdx == 0)
			ISP_MCLK1_EN(0);
		else if (pinSetIdx == 1)
			ISP_MCLK2_EN(0);

/********HIXX series**********/
/********HI843B POWER OFF***********/
		if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI843B_MIPI_RAW, currSensorName))) {
				
				
				/* Set Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				
				/* Set Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);

				ISP_MCLK1_EN(0);
				
				/* AF_VCC */
				if (TRUE != _hwPowerDown(VCAMAF)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF),power id = %d\n",VCAMAF);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}

				/* VCAM_D */
				if (TRUE != _hwPowerDown(VCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}

				/* VCAM_A */
				if (TRUE != _hwPowerDown(VCAMA)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
				
				/* VCAM_IO */
				if (TRUE != _hwPowerDown(VCAMIO)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
			
				
/**********HI553 POWEROFF  OK***********/
		} else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_HI553_MIPI_RAW, currSensorName))) {

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);

				ISP_MCLK2_EN(0);
#if 0				
				/* AF_VCC */
				if (TRUE != _hwPowerDown(VCAMAF)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %d\n",VCAMAF);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
#endif				
				/****SUB_DVDD****/
				if (TRUE != _hwPowerDown(SUB_VCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n",SUB_VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}
				
				/* VCAM_A */
				if (TRUE != _hwPowerDown(VCAMA)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}

				/* VCAM_IO */
				if (TRUE != _hwPowerDown(VCAMIO)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
				
/*********  OV8856_HUAQUN POWEROFF  ***********/
		} else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV8856_HUAQUAN_MIPI_RAW, currSensorName))) {
				/* Set Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				
				/* Set Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
				
				/* AF_VCC */
				if (TRUE != _hwPowerDown(VCAMAF)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF),power id = %d\n",VCAMAF);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}

				if (TRUE != _hwPowerDown(VCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}

				/* VCAM_A */
				if (TRUE != _hwPowerDown(VCAMA)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
				
				/* VCAM_IO */
				if (TRUE != _hwPowerDown(VCAMIO)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
			
				ISP_MCLK1_EN(0);
				
/*********  OV8856_QUNHUI POWEROFF ***********/				
		} else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_OV8856_QUNHUI_MIPI_RAW, currSensorName))) {
								/* Set Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				
				/* Set Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
				
				/* AF_VCC */
				if (TRUE != _hwPowerDown(VCAMAF)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF),power id = %d\n",VCAMAF);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}

				if (TRUE != _hwPowerDown(VCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}

				/* VCAM_A */
				if (TRUE != _hwPowerDown(VCAMA)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
				
				/* VCAM_IO */
				if (TRUE != _hwPowerDown(VCAMIO)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
			
				ISP_MCLK1_EN(0);

/*********	GC5005  POWEROFF ***********/ 
		} else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC5005_MIPI_RAW, currSensorName))) {
		

                        /* Set Power Pin low and Reset Pin HIGH */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON]);


			      /* Set Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				
				mdelay(1);
				ISP_MCLK2_EN(0);
#if 0				
				/* AF_VCC */
				if (TRUE != _hwPowerDown(VCAMAF)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF),power id = %d\n",VCAMAF);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
#endif

                        /* VCAM_A */
				if (TRUE != _hwPowerDown(VCAMA)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
				
				if (TRUE != _hwPowerDown(SUB_VCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}
			
				/* VCAM_IO */
				if (TRUE != _hwPowerDown(VCAMIO)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
								
                         mdelay(4);//AVDD power off to 0 need 4ms
                        
                        /* Set Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
					
	}
	else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_GC5025_MIPI_RAW, currSensorName))){
	//First Power Pin low and Reset Pin High
	            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
		            mtkcam_gpio_set(pinSetIdx, CAMPDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
					
	            mdelay(1);

	            if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
		            mtkcam_gpio_set(pinSetIdx, CAMRST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
					
	            mdelay(1);
				
	            ISP_MCLK2_EN(0);
				
	            mdelay(1);
				
	            //VCAM_A
	            if(TRUE != _hwPowerDown(VCAMA))
	            {
		            PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id = %d\n", VCAMA);
		            goto _kdCISModulePowerOn_exit_;
	            }
				
	            mdelay(1);

	            //DVDD
	            if(TRUE != _hwPowerDown(SUB_VCAMD))
	            {
		            PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (SUB_VCAMD), power id = %d \n", SUB_VCAMD);
		            goto _kdCISModulePowerOn_exit_;
	            }
				
	            mdelay(1);

	            //VCAM_IO
	            if(TRUE != _hwPowerDown(VCAMIO ))
	            {
	             	PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAMIO), power id = %d \n", VCAMIO);
		            goto _kdCISModulePowerOn_exit_;
	            }

	            mdelay(1);		
/**********  DEFAULT POWEROFF  **************/
		} 
        else if (currSensorName && (0 == strcmp(SENSOR_DRVNAME_S5K4H8_MIPI_RAW, currSensorName)))
        {
	            ISP_MCLK1_EN(0);

            	mdelay(2);

	            //Set Power Pin low and Reset Pin Low
            	if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
	            	mtkcam_gpio_set(pinSetIdx, CAMPDN, pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);

            	//Set Reset Pin Low
            	if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
	            	mtkcam_gpio_set(pinSetIdx, CAMRST, pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);

            	mdelay(1);

            	if (TRUE != _hwPowerDown(VCAMD)) {
	        	    PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %d \n",VCAMD);
		            goto _kdCISModulePowerOn_exit_;
	            }

            	//VCAM_A
             	if (TRUE != _hwPowerDown(VCAMA)) {
	            	PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= (%d) \n", VCAMA);
	            	goto _kdCISModulePowerOn_exit_;
            	}

            	//VCAM_IO
            	if (TRUE != _hwPowerDown(VCAMIO)) {
	            	PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %d \n", VCAMIO);
	            	goto _kdCISModulePowerOn_exit_;
             	}

            	//AF_VCC
	            if (TRUE != _hwPowerDown(VCAMAF)) {
	            	PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %d \n", VCAMAF);
	            	goto _kdCISModulePowerOn_exit_;
	            }
       }		
	   else {
				/* Set Power Pin low and Reset Pin Low */
				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN])
					mtkcam_gpio_set(pinSetIdx, CAMPDN,pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF]);
				

				if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST])
					mtkcam_gpio_set(pinSetIdx, CAMRST,pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF]);
				


				if (TRUE != _hwPowerDown(VCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n",VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}

				/* VCAM_A */
				if (TRUE != _hwPowerDown(VCAMA)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}

				/* VCAM_IO */
				if (TRUE != _hwPowerDown(VCAMIO)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %d\n",VCAMIO);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}

				/* AF_VCC */
				if (TRUE != _hwPowerDown(VCAMAF)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF),power id = %d\n",VCAMAF);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}

		}

	}

	return 0;

_kdCISModulePowerOn_exit_:
	return -EIO;

}


#else
/*
#ifndef BOOL
typedef unsigned char BOOL;
#endif
*/

struct regulator *regVCAMA = NULL;
struct regulator *regVCAMD = NULL;
struct regulator *regVCAMIO = NULL;
struct regulator *regVCAMAF = NULL;
struct regulator *regVCAMD_SUB = NULL;




/* extern struct platform_device *camerahw_platform_device; */

bool _hwPowerOn(char *powerId, int powerVolt, struct regulator **regVCAM)
{
	bool ret = false;
	struct regulator *temp = NULL;

	PK_DBG("[_hwPowerOn]before get, powerId:%s, regVCAM: %p\n", powerId, *regVCAM);
	if (*regVCAM == NULL)
		*regVCAM = regulator_get(sensor_device, powerId);
	temp = *regVCAM;

	PK_DBG("[_hwPowerOn]after get, powerId:%s, regVCAM: %p\n", powerId, temp);


	if (!IS_ERR(temp)) {
		if (powerId != CAMERA_POWER_VCAM_IO
		    && regulator_set_voltage(temp, powerVolt, powerVolt) != 0) {
			PK_DBG
			    ("[_hwPowerOn]fail to regulator_set_voltage, powerVolt:%d, powerID: %s\n",
			     powerVolt, powerId);
		}
		if (regulator_enable(temp) != 0) {
			PK_DBG("[_hwPowerOn]fail to regulator_enable, powerVolt:%d, powerID: %s\n",
			       powerVolt, powerId);
			/* regulator_put(regVCAM); */
			/* regVCAM = NULL; */
			return ret;
		}
		ret = true;
	} else
		PK_DBG("[_hwPowerOn]IS_ERR_OR_NULL regVCAM %s\n", powerId);

	return ret;
}

bool _hwPowerDown(char *powerId, struct regulator **regVCAM)
{
	bool ret = false;
	struct regulator *temp = *regVCAM;

	if (!IS_ERR(temp)) {
#if 1
		if (regulator_is_enabled(temp)) {
			PK_DBG("[_hwPowerDown]before disable %s is enabled\n", powerId);
		}
#endif
		if (regulator_disable(temp) != 0)
			PK_DBG("[_hwPowerDown]fail to regulator_disable, powerID: %s\n", powerId);
		/* for SMT stage, put seems always fail? */
		/* regulator_put(regVCAM); */
		/* regVCAM = NULL; */
		ret = true;
	} else {
		PK_DBG("[_hwPowerDown]%s fail to power down  due to regVCAM == NULL regVCAM 0x%p\n",
		       powerId, temp);
	}
	return ret;
}


int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On,
		       char *mode_name)
{

	u32 pinSetIdx = 0;	/* default main sensor */

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4
#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3

#define VOL_2800 2800000
#define VOL_1800 1800000
#define VOL_1500 1500000
#define VOL_1200 1200000
#define VOL_1000 1000000


	u32 pinSet[3][8] = {
		/* for main sensor */
		{CAMERA_CMRST_PIN,	/* The reset pin of main sensor uses GPIO10 of mt6306, please call mt6306 API to set */
		 CAMERA_CMRST_PIN_M_GPIO,	/* mode */
		 GPIO_OUT_ONE,	/* ON state */
		 GPIO_OUT_ZERO,	/* OFF state */
		 CAMERA_CMPDN_PIN,
		 CAMERA_CMPDN_PIN_M_GPIO,
		 GPIO_OUT_ONE,
		 GPIO_OUT_ZERO,
		 },
		/* for sub sensor */
		{CAMERA_CMRST1_PIN,
		 CAMERA_CMRST1_PIN_M_GPIO,
		 GPIO_OUT_ONE,
		 GPIO_OUT_ZERO,
		 CAMERA_CMPDN1_PIN,
		 CAMERA_CMPDN1_PIN_M_GPIO,
		 GPIO_OUT_ONE,
		 GPIO_OUT_ZERO,
		 },
		/* for main_2 sensor */
		{GPIO_CAMERA_INVALID,
		 GPIO_CAMERA_INVALID,	/* mode */
		 GPIO_OUT_ONE,	/* ON state */
		 GPIO_OUT_ZERO,	/* OFF state */
		 GPIO_CAMERA_INVALID,
		 GPIO_CAMERA_INVALID,
		 GPIO_OUT_ONE,
		 GPIO_OUT_ZERO,
		 }
	};



	if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx) {
		pinSetIdx = 0;
	} else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx) {
		pinSetIdx = 1;
	} else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx) {
		pinSetIdx = 2;
	}
	/* power ON */
	if (On) {
		if (pinSetIdx == 0)
			ISP_MCLK1_EN(1);
		else
			ISP_MCLK2_EN(1);


		PK_DBG("[PowerON]pinSetIdx:%d, currSensorName: %s\n", pinSetIdx, currSensorName);

		if ((currSensorName && (0 == strcmp(currSensorName, "imx135mipiraw"))) ||
		    (currSensorName && (0 == strcmp(currSensorName, "imx220mipiraw")))) {
			/* First Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}


			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
			/* AF_VCC */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(1);

			/* VCAM_A */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(1);

			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1000, &regVCAMD)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %s\n",
				     CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(1);

			/* VCAM_IO */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(2);


			/* enable active sensor */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

		} else if (currSensorName
			   && (0 == strcmp(SENSOR_DRVNAME_OV5648_MIPI_RAW, currSensorName))) {
#if 0
			mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_MODE_00);
			mt_set_gpio_dir(GPIO_SPI_MOSI_PIN, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO_SPI_MOSI_PIN, GPIO_OUT_ONE);
#endif
			/* First Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}

			/* VCAM_IO */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(1);

			/* VCAM_A */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(1);

			if (TRUE != _hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1500, &regVCAMD)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %s\n",
				     SUB_CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);

			/* AF_VCC */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				goto _kdCISModulePowerOn_exit_;
			}


			mdelay(1);


			/* enable active sensor */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}


			mdelay(2);


			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}

			}

			mdelay(20);
		} else if (currSensorName
			   && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW, currSensorName))) {
#if 0
			mt_set_gpio_mode(GPIO_SPI_MOSI_PIN, GPIO_MODE_00);
			mt_set_gpio_dir(GPIO_SPI_MOSI_PIN, GPIO_DIR_OUT);
			mt_set_gpio_out(GPIO_SPI_MOSI_PIN, GPIO_OUT_ONE);
#endif
			/* First Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}

			mdelay(50);

			/* VCAM_A */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(10);

			/* VCAM_IO */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(10);

			if (TRUE != _hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1500, &regVCAMD)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_D), power id = %s\n",
				     SUB_CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(10);

			/* AF_VCC */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				goto _kdCISModulePowerOn_exit_;
			}


			mdelay(50);

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
				mdelay(5);
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}

			}
			mdelay(5);
			/* enable active sensor */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
				mdelay(5);
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			mdelay(5);
		} else if (currSensorName
			   && (0 == strcmp(SENSOR_DRVNAME_GC0310_YUV, currSensorName))) {
			/* First Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}

			mdelay(50);
			/* VCAM_A & VCAM_IO use same source VCAM_A */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(10);

			/* VCAM_IO */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(10);

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
			mdelay(5);
			/* enable active sensor */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				mdelay(5);
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMPDN)\n");
				}
				mdelay(5);
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMPDN)\n");
				}
				mdelay(5);
			}
			mdelay(5);
		} else if (currSensorName
			   && (0 == strcmp(SENSOR_DRVNAME_S5K4ECGX_MIPI_YUV, currSensorName))) {
			/* First Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
			/* VCAM_D */

			if (pinSetIdx == 1
			    && TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200, &regVCAMD)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}
			mdelay(5);

			/* VCAM_IO */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);


			/* AF_VCC */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);

			/* enable active sensor */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			mdelay(1);

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
			mdelay(5);
		} else {
			/* First Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
			/* VCAM_IO */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_IO, VOL_1800, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_A, VOL_2800, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %s\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_D */
			if (currSensorName
			    && (0 == strcmp(SENSOR_DRVNAME_S5K2P8_MIPI_RAW, currSensorName))) {
				if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200, &regVCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
					goto _kdCISModulePowerOn_exit_;
				}
			} else if (currSensorName
				   && (0 == strcmp(SENSOR_DRVNAME_IMX179_MIPI_RAW, currSensorName)
				       || 0 == strcmp(SENSOR_DRVNAME_OV5670_MIPI_RAW,
						      currSensorName))) {
				if (pinSetIdx == 0
				    && TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,
							  &regVCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
					goto _kdCISModulePowerOn_exit_;
				} else if (pinSetIdx == 1
					   && TRUE != _hwPowerOn(SUB_CAMERA_POWER_VCAM_D, VOL_1200,
								 &regVCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
					goto _kdCISModulePowerOn_exit_;
				}
			} else if (currSensorName
				   && (0 ==
				       strcmp(SENSOR_DRVNAME_IMX219_MIPI_RAW, currSensorName))) {
				if (pinSetIdx == 0
				    && TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1200,
							  &regVCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
					goto _kdCISModulePowerOn_exit_;
				}
			} else {	/* Main VCAMD max 1.5V */
				PK_DBG("[CAMERA SENSOR] before vcamd power on\n");
				if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_D, VOL_1500, &regVCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable digital power\n");
					goto _kdCISModulePowerOn_exit_;
				}
				PK_DBG("[CAMERA SENSOR] after before vcamd power on regVCAMD=%p\n",
				       &regVCAMD);
			}


			/* AF_VCC */
			if (TRUE != _hwPowerOn(CAMERA_POWER_VCAM_AF, VOL_2800, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to enable analog power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);

			/* enable active sensor */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			mdelay(1);

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_ON])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
		}
	} else {		/* power OFF */

		PK_DBG("[PowerOFF]pinSetIdx:%d\n", pinSetIdx);
		if (pinSetIdx == 0)
			ISP_MCLK1_EN(0);
		else
			ISP_MCLK2_EN(0);

		if ((currSensorName && (0 == strcmp(currSensorName, "imx135mipiraw"))) ||
		    (currSensorName && (0 == strcmp(currSensorName, "imx220mipiraw")))) {


			/* Set Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}
			/* Set Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
			/* AF_VCC */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_AF, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}

			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D, &regVCAMD)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %s\n",
				     CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s\n",
				     CAMERA_POWER_VCAM_A);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}

		} else if (currSensorName
			   && (0 == strcmp(SENSOR_DRVNAME_OV5648_MIPI_RAW, currSensorName))) {
#if 0
			mt_set_gpio_out(GPIO_SPI_MOSI_PIN, GPIO_OUT_ZERO);
#endif
			/* Set Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}

			if (TRUE != _hwPowerDown(SUB_CAMERA_POWER_VCAM_D, &regVCAMD)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %s\n",
				     SUB_CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s\n",
				     CAMERA_POWER_VCAM_A);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* AF_VCC */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_AF, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}

		} else if (currSensorName
			   && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW, currSensorName))) {
#if 0
			mt_set_gpio_out(GPIO_SPI_MOSI_PIN, GPIO_OUT_ZERO);
#endif
			/* Set Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}

			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}


			if (TRUE != _hwPowerDown(SUB_CAMERA_POWER_VCAM_D, &regVCAMD)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %s\n",
				     SUB_CAMERA_POWER_VCAM_D);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_A */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s\n",
				     CAMERA_POWER_VCAM_A);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			/* AF_VCC */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_AF, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}

		} else if (currSensorName
			   && (0 == strcmp(SENSOR_DRVNAME_GC0310_YUV, currSensorName))) {
			/* Set Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_ON])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
			/* VCAM_A */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			}
		} else {
			/* Set Power Pin low and Reset Pin Low */
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMPDN]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_MODE])) {
					PK_DBG("[CAMERA LENS] set gpio mode failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMPDN], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA LENS] set gpio dir failed!! (CMPDN)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMPDN],
				     pinSet[pinSetIdx][IDX_PS_CMPDN + IDX_PS_OFF])) {
					PK_DBG("[CAMERA LENS] set gpio failed!! (CMPDN)\n");
				}
			}
			if (GPIO_CAMERA_INVALID != pinSet[pinSetIdx][IDX_PS_CMRST]) {
				if (mt_set_gpio_mode
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_MODE])) {
					PK_DBG("[CAMERA SENSOR] set gpio mode failed!! (CMRST)\n");
				}
				if (mt_set_gpio_dir(pinSet[pinSetIdx][IDX_PS_CMRST], GPIO_DIR_OUT)) {
					PK_DBG("[CAMERA SENSOR] set gpio dir failed!! (CMRST)\n");
				}
				if (mt_set_gpio_out
				    (pinSet[pinSetIdx][IDX_PS_CMRST],
				     pinSet[pinSetIdx][IDX_PS_CMRST + IDX_PS_OFF])) {
					PK_DBG("[CAMERA SENSOR] set gpio failed!! (CMRST)\n");
				}
			}
			if (currSensorName
			    && (0 == strcmp(SENSOR_DRVNAME_IMX219_MIPI_RAW, currSensorName))) {
				if (pinSetIdx == 0
				    && TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D, &regVCAMD)) {
					PK_DBG
					    ("[CAMERA SENSOR] main imx220 Fail to OFF core power (VCAM_D), power id = %s\n",
					     CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
			} else if (currSensorName
				   && (0 == strcmp(SENSOR_DRVNAME_IMX179_MIPI_RAW, currSensorName)
				       || 0 == strcmp(SENSOR_DRVNAME_OV5670_MIPI_RAW,
						      currSensorName))) {
				if (pinSetIdx == 0
				    && TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D, &regVCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
					goto _kdCISModulePowerOn_exit_;
				} else if (pinSetIdx == 1
					   && TRUE != _hwPowerDown(SUB_CAMERA_POWER_VCAM_D,
								   &regVCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power\n");
					goto _kdCISModulePowerOn_exit_;
				}
			} else if (currSensorName
				   && (0 ==
				       strcmp(SENSOR_DRVNAME_S5K4ECGX_MIPI_YUV, currSensorName))) {
				if (pinSetIdx == 1
				    && TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D, &regVCAMD)) {
					PK_DBG
					    ("[CAMERA SENSOR] sub imx219 Fail to OFF core power (VCAM_D), power id = %s\n",
					     CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
			} else {
				if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_D, &regVCAMD)) {
					PK_DBG
					    ("[CAMERA SENSOR] Fail to OFF core power (VCAM_D), power id = %s\n",
					     CAMERA_POWER_VCAM_D);
					goto _kdCISModulePowerOn_exit_;
				}
			}
			/* VCAM_A */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_A, &regVCAMA)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A), power id= %s\n",
				     CAMERA_POWER_VCAM_A);
				goto _kdCISModulePowerOn_exit_;
			}
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_IO, &regVCAMIO)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO), power id = %s\n",
				     CAMERA_POWER_VCAM_IO);
				goto _kdCISModulePowerOn_exit_;
			}
			/* AF_VCC */
			if (TRUE != _hwPowerDown(CAMERA_POWER_VCAM_AF, &regVCAMAF)) {
				PK_DBG
				    ("[CAMERA SENSOR] Fail to OFF AF power (VCAM_AF), power id = %s\n",
				     CAMERA_POWER_VCAM_AF);
				goto _kdCISModulePowerOn_exit_;
			}
		}

	}

	return 0;

_kdCISModulePowerOn_exit_:
	return -EIO;

}

#endif



EXPORT_SYMBOL(kdCISModulePowerOn);

/* !-- */
/*  */
