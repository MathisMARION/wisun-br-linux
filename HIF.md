# RCP Hardware Interface

This document describes the Hardware Interface (HIF) for the Silicon Labs
Wi-SUN Radio Co-Processor (RCP). The RCP implements IEEE 802.15.4 frame
transmission and reception using Wi-SUN frequency hopping and this API
allows a host to control it using commands sent on a serial bus.

## General observations

In this specification, the prefix used in the command means:
  - `REQ` (request): a command sent by the host to the device
  - `SET`: special case of `REQ` to configure the device
  - `CNF` (confirmation): a reply from the device to the host
  - `IND` (indication): a spontaneous frame from the device to the host

By default, requests do not receive confirmations, the only exceptions are
[`REQ_RADIO_LIST`][rf-get] and [`REQ_DATA_TX`][tx-req].

All the types used are little endian.

The type `bool` is a `uint8_t`, but only the LSB is interpreted. All the other
bits are reserved.

When a bit is reserved or unassigned, it must be set to 0 by the transmitter
and must be ignored by the receiver.

All strings are null-terminated.

All the version numbers are encoded using `uint32_t` with the following mask:
  - `0xFF000000` Major revision
  - `0x00FFFF00` Minor revision
  - `0x000000FF` Patch

All timestamps are based on the RCP timebase, measured in microseconds, with
the origin set at the device reset.

All channel numbers are defined as if there were no channel mask applied
(_ChanN = Chan0 + N * ChanSpacing_).

## Frame structure

The RCP can use the _Native UART_ or the [_CPC_ protocol][cpc]. With CPC, the
frame structure is defined by the CPC specification.

With Native UART, frames use the following structure, and use a CRC-16 for
error detection.

 - `uint16_t len`  
    Length of the `payload` field . Only the 11 least significant bits
    (`0x07FF`) are used. The 5 most significant bits (`0xF800`) must be
    ignored.

 - `uint16_t hcs`  
    [CRC-16/MCRF4XX][hcs] of the `len` field.

 - `uint8_t payload[]`  
    Frame payload.

 - `uint16_t fcs`  
    [CRC-A][fcs] of the `payload` field.

Regardless of the framing protocol used, the payload always has the following
structure:

 - `uint8_t cmd`  
    Command number.

 - `uint8_t body[]`  
    Command body.

[cpc]: https://docs.silabs.com/gecko-platform/latest/platform-cpc-overview
[hcs]: https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-mcrf4xx
[fcs]: https://reveng.sourceforge.io/crc-catalogue/16.htm#crc.cat.crc-16-iso-iec-14443-3-a

## Administrative commands

[reset]: #0x03-req_reset

### `0x01 REQ_NOP`

No-operation. Can be used to synchronize UART communication.

 - `uint8_t garbage[]`  
    Data after command is ignored.

### `0x02 IND_NOP`

No-operation. Can be used to synchronize UART communication.

 - `uint8_t garbage[]`  
    Extra data may be included. It must be ignored.

### `0x03 REQ_RESET`

Hard reset the RCP. After this command, the host will receive `IND_RESET`.

 - `bool enter_bootloader`  
    If set to `true`, the bootloader will start instead of the RCP application.
    Use this option for flashing a new RCP firmware. For details on how to
    upload a firmware (typically using the XMODEM protocol over UART), refer to
    the bootloader documentation.

### `0x04 IND_RESET`

Sent on boot of the RCP. This command may be caused by a power on, `REQ_RESET`,
a debugger, etc...

 - `uint32_t api_version`  
    Device API version

 - `uint32_t fw_version`  
    Device version

 - `char fw_version_str[]`  
    Device version string. It may include version annotation (ie.
    `1.5.0-5-ga91c352~bpo`).

 - `uint8_t hw_eui64[8]`  
    EUI-64 flashed on the device

 - `uint8_t reserved[]`  
    Extra data may be included. For backward compatibility, it must be ignored.

### `0x05 IND_FATAL`

Commands do not send a reply upon success. If the RCP is misconfigured or
encounters a fatal error happens, it will reset. In such cases, a `IND_RESET`
message will be sent after `IND_FATAL`, providing details about the error.
This information can be used to display the error to the user for debugging
purposes.

 - `uint16_t error_code`  
    Refer to the table below for the detailed list of errors.

 - `char error_string[]`  
    Human-readable error message.

