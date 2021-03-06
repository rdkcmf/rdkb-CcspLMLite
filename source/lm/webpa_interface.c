/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#define _GNU_SOURCE
#include "ssp_global.h"
#include "stdlib.h"
#include "ccsp_dm_api.h"
#include "webpa_interface.h"
#include "ccsp_lmliteLog_wrapper.h"
#include "lm_util.h"
#include <sysevent/sysevent.h>
#include <libparodus.h>
#include "webpa_pd.h"
#include <math.h>
#include "syscfg/syscfg.h"
#include "ccsp_memory.h"

#ifdef MLT_ENABLED
#include "rpl_malloc.h"
#include "mlt_malloc.h"
#endif

#define MAX_PARAMETERNAME_LEN   512

extern ANSC_HANDLE bus_handle;
pthread_mutex_t webpa_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t device_mac_mutex = PTHREAD_MUTEX_INITIALIZER;

char deviceMAC[32]={'\0'}; 
char fullDeviceMAC[32]={'\0'};
#define ETH_WAN_STATUS_PARAM "Device.Ethernet.X_RDKCENTRAL-COM_WAN.Enabled"
#define RDKB_ETHAGENT_COMPONENT_NAME                  "com.cisco.spvtg.ccsp.ethagent"
#define RDKB_ETHAGENT_DBUS_PATH                       "/com/cisco/spvtg/ccsp/ethagent"

libpd_instance_t client_instance;
static void *handle_parodus();

static void waitForEthAgentComponentReady();
static void checkComponentHealthStatus(char * compName, char * dbusPath, char *status, int *retStatus);
static int check_ethernet_wan_status();
int s_sysevent_connect(token_t *out_se_token);

int WebpaInterface_DiscoverComponent(char** pcomponentName, char** pcomponentPath )
{
    char CrName[256] = {0};
    int ret = 0;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    CrName[0] = 0;
    strcpy(CrName, "eRT.");
    strcat(CrName, CCSP_DBUS_INTERFACE_CR);

    componentStruct_t **components = NULL;
    int compNum = 0;
    int res = CcspBaseIf_discComponentSupportingNamespace (
            bus_handle,
            CrName,
#ifndef _XF3_PRODUCT_REQ_
            "Device.X_CISCO_COM_CableModem.MACAddress",
#else
            "Device.DPoE.Mac_address",
#endif      
            "",
            &components,
            &compNum);
    if(res != CCSP_SUCCESS || compNum < 1){
        CcspTraceError(("WebpaInterface_DiscoverComponent find eRT PAM component error %d\n", res));
        ret = -1;
    }
    else{
        *pcomponentName = LanManager_CloneString(components[0]->componentName);
        *pcomponentPath = LanManager_CloneString(components[0]->dbusPath);
        CcspTraceInfo(("WebpaInterface_DiscoverComponent find eRT PAM component %s--%s\n", *pcomponentName, *pcomponentPath));
    }
    free_componentStruct_t(bus_handle, compNum, components);
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return ret;
}

