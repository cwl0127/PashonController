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

typedef void (*callback_t)();

/* 用于存WIFI和密码和设备ID */ 
//#define MAGIC_NUMBER 0xAA

#define DEBUG  1

#if (DEBUG == 1)
    //以下三个定义为调试定义
    #define DebugPrintf(...)                Serial.printf(__VA_ARGS__)
    #define DebugPrintln(message)           Serial.println(message)
    #define DebugPrint(message)             Serial.print(message)
    #define DebugWriteArray(buffer, size)   Serial.write(buffer, size)
#else
    #define DebugPrintf(...)                ((void)0)
    #define DebugPrintln(message)           ((void)0)
    #define DebugPrint(message)             ((void)0)
    #define DebugWriteArray(buffer, size)   ((void)0)
#endif



#endif  //__CONFIG_H


 