| Name                 | Value  | Description                                                 |
|----------------------|--------|-------------------------------------------------------------|
|`EBUG`                |`0x0000`| An assert was triggered, reach out Silicon Labs support.    |
|`ECRC`                |`0x0001`| A framing error was detected on the bus.                    |
|`EHIF`                |`0x0002`| A parsing error occured while processing a command.         |
|`ENOBTL`              |`0x0003`| The RCP was not compiled with a bootloader.                 |
|`ENORF`               |`0x0004`| The radio layer has not been started.                       |
|`ENOMEM`              |`0x0005`| Not enough memory available.                                |
|`EINVAL`              |`0x1000`| Invalid parameter (generic).                                |
|`EINVAL_HOSTAPI`      |`0x1001`| Incompatible host API version.                              |
|`EINVAL_PHY`          |`0x1002`| Invalid PHY selection.                                      |
|`EINVAL_TXPOW`        |`0x1003`| Invalid TX power.                                           |
|`EINVAL_REG`          |`0x1004`| Invalid regulation code ([`SET_RADIO_REGULATION`][rf-reg]). |
|`EINVAL_FHSS`         |`0x1005`| Invalid frequency hopping configuration (generic).          |
|`EINVAL_FHSS_TYPE`    |`0x1006`| Invalid FHSS type ([`REQ_DATA_TX`][tx-req]).                |
|`EINVAL_CHAN_MASK`    |`0x1007`| Invalid channel mask.                                       |
|`EINVAL_CHAN_FUNC`    |`0x1008`| Invalid channel function.                                   |
|`EINVAL_ASYNC_TXLEN`  |`0x1009`| Invalid asynchronous transmission maximum duration ([`SET_FHSS_ASYNC`][async]).|
|`EINVAL_HANDLE`       |`0x100a`| Invalid packet handle.                                      |
|`EINVAL_KEY_INDEX`    |`0x100b`| Invalid key index.                                          |
|`EINVAL_FRAME_LEN`    |`0x100c`| Invalid IEEE 802.15.4 frame length.                         |
|`EINVAL_FRAME_VERSION`|`0x100d`| Invalid IEEE 802.15.4 frame version.                        |
|`EINVAL_FRAME_TYPE`   |`0x100c`| Invalid IEEE 802.15.4 frame type.                           |
|`EINVAL_ADDR_MODE`    |`0x100e`| Invalid IEEE 802.15.4 address mode.                         |
|`EINVAL_SCF`          |`0x100f`| Invalid IEEE 802.15.4 security control field.               |
|`EINVAL_FRAME`        |`0x1010`| Invalid IEEE 802.15.4 frame (generic).                      |
|`EINVAL_CHAN_FIXED`   |`0x1011`| Invalid fixed channel.                                      |
|`ENOTSUP`             |`0x2000`| Unsupported feature (generic).                              |
|`ENOTSUP_FHSS_DEFAULT`|`0x2001`| Unsupported configuration mode for selected FHSS type ([`REQ_DATA_TX`][tx-req]).|

### `0x06 SET_HOST_API`

Informs the RCP of the host API version. If this command is not sent, the RCP
will assume that the API version is `2.0.0`. This command should be sent after
`IND_RESET` and before any other command.

 - `uint32_t api_version`  
    Must be at least `0x02000000`.

## Send and receive data

[tx-req]: #0x10-req_data_tx
[rx]:     #0x13-ind_data_rx