void sendWebpaMsg(char *serviceName, char *dest, char *trans_id, char *contentType, char *payload, unsigned int payload_len)
{
    pthread_mutex_lock(&webpa_mutex);
    wrp_msg_t *wrp_msg ;
    int retry_count = 0, backoffRetryTime = 0, c = 2;
    int sendStatus = -1;
    char source[MAX_PARAMETERNAME_LEN/2] = {'\0'};

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, <======== Start of sendWebpaMsg =======>\n"));
	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, deviceMAC *********:%s\n",deviceMAC));
    if(serviceName!= NULL){
    	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, serviceName :%s\n",serviceName));
		snprintf(source, sizeof(source), "mac:%s/%s", deviceMAC, serviceName);
	}
	if(dest!= NULL){
    	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, dest :%s\n",dest));
	}
	if(trans_id!= NULL){
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, transaction_id :%s\n",trans_id));
	}
	if(contentType!= NULL){
	    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, contentType :%s\n",contentType));
    }
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, payload_len :%d\n",payload_len));

    

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Received DeviceMac from Atom side: %s\n",deviceMAC));
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Source derived is %s\n", source));
    
    wrp_msg = (wrp_msg_t *)malloc(sizeof(wrp_msg_t));
    

    if(wrp_msg != NULL)
    {	
		memset(wrp_msg, 0, sizeof(wrp_msg_t));
        wrp_msg->msg_type = WRP_MSG_TYPE__EVENT;
        wrp_msg->u.event.payload = (void *)payload;
        wrp_msg->u.event.payload_size = payload_len;
        wrp_msg->u.event.source = source;
        wrp_msg->u.event.dest = dest;
        wrp_msg->u.event.content_type = contentType;

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->msg_type :%d\n",wrp_msg->msg_type));
        if(wrp_msg->u.event.payload!=NULL) 
        	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.payload :%s\n",(char *)(wrp_msg->u.event.payload)));
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.payload_size :%d\n",wrp_msg->u.event.payload_size));
		if(wrp_msg->u.event.source != NULL)
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.source :%s\n",wrp_msg->u.event.source));
		if(wrp_msg->u.event.dest!=NULL)
        	CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.dest :%s\n",wrp_msg->u.event.dest));
		if(wrp_msg->u.event.content_type!=NULL)
	        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, wrp_msg->u.event.content_type :%s\n",wrp_msg->u.event.content_type));

        while(retry_count<=5)
        {
            backoffRetryTime = (int) pow(2, c) -1;

            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, retry_count : %d\n",retry_count));
            sendStatus = libparodus_send(client_instance, wrp_msg);
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, sendStatus is %d\n",sendStatus));
            if(sendStatus == 0)
            {
                retry_count = 0;
                CcspTraceInfo(("Sent message successfully to parodus\n"));
                break;
            }
            else
            {
                CcspTraceError(("Failed to send message: '%s', retrying ....\n",libparodus_strerror(sendStatus)));
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, backoffRetryTime %d seconds\n", backoffRetryTime));
                sleep(backoffRetryTime);
                c++;
                retry_count++;
            }
        }

        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before freeing wrp_msg\n"));
        free(wrp_msg);
        CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, After freeing wrp_msg\n"));
    }

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG,  <======== End of sendWebpaMsg =======>\n"));

    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    pthread_mutex_unlock(&webpa_mutex);
}

void initparodusTask()
{
	int err = 0;
	pthread_t parodusThreadId;
	
	err = pthread_create(&parodusThreadId, NULL, handle_parodus, NULL);
	if (err != 0) 
	{
		CcspLMLiteConsoleTrace(("RDK_LOG_ERROR, Error creating messages thread :[%s]\n", strerror(err)));
	}
	else
	{
		CcspLMLiteConsoleTrace(("RDK_LOG_INFO, handle_parodus thread created Successfully\n"));
	}
}

static void *handle_parodus()
{
    int backoffRetryTime = 0;
    int backoff_max_time = 9;
    int max_retry_sleep;
    //Retry Backoff count shall start at c=2 & calculate 2^c - 1.
    int c =2;
    char *parodus_url = NULL;

    CcspLMLiteConsoleTrace(("RDK_LOG_INFO, ******** Start of handle_parodus ********\n"));

    pthread_detach(pthread_self());

    max_retry_sleep = (int) pow(2, backoff_max_time) -1;
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, max_retry_sleep is %d\n", max_retry_sleep ));

        get_parodus_url(&parodus_url);
	if(parodus_url != NULL)
	{
	
		libpd_cfg_t cfg1 = {.service_name = "lmlite",
						.receive = false, .keepalive_timeout_secs = 0,
						.parodus_url = parodus_url,
						.client_url = NULL
					   };
		            
		CcspLMLiteConsoleTrace(("RDK_LOG_INFO, Configurations => service_name : %s parodus_url : %s client_url : %s\n", cfg1.service_name, cfg1.parodus_url, cfg1.client_url ));

		CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Call parodus library init api \n"));

		while(1)
		{
		    if(backoffRetryTime < max_retry_sleep)
		    {
		        backoffRetryTime = (int) pow(2, c) -1;
		    }

		    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, New backoffRetryTime value calculated as %d seconds\n", backoffRetryTime));
		    int ret =libparodus_init (&client_instance, &cfg1);
		    CcspLMLiteConsoleTrace(("RDK_LOG_INFO, ret is %d\n",ret));
		    if(ret ==0)
		    {
		        CcspTraceWarning(("LMLite: Init for parodus Success..!!\n"));
		        break;
		    }
		    else
		    {
		        CcspTraceError(("LMLite: Init for parodus (url %s) failed: '%s'\n", parodus_url, libparodus_strerror(ret)));
                        /*REVIST CID:67436 Logically dead code- parodus_url cant be NULL*/
			get_parodus_url(&parodus_url);
			cfg1.parodus_url = parodus_url;
			sleep(backoffRetryTime);
		        c++;
		    }
		libparodus_shutdown(client_instance);
		   
		}
	}
    return 0;
}

const char *rdk_logger_module_fetch(void)
{
    return "LOG.RDK.LM";
}

