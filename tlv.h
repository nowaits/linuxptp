/**
 * @file tlv.h
 * @brief Implements helper routines for processing Type Length Value fields.
 * @note Copyright (C) 2012 Richard Cochran <richardcochran@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef HAVE_TLV_H
#define HAVE_TLV_H

#include <sys/queue.h>

#include "ddt.h"
#include "ds.h"

/* TLV types */
// 1588-2008 14.1.1 and 1588-2019 14.1.1
#define foreach_tlv_type \
    _(MANAGEMENT, 0x0001) \
    _(MANAGEMENT_ERROR_STATUS, 0x0002) \
    _(ORGANIZATION_EXTENSION, 0x0003) \
    _(REQUEST_UNICAST_TRANSMISSION, 0x0004) \
    _(GRANT_UNICAST_TRANSMISSION, 0x0005) \
    _(CANCEL_UNICAST_TRANSMISSION, 0x0006) \
    _(ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION, 0x0007) \
    _(PATH_TRACE, 0x0008) \
    _(ALTERNATE_TIME_OFFSET_INDICATOR, 0x0009) \
    /*1588-2008 TLV types*/ \
    _(AUTHENTICATION_2008, 0x2000) \
    _(AUTHENTICATION_CHALLENGE, 0x2001) \
    _(SECURITY_ASSOCIATION_UPDATE, 0x2002) \
    _(CUM_FREQ_SCALE_FACTOR_OFFSET, 0x2003) \
    /*Experimental values*/ \
    _(PTPMON_REQ, 0x21FE) \
    _(PTPMON_RESP, 0x21FF) \
    /*1588-2019 TLV types*/ \
    _(ORGANIZATION_EXTENSION_PROPAGATE, 0x4000) \
    _(ENHANCED_ACCURACY_METRICS, 0x4001) \
    _(ORGANIZATION_EXTENSION_DO_NOT_PROPAGATE, 0x8000) \
    _(L1_SYNC, 0x8001) \
    _(PORT_COMMUNICATION_AVAILABILITY, 0x8002) \
    _(PROTOCOL_ADDRESS, 0x8003) \
    _(SLAVE_RX_SYNC_TIMING_DATA, 0x8004) \
    _(SLAVE_RX_SYNC_COMPUTED_DATA, 0x8005) \
    _(SLAVE_TX_EVENT_TIMESTAMPS, 0x8006) \
    _(SLAVE_DELAY_TIMING_DATA_NP, 0x7F00) \
    _(CUMULATIVE_RATE_RATIO, 0x8007) \
    _(PAD, 0x8008) \
    _(AUTHENTICATION, 0x8009) \

typedef enum {
#define _(name, value) TLV_ ##name = value,
    foreach_tlv_type
#undef _
} tlv_type_t;

enum management_action {
    GET,
    SET,
    RESPONSE,
    COMMAND,
    ACKNOWLEDGE,
};