### `0x10 REQ_DATA_TX`

 - `uint8_t id`  
    An arbitrary ID send back in `CNF_DATA_TX`.

 - `uint16_t payload_len`  
    Length of the next field

 - `uint8_t payload[]`  
    A well formed 15.4 frame (at least, it must start with a valid 15.4 header).
    The device will retrieve the destination address, the auxiliary security
    header and other 15.4 properties from this field.

    In addition, if the following fields are present, they are automatically
    updated by the device:
      - Sequence number
      - "Unicast Fractional Sequence Interval" in the UTT-IE
      - "Broadcast Slot Number" in the BT-IE
      - "Broadcast Interval Offset" in the BT-IE
      - "LFN Broadcast Slot Number" in the LBT-IE
      - "LFN Broadcast Interval Offset" in the LBT-IE
      - If the auxiliary security header is present, the device will encrypt the
        15.4 payload (see `SET_SEC_KEY`) and fill the MIC and the frame
        counter

 - `uint16_t flags`  
    A bitfield:
     - `0x0007 FHSS_TYPE_MASK`: Type of frequency hopping algorithm to use:
         - `0x00 FHSS_TYPE_FFN_UC`
         - `0x01 FHSS_TYPE_FFN_BC`
         - `0x02 FHSS_TYPE_LFN_UC`
         - `0x03 FHSS_TYPE_LFN_BC`
         - `0x04 FHSS_TYPE_ASYNC`
         - `0x06 FHSS_TYPE_LFN_PA`
     - `0x0008 RESERVED`: Possible extension of `FHSS_TYPE_MASK`
     - `0x0030 FHSS_CHAN_FUNC_MASK`:
         - `0x00 FHSS_CHAN_FUNC_FIXED`
         - `0x01 FHSS_CHAN_FUNC_TR51`
         - `0x02 FHSS_CHAN_FUNC_DH1`
         - `0x03 FHSS_CHAN_FUNC_AUTO`: Use value specified in `PROP_CONF_FHSS`.
           The exactly meaning depend of `FHSS_TYPE_MASK`
     - `0x0040 RESERVED`: Possible extension of `FHSS_CHAN_FUNC_MASK`
     - `0x0080 EDFE`
     - `0x0100 USE_MODE_SWITCH`
     - `0x0600 RESERVED`: For priority

Only present if `FHSS_TYPE_FFN_UC`:

 - `uint64_t rx_timestamp_mac_us`  
    Timestamp (relative to the date of the RCP reset) of the last received
    message for this node. The host will use the `timestamp_mac_us` value from
    commands `CNF_DATA_TX` and `IND_DATA_RX`.
    FIXME: Is uint32_t in milliseconds would be sufficient? Should we spend 4
    extra bytes for this data?

 - `uint24_t ufsi`  

 - `uint8_t dwell_interval`  

 - `uint8_t clock_drift`  
    (unused)

 - `uint8_t timing accuracy`  
    (unused)

Only present if `FHSS_TYPE_LFN_UC`:

 - `uint64_t rx_timestamp_mac_ms`  
    Timestamp (relative to the date of the RCP reset) of the last received
    message for this node. The relevant information can found in the field
    `timestamp_mac_us` of commands `CNF_DATA_TX` and `IND_DATA_RX`.
    FIXME: Is uint32_t in milliseconds would be sufficient? Should we spend 4
    extra bytes for this data?

 - `uint16_t slot_number`  

 - `uint24_t interval_offset_ms`  

 - `uint24_t interval_ms`  

 - `uint8_t  clock_drift`  
    (unused)

 - `uint8_t  timing accuracy`  
    (unused)

Only present if `FHSS_TYPE_FFN_BC`: (TBD)

 - `uint16_t slot_number`  

 - `uint32_t interval_offset_ms`  

Only present if `FHSS_TYPE_LFN_BC`: (TBD)

 - `uint16_t slot_number`  

 - `uint24_t interval_offset_ms`  

Only present if `FHSS_TYPE_LFN_PA`:

 - `uint16_t first_slot`  

 - `uint8_t  slot_time_ms`  

 - `uint8_t  slot_count`  

 - `uint24_t response_delay_ms`  

Only present if `FHSS_TYPE_ASYNC`:

 - `uint16_t max_tx_duration_ms`  

Only present if `FHSS_CHAN_FUNC_FIXED`:

 - `uint16_t chan_nr`  
    Fixed channel value.

Only present if `FHSS_CHAN_FUNC_DH1`:

 - `uint8_t chan_mask_len`  
    Length of the next field. Maximum value is 64.

 - `uint8_t chan_mask[]`  
    Bitmap of masked channels.

Only present if `FHSS_CHAN_FUNC_TR51`:

 TBD

Only present if `USE_MODE_SWITCH`:

 - `uint8_t phy_modes[2][4]`  
    An array of `phy_mode_id` and number of attempt to do for each of them.

### `0x12 CNF_DATA_TX`

Status of the earlier `REQ_DATA_TX`.

