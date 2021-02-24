/**
  ************************************* Copyright ******************************   
  *                 (C) Copyright 2019,CWL,China, GCU.
  *                            All Rights Reserved
  *                              
  *                     By(武汉市先讯科技有限公司)
  *                     https://www.sensertek.com
  *      
  * FileName   : hal_defs.h   
  * Version    : v1.0		
  * Author     : CWL			
  * Date       : 2019-05-07         
  * Description:    
  * Function List:  
  	1. ....
  	   <version>: 		
  <modify staff>:
  		  <data>:
   <description>:  
  	2. ...
  ******************************************************************************
 */



#ifndef __USRCONFIG_H
#define __USRCONFIG_H

#include <stdint.h>
#include "kprintf.h"

/* 用于存WIFI和密码和设备ID */ 
//#define MAGIC_NUMBER 0xAA

#define DEBUG  1

#if (DEBUG == 1)
    //以下三个定义为调试定义
    #define DebugPrintf(...)                kprintf(__VA_ARGS__)
    #define DebugPrintln(message)           do {taskENTER_CRITICAL(); Serial.println(message); taskEXIT_CRITICAL();} while (0)
    #define DebugPrint(message)             do {taskENTER_CRITICAL(); Serial.print(message); taskEXIT_CRITICAL();} while (0)
    #define DebugWrite(buf, len)            do {taskENTER_CRITICAL(); Serial.write(buf, len); taskEXIT_CRITICAL();} while (0)
#else
    #define DebugPrintf(...)                ((void)0)
    #define DebugPrintln(message)           ((void)0)
    #define DebugWrite(buf, len)            ((void)0)
#endif



#endif  //__CONFIG_H


 