/* Clock management ID values */
// 1588-2008 15.5.2.3
// 1588-2019 15.5.2.3
#define foreach_management_id \
    _(P_NULL_MANAGEMENT, 0x0000) \
    _(P_CLOCK_DESCRIPTION, 0x0001) \
    _(C_USER_DESCRIPTION, 0x0002) \
    _(C_SAVE_IN_NON_VOLATILE_STORAGE, 0x0003) \
    _(C_RESET_NON_VOLATILE_STORAGE, 0x0004) \
    _(C_INITIALIZE, 0x0005) \
    _(C_FAULT_LOG, 0x0006) \
    _(C_FAULT_LOG_RESET , 0x0007) \
    _(C_DEFAULT_DATA_SET, 0x2000) \
    _(C_CURRENT_DATA_SET, 0x2001) \
    _(C_PARENT_DATA_SET , 0x2002) \
    _(C_TIME_PROPERTIES_DATA_SET, 0x2003) \
    _(P_PORT_DATA_SET, 0x2004) \
    _(C_PRIORITY1, 0x2005) \
    _(C_PRIORITY2, 0x2006) \
    _(C_DOMAIN, 0x2007) \
    _(C_SLAVE_ONLY, 0x2008) \
    _(P_LOG_ANNOUNCE_INTERVAL, 0x2009) \
    _(P_ANNOUNCE_RECEIPT_TIMEOUT, 0x200A) \
    _(P_LOG_SYNC_INTERVAL, 0x200B) \
    _(P_VERSION_NUMBER, 0x200C) \
    _(P_ENABLE_PORT, 0x200D) \
    _(P_DISABLE_PORT, 0x200E) \
    _(C_TIME, 0x200F) \
    _(C_CLOCK_ACCURACY, 0x2010) \
    _(C_UTC_PROPERTIES, 0x2011) \
    _(C_TRACEABILITY_PROPERTIES, 0x2012) \
    _(C_TIMESCALE_PROPERTIES, 0x2013) \
    _(P_UNICAST_NEGOTIATION_ENABLE, 0x2014) \
    _(C_PATH_TRACE_LIST, 0x2015) \
    _(C_PATH_TRACE_ENABLE, 0x2016) \
    _(C_GRANDMASTER_CLUSTER_TABLE, 0x2017) \
    _(P_UNICAST_MASTER_TABLE, 0x2018) \
    _(P_UNICAST_MASTER_MAX_TABLE_SIZE, 0x2019) \
    _(C_ACCEPTABLE_MASTER_TABLE, 0x201A) \
    _(P_ACCEPTABLE_MASTER_TABLE_ENABLED, 0x201B) \
    _(C_ACCEPTABLE_MASTER_MAX_TABLE_SIZE, 0x201C) \
    _(P_ALTERNATE_MASTER, 0x201D) \
    _(C_ALTERNATE_TIME_OFFSET_ENABLE, 0x201E) \
    _(C_ALTERNATE_TIME_OFFSET_NAME, 0x201F) \
    _(C_ALTERNATE_TIME_OFFSET_MAX_KEY, 0x2020) \
    _(C_ALTERNATE_TIME_OFFSET_PROPERTIES, 0x2021) \
    /*1588-2019 TLV types*/ \
    _(C_EXTERNAL_PORT_CONFIGURATION_ENABLED, 0x3000) \
    _(P_MASTER_ONLY, 0x3001) \
    _(C_HOLDOVER_UPGRADE_ENABLE, 0x3002) \
    _(P_EXT_PORT_CONFIG_PORT_DATA_SET, 0x3003) \
    /* END */ \
    _(C_TRANSPARENT_CLOCK_DEFAULT_DATA_SET, 0x4000) \
    _(P_TRANSPARENT_CLOCK_PORT_DATA_SET, 0x4001) \
    _(C_PRIMARY_DOMAIN, 0x4002) \
    _(P_DELAY_MECHANISM, 0x6000) \
    _(P_LOG_MIN_PDELAY_REQ_INTERVAL, 0x6001) \
    _(C_TIME_STATUS_NP, 0xC000) \
    _(C_GRANDMASTER_SETTINGS_NP, 0xC001) \
    _(P_PORT_DATA_SET_NP, 0xC002) \
    _(C_SUBSCRIBE_EVENTS_NP, 0xC003) \
    _(P_PORT_PROPERTIES_NP, 0xC004) \
    _(P_PORT_STATS_NP, 0xC005) \
    _(C_SYNCHRONIZATION_UNCERTAIN_NP, 0xC006) \
    _(P_PORT_SERVICE_STATS_NP, 0xC007) \
    _(P_UNICAST_MASTER_TABLE_NP, 0xC008) \
    _(P_PORT_HWCLOCK_NP, 0xC009) \
    _(P_POWER_PROFILE_SETTINGS_NP, 0xC00A) \
    _(P_CMLDS_INFO_NP, 0xC00B) \
    _(P_PORT_CORRECTIONS_NP, 0xC00C) \
    _(C_EXTERNAL_GRANDMASTER_PROPERTIES_NP, 0xC00D) \


typedef enum {
#define _(name, value) MID_ ##name = value,
    foreach_management_id
#undef _
} mid_type_t;

