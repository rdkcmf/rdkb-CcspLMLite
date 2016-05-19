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

#ifndef  NETWORK_DEVICES_STATUS_H
#define  NETWORK_DEVICES_STATUS_H

#include "ansc_platform.h"

/**
 * @brief Set the Harvesting Status for Network Devices.
 *
 * @param[in] status New Harvesting Status.
 * @return status 0 for success and 1 for failure
 */
int SetNDSHarvestingStatus(BOOL status);

/**
 * @brief Gets the Harvesting Status for Network Devices.
 *
 * @return status true if enabled and false if disabled
 */
BOOL GetNDSHarvestingStatus();

/**
 * @brief Set the Reporting Period for Network Devices Scan.
 *
 * @param[in] period Time in Seconds.
 * @return status 0 for success and 1 for failure
 */
int SetNDSReportingPeriod(ULONG period);

/**
 * @brief Gets the Network Devices Reporting Period
 *
 * @return period : The Current Reporting Period
 */
ULONG GetNDSReportingPeriod();

/**
 * @brief Set the Polling Period for Network Devices Scan.
 *
 * @param[in] period Time in Seconds.
 * @return status 0 for success and 1 for failure
 */
int SetNDSPollingPeriod(ULONG period);

/**
 * @brief Gets the Network Devices Polling Period
 *
 * @return period : The Current Polling Period
 */
ULONG GetNDSPollingPeriod();

/**
 * @brief Gets the Default Network Devices Reporting Period
 *
 * @return period : The Current Reporting Period
 */
ULONG GetNDSReportingPeriodDefault();

/**
 * @brief Gets the Default Network Devices Polling Period
 *
 * @return period : The Current Reporting Period
 */
ULONG GetNDSPollingPeriodDefault();

/**
 * @brief Gets the Default timeout for Accelerated Scans
 *
 * @return period : The default timeout
 */
ULONG GetNDSOverrideTTLDefault();

/**
 * @brief Validated the Period Values for ND Scan and makes sure they are 
 *        present in the valid range of Values.
 *
 * @param[in] period period to be validated.
 * @return status 0 for success and 1 for failure
 */
BOOL ValidateNDSPeriod(ULONG period);

#endif 