char * getFullDeviceMac()
{
    if(strlen(fullDeviceMAC) == 0)
    {
        getDeviceMac();
    }

    return fullDeviceMAC;
}

static void waitForEthAgentComponentReady()
{
    char status[32] = {'\0'};
    int count = 0;
    int ret = -1;
    while(1)
    {
        checkComponentHealthStatus(RDKB_ETHAGENT_COMPONENT_NAME, RDKB_ETHAGENT_DBUS_PATH, status,&ret);
        if(ret == CCSP_SUCCESS && (strcmp(status, "Green") == 0))
        {
            CcspTraceInfo(("%s component health is %s, continue\n", RDKB_ETHAGENT_COMPONENT_NAME, status));
            break;
        }
        else
        {
            count++;
            if(count > 60)
            {
                CcspTraceError(("%s component Health check failed (ret:%d), continue\n",RDKB_ETHAGENT_COMPONENT_NAME, ret));
                break;
            }
            if(count%5 == 0)
            {
                CcspTraceError(("%s component Health, ret:%d, waiting\n", RDKB_ETHAGENT_COMPONENT_NAME, ret));
            }
            sleep(5);
        }
    }
}

static void checkComponentHealthStatus(char * compName, char * dbusPath, char *status, int *retStatus)
{
	int ret = 0, val_size = 0;
	parameterValStruct_t **parameterval = NULL;
	char *parameterNames[1] = {};
	char tmp[MAX_PARAMETERNAME_LEN];
	char str[MAX_PARAMETERNAME_LEN/2];     

	sprintf(tmp,"%s.%s",compName, "Health");
	parameterNames[0] = tmp;

	snprintf(str, sizeof(str), "eRT.%s", compName);
	CcspTraceDebug(("str is:%s\n", str));

	ret = CcspBaseIf_getParameterValues(bus_handle, str, dbusPath,  parameterNames, 1, &val_size, &parameterval);
	CcspTraceDebug(("ret = %d val_size = %d\n",ret,val_size));
	if(ret == CCSP_SUCCESS)
	{
		CcspTraceDebug(("parameterval[0]->parameterName : %s parameterval[0]->parameterValue : %s\n",parameterval[0]->parameterName,parameterval[0]->parameterValue));
		strcpy(status, parameterval[0]->parameterValue);
		CcspTraceDebug(("status of component:%s\n", status));
	}
	free_parameterValStruct_t (bus_handle, val_size, parameterval);

	*retStatus = ret;
}

static int check_ethernet_wan_status()
{
    int ret = -1, size =0, val_size =0;
    char compName[MAX_PARAMETERNAME_LEN/2] = { '\0' };
    char dbusPath[MAX_PARAMETERNAME_LEN/2] = { '\0' };
    parameterValStruct_t **parameterval = NULL;
    char *getList[] = {ETH_WAN_STATUS_PARAM};
    componentStruct_t **        ppComponents = NULL;
    char dst_pathname_cr[256] = {0};
    char isEthEnabled[64]={'\0'};
    
    if(0 == syscfg_init())
    {
        if( 0 == syscfg_get( NULL, "eth_wan_enabled", isEthEnabled, sizeof(isEthEnabled)) && (isEthEnabled[0] != '\0' && strncmp(isEthEnabled, "true", strlen("true")) == 0))
        {
            CcspTraceInfo(("Ethernet WAN is enabled\n"));
            ret = CCSP_SUCCESS;
        }
    }
    else
    {
        waitForEthAgentComponentReady();
        sprintf(dst_pathname_cr, "%s%s", "eRT.", CCSP_DBUS_INTERFACE_CR);
        ret = CcspBaseIf_discComponentSupportingNamespace(bus_handle, dst_pathname_cr, ETH_WAN_STATUS_PARAM, "", &ppComponents, &size);
        if ( ret == CCSP_SUCCESS && size >= 1)
        {
            strncpy(compName, ppComponents[0]->componentName, sizeof(compName)-1);
            strncpy(dbusPath, ppComponents[0]->dbusPath, sizeof(compName)-1);
        }
        else
        {
            CcspTraceError(("Failed to get component for %s ret: %d\n",ETH_WAN_STATUS_PARAM,ret));
        }
        free_componentStruct_t(bus_handle, size, ppComponents);

        if(strlen(compName) != 0 && strlen(dbusPath) != 0)
        {
            ret = CcspBaseIf_getParameterValues(bus_handle, compName, dbusPath, getList, 1, &val_size, &parameterval);
            if(ret == CCSP_SUCCESS && val_size > 0)
            {
                if(parameterval[0]->parameterValue != NULL && strncmp(parameterval[0]->parameterValue, "true", strlen("true")) == 0)
                {
                    CcspTraceInfo(("Ethernet WAN is enabled\n"));
                    ret = CCSP_SUCCESS;
                }
                else
                {
                    CcspTraceInfo(("Ethernet WAN is disabled\n"));
                    ret = CCSP_FAILURE;
                }
            }
            else
            {
                CcspTraceError(("Failed to get values for %s ret: %d\n",getList[0],ret));
            }
            free_parameterValStruct_t(bus_handle, val_size, parameterval);
        }
    }
    return ret;
}