// 1588-2008 15.5.4.1.4
// 1588-2019 15.5.4.4
/* Management error ID values */
#define foreach_management_error_id \
    _(E_RESPONSE_TOO_BIG, 0x0001) \
    _(E_NO_SUCH_ID, 0x0002) \
    _(E_WRONG_LENGTH, 0x0003) \
    _(E_WRONG_VALUE, 0x0004) \
    _(E_NOT_SETABLE, 0x0005) \
    _(E_NOT_SUPPORTED, 0x0006) \
    _(E_GENERAL_ERROR, 0xFFFE) \

typedef enum {
#define _(name, value) MID_ ##name = value,
    foreach_management_error_id
#undef _
} mid_error_t;

/* Values for the SYNCHRONIZATION_UNCERTAIN_NP management TLV */
#define SYNC_UNCERTAIN_DONTCARE 0xff
#define SYNC_UNCERTAIN_FALSE    0
#define SYNC_UNCERTAIN_TRUE 1

#define CANCEL_UNICAST_MAINTAIN_REQUEST (1 << 0)
#define CANCEL_UNICAST_MAINTAIN_GRANT   (1 << 1)
#define GRANT_UNICAST_RENEWAL_INVITED   (1 << 0)

/* Flags in PORT_HWCLOCK_NP */
#define PORT_HWCLOCK_VCLOCK     (1 << 0)

struct ack_cancel_unicast_xmit_tlv {
    Enumeration16   type;
    UInteger16      length;
    uint8_t         message_type_flags;
    uint8_t         reserved;
} PACKED;

struct cancel_unicast_xmit_tlv {
    Enumeration16   type;
    UInteger16      length;
    uint8_t         message_type_flags;
    uint8_t         reserved;
} PACKED;

struct grant_unicast_xmit_tlv {
    Enumeration16   type;
    UInteger16      length;
    uint8_t         message_type;
    Integer8        logInterMessagePeriod;
    UInteger32      durationField;
    uint8_t         reserved;
    uint8_t         flags;
} PACKED;

struct alternate_time_offset_indicator_tlv {
    Enumeration16   type;
    UInteger16      length;
    UInteger8       keyField;
    /* Message alignment broken by design. */
    Integer32       currentOffset;
    Integer32       jumpSeconds;
    struct {
        uint16_t   seconds_msb; /* 16 bits + */
        uint32_t   seconds_lsb; /* 32 bits = 48 bits*/
    } PACKED timeOfNextJump;
    struct PTPText  displayName;
} PACKED;

struct alternate_time_offset_name {
    UInteger8       keyField;
    struct PTPText  displayName;
} PACKED;

struct alternate_time_offset_properties {
    UInteger8       keyField;
    /* Message alignment broken by design. */
    Integer32       currentOffset;
    Integer32       jumpSeconds;
    struct {
        uint16_t   seconds_msb; /* 16 bits + */
        uint32_t   seconds_lsb; /* 32 bits = 48 bits*/
    } PACKED timeOfNextJump;
    uint8_t pad;
} PACKED;

struct management_tlv {
    Enumeration16 type;
    UInteger16    length;
    Enumeration16 id;
    Octet         data[0];
} PACKED;

struct management_tlv_datum {
    uint8_t val;
    uint8_t reserved;
} PACKED;

struct management_error_status {
    Enumeration16 type;
    UInteger16    length;
    Enumeration16 error;
    Enumeration16 id;
    Octet         reserved[4];
    Octet         data[0];
} PACKED;

struct authentication_tlv {
    Enumeration16 type;
    UInteger16    length;
    UInteger8     spp;
    Octet         secParamIndicator;
    UInteger32    keyID;
    Octet         data[0];
} PACKED;

struct nsm_resp_tlv_head {
    Enumeration16           type;
    UInteger16              length;
    uint8_t                 port_state;
    uint8_t                 reserved;
    struct PortAddress      parent_addr;
} PACKED;

struct nsm_resp_tlv_foot {
    struct parentDS         parent;
    struct currentDS        current;
    struct timePropertiesDS timeprop;
    struct Timestamp        lastsync;
} PACKED;

/* Organizationally Unique Identifiers */
#define IEEE_802_1_COMMITTEE 0x00, 0x80, 0xC2
extern uint8_t ieee8021_id[3];
#define IEEE_C37_238_PROFILE 0x1C, 0x12, 0x9D
extern uint8_t ieeec37_238_id[3];