This API contains several timestamps. Instead encoding absolute timestamps, it
provide difference with the last step. Thus the message is shorter.

 - `uint8_t id`  
    `id` provided in `REQ_DATA_TX`

 - `uint8_t status`  
    Fine if 0. The fields below may be invalid if `status != 0`

 - `uint16_t payload_len`  
    Length of the next field (can be 0 if `status != 0`). Maximum value is 2047.

 - `uint8_t payload[]`  
    15.4 ack frame. Contains data if EDFE is enabled.

 - `uint64_t timestamp_mac_us`  
    The timestamp (relative to the date of the RCP reset) when the frame has
    been received on the device.

 - `uint32_t delay_mac_buffers_us`  
    Time spent by the frame in the MAC buffer before accessing the RF
    hardware. `timestamp_mac_us + delay_mac_buffer_us` give the timestamp
    of the beginning of the first tentative to send the frame.

 - `uint32_t delay_rf_us`  
    Time spent by the frame in the RF hardware before successful transmission.
    `timestamp_mac_us + delay_mac_buffer_us + delay_rf_us` give the time when
    the frame has started to be sent.

 - `uint16_t duration_tx_us`  
    Effective duration of the frame on the air. `timestamp_mac_us +
    delay_mac_buffer_us + delay_rf_us + duration_tx` gives the timestamp of
    the end of the tx transmission.  

 - `uint16_t delay_ack_us`  
    Delay between end of frame transmission and the start of ack frame.
    `tx_timestamp_mac_us + delay_mac_buffer_us + delay_rf_us +
    duration_tx + delay_ack_us` give the timestamp of the reception of the ack
    (aka `rx_timestamp_us`).  

 - `uint8_t  rx_lqi`  

 - `int16_t  rx_signal_strength_dbm`  
    More or less the same than RSSI.  

 - `uint32_t tx_frame_counter`  

 - `uint16_t tx_channel`  
    The effective channel used.  

 - `uint8_t tx_count1`  
    Number of transmissions including retries due to CCA.  
    If `status == 0`, `tx_count1` is at least 1.

 - `uint8_t tx_count2`  
    Number of transmission without counting failed CCA. `tx_count1` is always `>=
    tx_count2`.  
    If `status == 0`, `tx_count2` is at least 1.

 - `uint8_t flags`  
    A bitfield:
    - `0x01: HAVE_MODE_SWITCH_STAT`

Only present if `HAVE_MODE_SWITCH_STAT`:

 - `uint8_t phy_modes[4]`
    Number of times each `phy_mode` has been used. Sum of these fields is equal
    to `tx_count2`.

### `0x13 IND_DATA_RX`

 - `uint16_t payload_len`  
    Length of the next field. Maximum value is 2047.

 - `uint8_t payload[]`  
    15.4 frame.

 - `uint64_t timestamp_rx_us`  
    The timestamp (relative to the date of the RCP reset) when the frame has
    been received on the device.

 - `uint16_t duration_rx_us`  
    Effective duration of the frame on the air. `timestamp_rx_us +
    duration_rx` gives the timestamp of the end of the rx transmission.

 - `uint8_t rx_lqi`  

 - `int16_t rx_signal_strength_dbm`  

 - `uint8_t rx_phymode_id`  
    The effective phy used.

 - `uint16_t rx_channel`  
    The effective channel used.

 - `uint16_t filter_flags`  
    Same bit field than `PROP_FILTERS`  
    If one bit is set, the frame should had been filtered but the filter is not
    enabled.

 - `uint8_t flags`  
    Reserved for future usage.

## Radio configuration

PHY configuration and RCP level regional regulation enforcement. For
frequency hopping parameters, see ["FHSS configuration"][fhss].

[rf-get]:  #0x21-req_radio_list
[rf-list]: #0x22-cnf_radio_list
[rf-set]:  #0x23-set_radio
[rf-reg]:  #0x24-set_radio_regulation

### `0x20 REQ_RADIO_ENABLE`

Start the radio module. Before this, no packet can be transmitted
([`REQ_DATA_TX`][tx-req]) nor received ([`IND_DATA_RX`][rx]). Some
configuration needs to be done before calling this command, typically a PHY
must be selected with [`SET_RADIO`][rf-set], and a unicast schedule must be
configured with [`SET_FHSS_UC`][uc].

Body of this command is empty.

### `0x21 REQ_RADIO_LIST`

