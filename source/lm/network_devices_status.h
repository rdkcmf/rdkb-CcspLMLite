/******************************************************************************
   Copyright [2016] [Comcast]

   Comcast Proprietary and Confidential

   All Rights Reserved.

   Unauthorized copying of this file, via any medium is strictly prohibited

******************************************************************************/

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