struct organization_tlv {
    Enumeration16 type;
    UInteger16    length;
    Octet         id[3];
    Octet         subtype[3];
} PACKED;

#define PATH_TRACE_MAX \
    ((sizeof(struct message_data) - sizeof(struct announce_msg) - sizeof(struct TLV)) / \
        sizeof(struct ClockIdentity))

struct path_trace_tlv {
    Enumeration16 type;
    UInteger16    length;
    struct ClockIdentity cid[0];
} PACKED;

static inline unsigned int path_length(struct path_trace_tlv *p)
{
    return p->length / sizeof(struct ClockIdentity);
}

struct request_unicast_xmit_tlv {
    Enumeration16   type;
    UInteger16      length;
    uint8_t         message_type;
    Integer8        logInterMessagePeriod;
    UInteger32      durationField;
} PACKED;

struct slave_delay_timing_record {
    UInteger16          sequenceId;
    struct Timestamp    delayOriginTimestamp;
    TimeInterval        totalCorrectionField;
    struct Timestamp    delayResponseTimestamp;
} PACKED;

struct slave_delay_timing_data_tlv {
    Enumeration16        type;
    UInteger16           length;
    struct PortIdentity  sourcePortIdentity;
    struct slave_delay_timing_record record[0];
} PACKED;

#define SLAVE_DELAY_TIMING_MAX \
    ((sizeof(struct message_data) - sizeof(struct signaling_msg) -  \
            sizeof(struct slave_delay_timing_data_tlv)) /     \
        sizeof(struct slave_delay_timing_record))

struct slave_rx_sync_timing_record {
    UInteger16          sequenceId;
    struct Timestamp    syncOriginTimestamp;
    TimeInterval        totalCorrectionField;
    Integer32           scaledCumulativeRateOffset;
    struct Timestamp    syncEventIngressTimestamp;
} PACKED;

struct slave_rx_sync_timing_data_tlv {
    Enumeration16        type;
    UInteger16           length;
    struct PortIdentity  sourcePortIdentity;
    struct slave_rx_sync_timing_record record[0];
} PACKED;

#define SLAVE_RX_SYNC_TIMING_MAX \
    ((sizeof(struct message_data) - sizeof(struct signaling_msg) -  \
            sizeof(struct slave_rx_sync_timing_data_tlv)) /       \
        sizeof(struct slave_rx_sync_timing_record))

typedef struct Integer96 {
    uint16_t nanoseconds_msb;
    uint64_t nanoseconds_lsb;
    uint16_t fractional_nanoseconds;
} PACKED ScaledNs;

struct cmlds_info_np {
    TimeInterval meanLinkDelay;
    Integer32    scaledNeighborRateRatio;
    uint32_t     as_capable;
} PACKED;

struct follow_up_info_tlv {
    Enumeration16 type;
    UInteger16    length;
    Octet         id[3];
    Octet         subtype[3];
    Integer32     cumulativeScaledRateOffset;
    UInteger16    gmTimeBaseIndicator;
    ScaledNs      lastGmPhaseChange;
    Integer32     scaledLastGmPhaseChange;
} PACKED;

struct ieee_c37_238_2011_tlv {
    Enumeration16 type;
    UInteger16    length;
    Octet         id[3];
    Octet         subtype[3];
    UInteger16    grandmasterID;
    UInteger32    grandmasterTimeInaccuracy;
    UInteger32    networkTimeInaccuracy;
    Octet         pad[2];
} PACKED;

struct ieee_c37_238_2017_tlv {
    Enumeration16 type;
    UInteger16    length;
    Octet         id[3];
    Octet         subtype[3];
    UInteger16    grandmasterID;
    UInteger32    reserved1;
    UInteger32    totalTimeInaccuracy;
    Octet         pad[2];
} PACKED;

struct ieee_c37_238_settings_np {
    Enumeration16 version;
    UInteger16    grandmasterID;
    UInteger32    grandmasterTimeInaccuracy;
    UInteger32    networkTimeInaccuracy;
    UInteger32    totalTimeInaccuracy;
} PACKED;