Request the list of radio configuration supported by the device, typically
issued by the host during a startup sequence before selecting a PHY
configuration. The RCP will answer with a series of [`CNF_RADIO_LIST`][rf-list]
commands.

Body of this command is empty.

### `0x22 CNF_RADIO_LIST`

List of radio configuration supported by the RCP, as configured during project
generation. Entries are defined in groups of mode-switch compatible PHYs. For
OFDM configurations, all MCS are defined in the same entry.

 - `uint8_t entry_size`  
    Size of a radio configuration entry in bytes. This is meant to support API
    evolution.

 - `bool list_end`  
    If not set, the list will continue in another [`CNF_RADIO_LIST`][rf-list].

 - `uint8_t count`  
    Number of radio configuration entries in next field.

 - `struct rf_config[]`  
    - `uint16_t flags`
        - `0x0001`: If set, this entry is in same group than the previous.
        - `0x01FE`: Bitfield of supported OFDM MCS (from MCS0 to MCS11).
    - `uint8_t rail_phy_mode_id`  
       Wi-SUN _PhyModeId_. For OFDM, only the _PhyType_ is set, and MCS support
       is indicated in the `flags` field.
    - `uint32_t chan_f0`  
       Frequency of the first channel in Hz.
    - `uint32_t chan_spacing`  
       Channel spacing in Hz.
    - `uint16_t chan_count`  
       Number of channels without holes in the spectrum.
    - `uint16_t sensitivity` (API >= 2.4.0)  
       Minimum RX sensitivity in dBm for this PHY.

### `0x23 SET_RADIO`

Configure the radio parameters.

 - `uint8_t index`  
    Index in the `rf_config` list.

 - `uint8_t mcs`  
    MCS to be used if `index` points to an OFDM modulation.

 - `bool enable_ms` (API > 2.0.1)  
    Enable mode switch in reception at PHY level (PHR), using all the PHY
    configurations available in the same group as the selected PHY. Note that
    mode switch cannot be disabled at MAC level (MAC command frame).

> [!NOTE]
> The host is responsible for advertising a list of supported _PhyModeId_ in a
> POM-IE. There may be more PHY supported in the RCP than advertised since the
> IE format restricts to 16 entries.

### `0x24 SET_RADIO_REGULATION`

Enable specific radio regulation rules. Most regulations only make sense with
specific channel configurations.

 - `uint32_t value`  
    - `0`: None (disabled, default)
    - `2`: Japan ([ARIB][arib])
    - `5`: India ([WPC][wpc])  

[arib]: https://www.arib.or.jp/
[wpc]:  https://dot.gov.in/spectrum-management/2457

### `0x25 SET_RADIO_TX_POWER`

 - `int8_t tx_power_dbm`  
    Maximum transmission power in dBm. The RCP may use a lower value based on
    internal decision making or hardware limitations but will never exceed the
    given value. The default value is 14dBm.

## Frequency Hopping (FHSS) configuration

Several schemes are used in Wi-SUN to transmit or receive packets using
frequency hopping. These are independently defined using a set of parameters
and may differ between transmission and reception. The following table
summarizes which commands configure the various FHSS parameters. Note that
[`REQ_DATA_TX`][tx-req] always needs to be called for transmission, but timing
parameters are sometimes passed with the command, and sometimes stored in the
RCP and configured once using a [`SET_FHSS`][fhss] command. See the description
of these commands for a more detailed explanation.

|  FHSS type                            | Location of FHSS parameters |
|---------------------------------------|-----------------------------|
| RX Unicast                            | [`SET_FHSS_UC`][uc]         |
| RX Broadcast                          | [`SET_FHSS_FFN_BC`][bc]     |
| TX Unicast to FFN                     | [`REQ_DATA_TX`][tx-req]     |
| TX Broadcast to FFN                   | [`SET_FHSS_FFN_BC`][bc]     |
| TX Unicast to LFN                     | [`REQ_DATA_TX`][tx-req]     |
| TX Unicast PAN Advert to LFN          | [`REQ_DATA_TX`][tx-req]     |
| TX Broadcast to LFN                   | [`SET_FHSS_LFN_BC`][bc-lfn] |
| TX Asynchronous (MLME-WS-ASYNC-FRAME) | [`SET_FHSS_ASYNC`][async]   |

### Channel Sequence