char * getDeviceMac()
{
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s ENTER\n", __FUNCTION__ ));

#if defined(_PLATFORM_RASPBERRYPI_)
    // Return without performing any operation as RPi Platform don't have Cable Modem and execution
    // of this function on RPI puts calling thread in infinite wait.
    return deviceMAC;
#endif

    while(!strlen(deviceMAC))
    {
        pthread_mutex_lock(&device_mac_mutex);
        int ret = -1, val_size =0,cnt =0, fd = 0;
        char *pcomponentName = NULL, *pcomponentPath = NULL;
        parameterValStruct_t **parameterval = NULL;
        token_t  token;
        char deviceMACValue[32] = { '\0' };
#ifndef _XF3_PRODUCT_REQ_
        char *getList[] = {"Device.X_CISCO_COM_CableModem.MACAddress"};
#else
        char *getList[] = {"Device.DPoE.Mac_address"};
#endif
        
        if (strlen(deviceMAC))
        {
            pthread_mutex_unlock(&device_mac_mutex);
            break;
        }

        fd = s_sysevent_connect(&token);
        if(CCSP_SUCCESS == check_ethernet_wan_status() && sysevent_get(fd, token, "eth_wan_mac", deviceMACValue, sizeof(deviceMACValue)) == 0 && deviceMACValue[0] != '\0')
        {
            strcpy(fullDeviceMAC, deviceMACValue);
            AnscMacToLower(deviceMAC, deviceMACValue, sizeof(deviceMAC));
            CcspTraceInfo(("deviceMAC is %s\n", deviceMAC));
        }
        else
        {
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before WebpaInterface_DiscoverComponent ret: %d\n",ret));

            if(pcomponentPath == NULL || pcomponentName == NULL)
            {
                if(-1 == WebpaInterface_DiscoverComponent(&pcomponentName, &pcomponentPath)){
                    CcspTraceError(("%s ComponentPath or pcomponentName is NULL\n", __FUNCTION__));
            		pthread_mutex_unlock(&device_mac_mutex);
                    return NULL;
                }
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, WebpaInterface_DiscoverComponent ret: %d  ComponentPath %s ComponentName %s \n",ret, pcomponentPath, pcomponentName));
            }

            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before GPV ret: %d\n",ret));
            ret = CcspBaseIf_getParameterValues(bus_handle,
                        pcomponentName, pcomponentPath,
                        getList,
                        1, &val_size, &parameterval);
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, After GPV ret: %d\n",ret));
            if(ret == CCSP_SUCCESS)
            {
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, val_size : %d\n",val_size));
                for (cnt = 0; cnt < val_size; cnt++)
                {
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->parameterName : %s\n",cnt,parameterval[cnt]->parameterName));
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->parameterValue : %s\n",cnt,parameterval[cnt]->parameterValue));
                    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, parameterval[%d]->type :%d\n",cnt,parameterval[cnt]->type));
                
                }
                CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Calling macToLower to get deviceMacId\n"));
                strcpy(fullDeviceMAC, parameterval[0]->parameterValue);
                AnscMacToLower(deviceMAC, parameterval[0]->parameterValue, sizeof(deviceMAC));
                if(pcomponentName)
                {
                    AnscFreeMemory(pcomponentName);
                }
                if(pcomponentPath)
                {
                    AnscFreeMemory(pcomponentPath);
                }

            }
            else
            {
                CcspLMLiteTrace(("RDK_LOG_ERROR, Failed to get values for %s ret: %d\n",getList[0],ret));
                CcspTraceError(("RDK_LOG_ERROR, Failed to get values for %s ret: %d\n",getList[0],ret));
                sleep(10);
            }
         
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, Before free_parameterValStruct_t...\n"));
            free_parameterValStruct_t(bus_handle, val_size, parameterval);
            CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, After free_parameterValStruct_t...\n"));
        }   
        pthread_mutex_unlock(&device_mac_mutex);
    
    }
        
    CcspLMLiteConsoleTrace(("RDK_LOG_DEBUG, LMLite %s EXIT\n", __FUNCTION__ ));

    return deviceMAC;
}