struct msg_interval_req_tlv {
    Enumeration16 type;
    UInteger16    length;
    Octet         id[3];
    Octet         subtype[3];
    Integer8      linkDelayInterval;
    Integer8      timeSyncInterval;
    Integer8      announceInterval;
    Octet         flags;
    Octet         reserved[2];
} PACKED;

struct time_status_np {
    int64_t       master_offset; /*nanoseconds*/
    int64_t       ingress_time;  /*nanoseconds*/
    Integer32     cumulativeScaledRateOffset;
    Integer32     scaledLastGmPhaseChange;
    UInteger16    gmTimeBaseIndicator;
    ScaledNs      lastGmPhaseChange;
    Integer32     gmPresent;
    struct ClockIdentity gmIdentity;
} PACKED;

struct grandmaster_settings_np {
    struct ClockQuality clockQuality;
    Integer16 utc_offset;
    UInteger8 time_flags;
    Enumeration8 time_source;
} PACKED;

struct port_ds_np {
    UInteger32    neighborPropDelayThresh; /*nanoseconds*/
    Integer32     asCapable;
} PACKED;


#define EVENT_BITMASK_CNT 64

struct subscribe_events_np {
    uint16_t      duration; /* seconds */
    uint8_t       bitmask[EVENT_BITMASK_CNT];
} PACKED;

struct port_properties_np {
    struct PortIdentity portIdentity;
    uint8_t port_state;
    uint8_t timestamping;
    struct PTPText interface;
} PACKED;

struct port_hwclock_np {
    struct PortIdentity portIdentity;
    Integer32 phc_index;
    UInteger8 flags;
    uint8_t reserved;
} PACKED;

struct port_stats_np {
    struct PortIdentity portIdentity;
    struct PortStats stats;
} PACKED;

struct port_service_stats_np {
    struct PortIdentity portIdentity;
    struct PortServiceStats stats;
} PACKED;

struct unicast_master_table_np {
    uint16_t actual_table_size;
    struct unicast_master_entry unicast_masters[0];
} PACKED;

#define PROFILE_ID_LEN 6

struct mgmt_clock_description {
    UInteger16             *clockType;
    struct PTPText         *physicalLayerProtocol;
    struct PhysicalAddress *physicalAddress;
    struct PortAddress     *protocolAddress;
    Octet                  *manufacturerIdentity;
    struct PTPText         *productDescription;
    struct PTPText         *revisionData;
    struct PTPText         *userDescription;
    Octet                  *profileIdentity;
};

struct tlv_extra {
    TAILQ_ENTRY(tlv_extra) list;
    struct TLV *tlv;
    union {
        struct mgmt_clock_description cd;
        struct nsm_resp_tlv_foot *foot;
    };
};

/* Organizationally Unique Identifiers */
#define ITU_T_COMMITTEE 0x00, 0x19, 0xA7
extern uint8_t itu_t_id[3];

struct msg_interface_rate_tlv {
    Enumeration16 type;
    UInteger16    length;
    Octet         id[3];
    Octet         subtype[3];
    UInteger64    interfaceBitPeriod;
    UInteger16    numberOfBitsBeforeTimestamp;
    UInteger16    numberOfBitsAfterTimestamp;
} PACKED;

struct external_grandmaster_properties_np {
    struct ClockIdentity gmIdentity;
    UInteger16 stepsRemoved;
} PACKED;

struct port_corrections_np {
    Integer64 egressLatency;
    Integer64 ingressLatency;
    Integer64 delayAsymmetry;
} PACKED;

/**
 * Converts recognized value sub-fields into host byte order.
 * @param extra  TLV descriptor pointing to the protocol data.
 * @return Zero if successful, otherwise non-zero
 */
int tlv_post_recv(struct tlv_extra *extra);

/**
 * Converts recognized value sub-fields into network byte order.
 * @param tlv Pointer to a Type Length Value field.
 * @param extra Additional struct containing tlv data to send, can be
 * NULL.
 */
void tlv_pre_send(struct TLV *tlv, struct tlv_extra *extra);

#endif