Most [`SET_FHSS`][fhss] commands configure a channel sequence using the
following variable-length structure:

 - `uint8_t chan_func`  
    Wi-SUN channel function, supported values are:
    - `0`: Single fixed channel. (API >= 2.1.1)
    - `2`: Direct Hash 1 (DH1CF) defined by Wi-SUN.

Only present if `chan_func == 0`:

  - `uint16_t chan_fixed`  
     Fixed channel number, valid range depends on the selected PHY.

Only present if `chan_func == 2`:

  - `uint8_t chan_mask_len`  
     Number of bytes in the following channel mask.

  - `uint8_t chan_mask[]`  
     Bitmask of used channels (1 for used, 0 for excluded). Channels are
     counted byte by byte, from least to most significant bit (channel `n` maps
     to `chan_mask[n / 8] & (1 << (n % 8))`). Integrators are responsible for
     meeting local regulation constraints by excluding disallowed channels from
     the selected PHY.

[fhss]:     #frequency-hopping-fhss-configuration
[chan-seq]: #channel-sequence
[uc]:       #0x30-set_fhss_uc
[bc]:       #0x31-set_fhss_ffn_bc
[bc-lfn]:   #0x32-set_fhss_lfn_bc
[async]:    #0x33-set_fhss_async

### `0x30 SET_FHSS_UC`

Configure unicast schedule for reception.

 - `uint8_t dwell_interval`  
    Unicast dwell interval in milliseconds (from US-IE).

 - `struct chan_seq`  
   See ["Channel Sequence"][chan-seq].

### `0x31 SET_FHSS_FFN_BC`

Configure broadcast schedule for reception and transmission from/to FFN.

 - `uint32_t interval`  
    Broadcast interval in milliseconds (from BS-IE).

 - `uint16_t bsi`  
    Broadcast Schedule ID (from BS-IE).

 - `uint16_t dwell_interval`  
    Broadcast dwell interval in milliseconds (from BS-IE).

 - `struct chan_seq`  
   See ["Channel Sequence"][chan-seq].

<!-- TODO: document parent BS-IE following -->

### `0x32 SET_FHSS_LFN_BC`

Configure broadcast schedule for transmission to LFN.

 - `uint16_t interval`  
    LFN broadcast interval in milliseconds (from LBS-IE).

 - `uint16_t bsi`  
    LFN broadcast Schedule ID (from LBS-IE).

 - `struct chan_seq`  
   See ["Channel Sequence"][chan-seq].

> [!WARNING]
> The current RCP implementation uses the same set of parameters for FFN and
> LFN broadcast channel sequences. Thus, [`SET_FHSS_FFN_BC`][bc] and
> [`SET_FHSS_LFN_BC`][bc-lfn] override each other's channel sequence. The other
> parameters are distinct.

> [!NOTE]
> The host is responsible for periodically sending LFN Time Sync frames in
> order to maintain broadcast timing in LFN children.

### `0x33 SET_FHSS_ASYNC`

Configure asynchronous transmissions for network discovery.

 - `uint32_t tx_duration_ms`  
    Maximum number of milliseconds the RCP is allowed to stay in continuous TX
    for an async transmission. If that duration is exceeded, the async
    transmission is split into chunks of that duration until all channels have
    been used. This mechanism makes the RCP radio available for other tasks
    between the async chunks, which becomes relevant for PHY configurations
    with many channels. By default, the value is set to `0xffffffff`, which
    disables this feature.

  - `uint8_t chan_mask_len`  
     Number of bytes in the following channel mask.

  - `uint8_t chan_mask[]`  
     Bitmask of channels to use for transmission. See the same field in
     ["Channel Sequence"][chan-seq] for more details.

## Security

> [!CAUTION]
> The RCP is responsible for encrypting and decrypting IEEE 802.15.4 frames.
> MAC layer data appears in cleartext in HIF commands so integrators are
> expected to secure the serial link, for example using
> [CPC with link encpryption][cpc-sec], otherwise the system may be exposed to
> packet injection or eavesdropping.

To encrypt transmitted IEEE 802.15.4 frames, an auxiliary security header must
be included in the buffer passed to [`REQ_DATA_TX`](#0x10-req_data_tx), and
space must be reserved at the end of the frame for the Message Integrity Code
(MIC) depending on the security level. The packet payload is not yet encrypted
when passed to the RCP. The RCP is responsible for reading the auxiliary
security header, finding the associated key, filling the frame counter field,
encrypting the payload, and filling the MIC.

Similarly, the RCP decrypts received packets and provides them to host in
cleartext with [`IND_DATA_RX`](#0x13-ind_data_rx), with the auxiliary
security header untouched so that the host can process it.

> [!WARNING]
> The RCP does not verify the frame counter for received frames. It is the
> responsability of the host to maintain frame counters per key and per
> neighbor, and check them to prevent any replay attacks.

Acknowledgement frames are decrypted and sent to the host in the same manner
with the exception that the RCP does perform a frame counter check based on
the minimum values provided in the [`REQ_DATA_TX`](#0x10-req_data_tx) for that
packet.

> [!NOTE]
> Only a subset of the IEEE 802.15.4 security modes is supported. Wi-SUN uses
> security level `6` and key ID mode `1` with key indices 1 through 7. Frame
> counter is always used.

[cpc-sec]: https://github.com/SiliconLabs/cpc-daemon/blob/main/readme.md#encrypted-serial-link

### `0x40 SET_SEC_KEY`

Install a security key for encrypting/decrypting IEEE 802.15.4 frames.

 - `uint8_t key_index`  
    Key index to use. Only values from 1 to 7 (inclusive) are supported.

 - `uint8_t key[16]`  
    Key in cleartext. If all zeroes, the key is un-installed.

 - `uint32_t frame_counter`  
    The initial frame counter value (for transmission). Should be 0 at first
    installation, should be positive on re-installation (after a RCP reboot).
    The RCP is responsible for incrementing the frame counter, and the value
    is communicated to the host in [`CNF_DATA_TX`](#0x12-cnf_data_tx) after
    each encrypted transmission.

## Packet Filtering

Filter out received packets in the RCP to prevent unecessary
[`IND_DATA_RX`](#0x13-ind_data_rx) indications.

### `0x58 SET_FILTER_PANID`

Refuse frames with an explicit PAN ID different from this value. By default,
the value is set to `0xffff` which disables the filter.

 - `uint16_t pan_id`  

### `0x5A SET_FILTER_SRC64`

Refuse frames based on the source MAC address. This should only be used in
specific testing scenarios to force network topologies. By default, the address
list is empty.

 - `bool allowed_list`  
   Instead of setting a list of denied address, set a list of allowed addresses
   where unknown ones are refused. Both ways are mutually exclusive and this
   command always resets the address list.

 - `uint8_t count`  
   Number of MAC addresses in the next field.

 - `uint8_t eui64[8][]`  
   List of MAC addresses (big endian) to deny/allow.

### `0x59 SET_FILTER_DST64`

Refuse unicast frames whose destination MAC address is not this EUI-64. By
default, the EUI-64 returned in [`IND_RESET`](#0x04-ind_reset) is used. This
filter cannot be disabled currently.

 - `uint8_t eui64[8]`  
    MAC address (big endian).

## Debug

[ping-req]: #0xe1-req_ping
[ping-cnf]: #0xe2-cnf_ping

### `0xE1 REQ_PING`

Send some arbitrary data to the device. The device will reply with
[`CNF_PING`][ping-cnf]. Use this command to stress and debug the serial link
with the device.

 - `uint16_t counter`  
    Arbitrary value (usually an incrementing counter that will be sent back in
    confirmation).

 - `uint16_t reply_payload_size`  
    Size of the payload in the [`CNF_PING`][ping-cnf].

 - `uint16_t payload_size`  
    Size of the `payload` field.

 - `uint8_t payload[]`  
    Arbitrary data. Data is not interpreted by the RCP.

### `0xE2 CNF_PING`

Reply to [`REQ_PING`][ping-req] with some arbitrary data.

 - `uint16_t counter`  
    `counter` received in [`REQ_PING`][ping-req].

 - `uint16_t payload_size`  
    Size of the `payload` field. Same value as the `reply_payload_size` field
    in [`REQ_PING`][ping-req].

 - `uint8_t payload[]`  
    Arbitrary data.

## Simulation

### `0xF0 IND_REPLAY_TIMER`

Only for simulation purpose. The device will never send it.

### `0xF1 IND_REPLAY_SOCKET`

Only for simulation purpose. The device will never send it.

