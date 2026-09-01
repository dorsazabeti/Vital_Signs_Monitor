#include "network_alert.h"

#include "eth.h"
#include "network_alert_config.h"
#include "network_alert_logic.h"

#include <stddef.h>
#include <string.h>

#define NETWORK_ETHERTYPE_IPV4       0x0800U
#define NETWORK_ETHERTYPE_ARP        0x0806U
#define NETWORK_ARP_REQUEST          1U
#define NETWORK_ARP_REPLY            2U
#define NETWORK_IP_PROTOCOL_ICMP     1U
#define NETWORK_IP_PROTOCOL_UDP      17U
#define NETWORK_FRAME_MAX            1536U
#define NETWORK_RX_POOL_COUNT        8U
#define NETWORK_ETH_HEADER_SIZE      14U
#define NETWORK_IPV4_HEADER_SIZE     20U
#define NETWORK_UDP_HEADER_SIZE      8U
#define NETWORK_ARP_FRAME_SIZE       42U
#define NETWORK_PHY_BSR              1U
#define NETWORK_PHY_SCSR             31U
#define NETWORK_PHY_LINK_UP          (1U << 2)
#define NETWORK_PHY_SPEED_MASK       0x001CU
#define NETWORK_PHY_10_HALF          0x0004U
#define NETWORK_PHY_100_HALF         0x0008U
#define NETWORK_PHY_10_FULL          0x0014U
#define NETWORK_PHY_100_FULL         0x0018U
#define NETWORK_TX_TIMEOUT_MS        20U

#if defined(__APPLE__)
/* Allows host syntax checks; the embedded GCC build uses the linker sections. */
#define NETWORK_DMA_SECTION(name) __attribute__((aligned(32)))
#else
#define NETWORK_DMA_SECTION(name) __attribute__((section(name), aligned(32)))
#endif

typedef enum
{
  NETWORK_RX_FREE = 0,
  NETWORK_RX_DMA,
  NETWORK_RX_APPLICATION
} NetworkRxBufferState;

typedef struct
{
  ETH_BufferTypeDef node;
  NetworkRxBufferState state;
} NetworkRxMetadata;

typedef struct
{
  NetworkAlertStatus status;
  NetworkAlertLogic logic;
  NetworkAlertEvent pending_event;
  uint8_t initialized;
  uint8_t pending;
  uint8_t arp_valid;
  uint8_t next_hop_mac[6];
  uint32_t last_arp_ms;
  uint32_t last_link_poll_ms;
  uint16_t ip_identification;
} NetworkAlertContext;

static const uint8_t network_device_mac[6] = {0x00U, 0x80U, 0xE1U, 0x00U, 0x00U, 0x00U};
static const uint8_t network_device_ip[4] = {
  NETWORK_ALERT_DEVICE_IP_0, NETWORK_ALERT_DEVICE_IP_1,
  NETWORK_ALERT_DEVICE_IP_2, NETWORK_ALERT_DEVICE_IP_3
};
static const uint8_t network_receiver_ip[4] = {
  NETWORK_ALERT_RECEIVER_IP_0, NETWORK_ALERT_RECEIVER_IP_1,
  NETWORK_ALERT_RECEIVER_IP_2, NETWORK_ALERT_RECEIVER_IP_3
};
static const uint8_t network_netmask[4] = {
  NETWORK_ALERT_NETMASK_0, NETWORK_ALERT_NETMASK_1,
  NETWORK_ALERT_NETMASK_2, NETWORK_ALERT_NETMASK_3
};
static const uint8_t network_gateway_ip[4] = {
  NETWORK_ALERT_GATEWAY_IP_0, NETWORK_ALERT_GATEWAY_IP_1,
  NETWORK_ALERT_GATEWAY_IP_2, NETWORK_ALERT_GATEWAY_IP_3
};
static const uint8_t network_broadcast_mac[6] = {
  0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU
};

static uint8_t network_rx_buffers[NETWORK_RX_POOL_COUNT][NETWORK_FRAME_MAX]
  NETWORK_DMA_SECTION(".EthRxBufferSection");
static uint8_t network_tx_frame[NETWORK_FRAME_MAX]
  NETWORK_DMA_SECTION(".EthTxBufferSection");
static NetworkRxMetadata network_rx_metadata[NETWORK_RX_POOL_COUNT];
static uint8_t network_rx_frame[NETWORK_FRAME_MAX];
static NetworkAlertContext network_context;

static uint16_t Network_ReadU16(const uint8_t *data)
{
  return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static void Network_WriteU16(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)value;
}

static uint16_t Network_Checksum(const uint8_t *data, uint16_t length)
{
  uint32_t sum = 0U;

  while (length > 1U)
  {
    sum += Network_ReadU16(data);
    data += 2;
    length -= 2U;
  }

  if (length != 0U)
  {
    sum += (uint32_t)data[0] << 8;
  }

  while ((sum >> 16) != 0U)
  {
    sum = (sum & 0xFFFFU) + (sum >> 16);
  }

  return (uint16_t)(~sum);
}

static uint8_t Network_IpEquals(const uint8_t *left, const uint8_t *right)
{
  return (memcmp(left, right, 4U) == 0) ? 1U : 0U;
}

static const uint8_t *Network_NextHopIp(void)
{
  uint32_t index;

  for (index = 0U; index < 4U; index++)
  {
    if ((network_receiver_ip[index] & network_netmask[index]) !=
        (network_device_ip[index] & network_netmask[index]))
    {
      return network_gateway_ip;
    }
  }

  return network_receiver_ip;
}

static HAL_StatusTypeDef Network_Transmit(uint16_t length)
{
  ETH_BufferTypeDef buffer;
  ETH_TxPacketConfigTypeDef config;
  HAL_StatusTypeDef result;

  if ((length < NETWORK_ETH_HEADER_SIZE) || (length > NETWORK_FRAME_MAX))
  {
    return HAL_ERROR;
  }

  memset(&buffer, 0, sizeof(buffer));
  memset(&config, 0, sizeof(config));
  buffer.buffer = network_tx_frame;
  buffer.len = length;
  config.Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD;
  config.Length = length;
  config.TxBuffer = &buffer;
  config.CRCPadCtrl = ETH_CRC_PAD_INSERT;

  result = HAL_ETH_Transmit(&heth, &config, NETWORK_TX_TIMEOUT_MS);
  if (result != HAL_OK)
  {
    network_context.status.transmit_errors++;
  }
  return result;
}

static HAL_StatusTypeDef Network_SendArp(uint16_t operation,
                                        const uint8_t *destination_mac,
                                        const uint8_t *target_ip,
                                        const uint8_t *target_mac)
{
  uint8_t *arp = network_tx_frame + NETWORK_ETH_HEADER_SIZE;

  memcpy(network_tx_frame, destination_mac, 6U);
  memcpy(network_tx_frame + 6U, network_device_mac, 6U);
  Network_WriteU16(network_tx_frame + 12U, NETWORK_ETHERTYPE_ARP);

  Network_WriteU16(arp, 1U);
  Network_WriteU16(arp + 2U, NETWORK_ETHERTYPE_IPV4);
  arp[4] = 6U;
  arp[5] = 4U;
  Network_WriteU16(arp + 6U, operation);
  memcpy(arp + 8U, network_device_mac, 6U);
  memcpy(arp + 14U, network_device_ip, 4U);
  memcpy(arp + 18U, target_mac, 6U);
  memcpy(arp + 24U, target_ip, 4U);

  return Network_Transmit(NETWORK_ARP_FRAME_SIZE);
}

static void Network_SendArpRequest(uint32_t now_ms)
{
  static const uint8_t empty_mac[6] = {0U, 0U, 0U, 0U, 0U, 0U};

  (void)Network_SendArp(NETWORK_ARP_REQUEST, network_broadcast_mac,
                        Network_NextHopIp(), empty_mac);
  network_context.last_arp_ms = now_ms;
}

static HAL_StatusTypeDef Network_SendUdp(const uint8_t *payload, uint16_t payload_length)
{
  uint8_t *ip = network_tx_frame + NETWORK_ETH_HEADER_SIZE;
  uint8_t *udp = ip + NETWORK_IPV4_HEADER_SIZE;
  uint16_t ip_length = (uint16_t)(NETWORK_IPV4_HEADER_SIZE +
                                  NETWORK_UDP_HEADER_SIZE + payload_length);

  if ((uint32_t)NETWORK_ETH_HEADER_SIZE + ip_length > NETWORK_FRAME_MAX)
  {
    return HAL_ERROR;
  }

  memcpy(network_tx_frame, network_context.next_hop_mac, 6U);
  memcpy(network_tx_frame + 6U, network_device_mac, 6U);
  Network_WriteU16(network_tx_frame + 12U, NETWORK_ETHERTYPE_IPV4);

  memset(ip, 0, NETWORK_IPV4_HEADER_SIZE);
  ip[0] = 0x45U;
  Network_WriteU16(ip + 2U, ip_length);
  Network_WriteU16(ip + 4U, ++network_context.ip_identification);
  Network_WriteU16(ip + 6U, 0x4000U);
  ip[8] = 64U;
  ip[9] = NETWORK_IP_PROTOCOL_UDP;
  memcpy(ip + 12U, network_device_ip, 4U);
  memcpy(ip + 16U, network_receiver_ip, 4U);
  Network_WriteU16(ip + 10U, Network_Checksum(ip, NETWORK_IPV4_HEADER_SIZE));

  Network_WriteU16(udp, NETWORK_ALERT_SOURCE_PORT);
  Network_WriteU16(udp + 2U, NETWORK_ALERT_UDP_PORT);
  Network_WriteU16(udp + 4U, (uint16_t)(NETWORK_UDP_HEADER_SIZE + payload_length));
  Network_WriteU16(udp + 6U, 0U); /* An IPv4 UDP checksum of zero is valid. */
  memcpy(udp + NETWORK_UDP_HEADER_SIZE, payload, payload_length);

  return Network_Transmit((uint16_t)(NETWORK_ETH_HEADER_SIZE + ip_length));
}

static void Network_TrySendPending(uint32_t now_ms)
{
  char payload[512];
  size_t payload_length;

  if (network_context.pending == 0U)
  {
    return;
  }

  if (network_context.arp_valid == 0U)
  {
    network_context.status.state = NETWORK_ALERT_STATE_WAITING_FOR_ARP;
    if ((network_context.last_arp_ms == 0U) ||
        ((uint32_t)(now_ms - network_context.last_arp_ms) >= NETWORK_ALERT_ARP_RETRY_MS))
    {
      Network_SendArpRequest(now_ms);
    }
    return;
  }

  payload_length = NetworkAlertLogic_FormatJson(&network_context.pending_event,
                                                 payload, sizeof(payload));
  if ((payload_length != 0U) &&
      (Network_SendUdp((const uint8_t *)payload, (uint16_t)payload_length) == HAL_OK))
  {
    network_context.pending = 0U;
    network_context.status.alerts_sent++;
    network_context.status.state = NETWORK_ALERT_STATE_READY;
  }
}

static void Network_HandleArp(const uint8_t *frame, uint16_t length, uint32_t now_ms)
{
  const uint8_t *arp;
  uint16_t operation;

  if (length < NETWORK_ARP_FRAME_SIZE)
  {
    return;
  }

  arp = frame + NETWORK_ETH_HEADER_SIZE;
  if ((Network_ReadU16(arp) != 1U) ||
      (Network_ReadU16(arp + 2U) != NETWORK_ETHERTYPE_IPV4) ||
      (arp[4] != 6U) || (arp[5] != 4U))
  {
    return;
  }

  operation = Network_ReadU16(arp + 6U);
  if ((operation == NETWORK_ARP_REQUEST) &&
      (Network_IpEquals(arp + 24U, network_device_ip) != 0U))
  {
    (void)Network_SendArp(NETWORK_ARP_REPLY, arp + 8U, arp + 14U, arp + 8U);
  }

  if (((operation == NETWORK_ARP_REPLY) || (operation == NETWORK_ARP_REQUEST)) &&
      (Network_IpEquals(arp + 14U, Network_NextHopIp()) != 0U))
  {
    memcpy(network_context.next_hop_mac, arp + 8U, 6U);
    network_context.arp_valid = 1U;
    network_context.status.state = NETWORK_ALERT_STATE_READY;
    Network_TrySendPending(now_ms);
  }
}

static void Network_HandleIcmp(const uint8_t *frame, uint16_t length)
{
  const uint8_t *received_ip = frame + NETWORK_ETH_HEADER_SIZE;
  uint16_t ip_header_length;
  uint16_t ip_total_length;
  uint8_t *reply_ip;
  uint8_t *reply_icmp;
  uint16_t icmp_length;

  if (length < (NETWORK_ETH_HEADER_SIZE + NETWORK_IPV4_HEADER_SIZE + 8U))
  {
    return;
  }

  ip_header_length = (uint16_t)((received_ip[0] & 0x0FU) * 4U);
  ip_total_length = Network_ReadU16(received_ip + 2U);
  if ((ip_header_length < NETWORK_IPV4_HEADER_SIZE) ||
      (ip_total_length < (ip_header_length + 8U)) ||
      ((uint32_t)NETWORK_ETH_HEADER_SIZE + ip_total_length > length) ||
      (received_ip[9] != NETWORK_IP_PROTOCOL_ICMP) ||
      (Network_IpEquals(received_ip + 16U, network_device_ip) == 0U) ||
      (received_ip[ip_header_length] != 8U) ||
      ((uint32_t)NETWORK_ETH_HEADER_SIZE + ip_total_length > NETWORK_FRAME_MAX))
  {
    return;
  }

  memcpy(network_tx_frame, frame, NETWORK_ETH_HEADER_SIZE + ip_total_length);
  memcpy(network_tx_frame, frame + 6U, 6U);
  memcpy(network_tx_frame + 6U, network_device_mac, 6U);
  reply_ip = network_tx_frame + NETWORK_ETH_HEADER_SIZE;
  memcpy(reply_ip + 12U, network_device_ip, 4U);
  memcpy(reply_ip + 16U, received_ip + 12U, 4U);
  reply_ip[8] = 64U;
  Network_WriteU16(reply_ip + 4U, ++network_context.ip_identification);
  Network_WriteU16(reply_ip + 10U, 0U);
  Network_WriteU16(reply_ip + 10U, Network_Checksum(reply_ip, ip_header_length));

  reply_icmp = reply_ip + ip_header_length;
  icmp_length = (uint16_t)(ip_total_length - ip_header_length);
  reply_icmp[0] = 0U;
  reply_icmp[1] = 0U;
  Network_WriteU16(reply_icmp + 2U, 0U);
  Network_WriteU16(reply_icmp + 2U, Network_Checksum(reply_icmp, icmp_length));
  (void)Network_Transmit((uint16_t)(NETWORK_ETH_HEADER_SIZE + ip_total_length));
}

static void Network_HandleFrame(const uint8_t *frame, uint16_t length, uint32_t now_ms)
{
  uint16_t ether_type;

  if (length < NETWORK_ETH_HEADER_SIZE)
  {
    return;
  }

  ether_type = Network_ReadU16(frame + 12U);
  if (ether_type == NETWORK_ETHERTYPE_ARP)
  {
    Network_HandleArp(frame, length, now_ms);
  }
  else if (ether_type == NETWORK_ETHERTYPE_IPV4)
  {
    Network_HandleIcmp(frame, length);
  }
}

static uint16_t Network_CopyAndReleaseRx(ETH_BufferTypeDef *buffer)
{
  uint16_t total = 0U;

  while (buffer != NULL)
  {
    ETH_BufferTypeDef *next = buffer->next;
    uint32_t index;
    uint32_t copy_length = buffer->len;

    if (copy_length > (NETWORK_FRAME_MAX - total))
    {
      copy_length = NETWORK_FRAME_MAX - total;
    }
    if (copy_length != 0U)
    {
      memcpy(network_rx_frame + total, buffer->buffer, copy_length);
      total = (uint16_t)(total + copy_length);
    }

    for (index = 0U; index < NETWORK_RX_POOL_COUNT; index++)
    {
      if (&network_rx_metadata[index].node == buffer)
      {
        network_rx_metadata[index].state = NETWORK_RX_FREE;
        network_rx_metadata[index].node.next = NULL;
        break;
      }
    }
    buffer = next;
  }

  return total;
}

static uint8_t Network_PhyLinkUp(void)
{
  uint32_t status;

  /* Read twice because the LAN8742 link bit is latch-low. */
  if ((HAL_ETH_ReadPHYRegister(&heth, NETWORK_ALERT_PHY_ADDRESS,
                               NETWORK_PHY_BSR, &status) != HAL_OK) ||
      (HAL_ETH_ReadPHYRegister(&heth, NETWORK_ALERT_PHY_ADDRESS,
                               NETWORK_PHY_BSR, &status) != HAL_OK))
  {
    return 0U;
  }

  return ((status & NETWORK_PHY_LINK_UP) != 0U) ? 1U : 0U;
}

static void Network_ConfigureMacFromPhy(void)
{
  uint32_t phy_status;
  ETH_MACConfigTypeDef mac_config;

  if ((HAL_ETH_ReadPHYRegister(&heth, NETWORK_ALERT_PHY_ADDRESS,
                               NETWORK_PHY_SCSR, &phy_status) != HAL_OK) ||
      (HAL_ETH_GetMACConfig(&heth, &mac_config) != HAL_OK))
  {
    return;
  }

  switch (phy_status & NETWORK_PHY_SPEED_MASK)
  {
    case NETWORK_PHY_10_HALF:
      mac_config.Speed = ETH_SPEED_10M;
      mac_config.DuplexMode = ETH_HALFDUPLEX_MODE;
      break;
    case NETWORK_PHY_100_HALF:
      mac_config.Speed = ETH_SPEED_100M;
      mac_config.DuplexMode = ETH_HALFDUPLEX_MODE;
      break;
    case NETWORK_PHY_10_FULL:
      mac_config.Speed = ETH_SPEED_10M;
      mac_config.DuplexMode = ETH_FULLDUPLEX_MODE;
      break;
    case NETWORK_PHY_100_FULL:
    default:
      mac_config.Speed = ETH_SPEED_100M;
      mac_config.DuplexMode = ETH_FULLDUPLEX_MODE;
      break;
  }

  (void)HAL_ETH_SetMACConfig(&heth, &mac_config);
}

static void Network_UpdateLink(uint32_t now_ms)
{
  uint8_t link_up;
  HAL_ETH_StateTypeDef eth_state;

  if ((network_context.last_link_poll_ms != 0U) &&
      ((uint32_t)(now_ms - network_context.last_link_poll_ms) < NETWORK_ALERT_LINK_POLL_MS))
  {
    return;
  }
  network_context.last_link_poll_ms = now_ms;
  link_up = Network_PhyLinkUp();

  if ((link_up != 0U) && (network_context.status.link_up == 0U))
  {
    Network_ConfigureMacFromPhy();
    eth_state = HAL_ETH_GetState(&heth);
    if (eth_state == HAL_ETH_STATE_READY)
    {
      if (HAL_ETH_Start(&heth) != HAL_OK)
      {
        network_context.status.state = NETWORK_ALERT_STATE_ERROR;
        return;
      }
    }
    else if (eth_state != HAL_ETH_STATE_STARTED)
    {
      network_context.status.state = NETWORK_ALERT_STATE_ERROR;
      return;
    }
    network_context.status.link_up = 1U;
    network_context.arp_valid = 0U;
    network_context.status.state = NETWORK_ALERT_STATE_WAITING_FOR_ARP;
    Network_SendArpRequest(now_ms);
  }
  else if ((link_up == 0U) && (network_context.status.link_up != 0U))
  {
    if (HAL_ETH_GetState(&heth) == HAL_ETH_STATE_STARTED)
    {
      (void)HAL_ETH_Stop(&heth);
    }
    network_context.status.link_up = 0U;
    network_context.arp_valid = 0U;
    network_context.status.state = NETWORK_ALERT_STATE_LINK_DOWN;
  }
}

void HAL_ETH_RxAllocateCallback(uint8_t **buffer)
{
  uint32_t index;

  if (buffer == NULL)
  {
    return;
  }
  *buffer = NULL;

  for (index = 0U; index < NETWORK_RX_POOL_COUNT; index++)
  {
    if (network_rx_metadata[index].state == NETWORK_RX_FREE)
    {
      network_rx_metadata[index].state = NETWORK_RX_DMA;
      *buffer = network_rx_buffers[index];
      return;
    }
  }
}

void HAL_ETH_RxLinkCallback(void **start, void **end, uint8_t *buffer, uint16_t length)
{
  uint32_t index;
  ETH_BufferTypeDef *node = NULL;

  if ((start == NULL) || (end == NULL) || (buffer == NULL))
  {
    return;
  }

  for (index = 0U; index < NETWORK_RX_POOL_COUNT; index++)
  {
    if (network_rx_buffers[index] == buffer)
    {
      network_rx_metadata[index].state = NETWORK_RX_APPLICATION;
      node = &network_rx_metadata[index].node;
      node->buffer = buffer;
      node->len = length;
      node->next = NULL;
      break;
    }
  }

  if (node == NULL)
  {
    return;
  }

  if (*start == NULL)
  {
    *start = node;
  }
  else
  {
    ((ETH_BufferTypeDef *)(*end))->next = node;
  }
  *end = node;
}

void NetworkAlert_Init(void)
{
  HAL_ETH_StateTypeDef eth_state;

  memset(&network_context, 0, sizeof(network_context));
  memset(network_rx_metadata, 0, sizeof(network_rx_metadata));
  NetworkAlertLogic_Init(&network_context.logic);
  network_context.status.state = NETWORK_ALERT_STATE_INITIALIZING;

  eth_state = HAL_ETH_GetState(&heth);
  if (eth_state == HAL_ETH_STATE_RESET)
  {
    /* LCD_BRINGUP_MODE skips this generated initializer in main.c. */
    MX_ETH_Init();
    eth_state = HAL_ETH_GetState(&heth);
  }

  if ((eth_state != HAL_ETH_STATE_READY) && (eth_state != HAL_ETH_STATE_STARTED))
  {
    network_context.status.state = NETWORK_ALERT_STATE_ERROR;
    return;
  }

  network_context.initialized = 1U;
  network_context.status.state = NETWORK_ALERT_STATE_LINK_DOWN;
  Network_UpdateLink(HAL_GetTick());
}

void NetworkAlert_Process(void)
{
  uint32_t now_ms;
  uint32_t frames = 0U;

  if (network_context.initialized == 0U)
  {
    return;
  }

  now_ms = HAL_GetTick();
  Network_UpdateLink(now_ms);
  if ((network_context.status.link_up == 0U) ||
      (HAL_ETH_GetState(&heth) != HAL_ETH_STATE_STARTED))
  {
    return;
  }

  while (frames < ETH_RX_DESC_CNT)
  {
    ETH_BufferTypeDef *received = NULL;
    uint16_t length;

    if (HAL_ETH_ReadData(&heth, (void **)&received) != HAL_OK)
    {
      break;
    }
    if (received == NULL)
    {
      network_context.status.receive_errors++;
      break;
    }

    length = Network_CopyAndReleaseRx(received);
    network_context.status.frames_received++;
    Network_HandleFrame(network_rx_frame, length, now_ms);
    frames++;
  }

  if ((network_context.arp_valid == 0U) &&
      ((network_context.last_arp_ms == 0U) ||
       ((uint32_t)(now_ms - network_context.last_arp_ms) >= NETWORK_ALERT_ARP_RETRY_MS)))
  {
    Network_SendArpRequest(now_ms);
  }
  Network_TrySendPending(now_ms);
}

void NetworkAlert_UpdateVitals(uint16_t heart_rate, uint8_t spo2,
                               int16_t temperature_tenths,
                               const char *scenario)
{
  NetworkAlertVitals vitals;
  NetworkAlertEvent event;
  size_t scenario_length = 0U;

  memset(&vitals, 0, sizeof(vitals));
  vitals.heart_rate = heart_rate;
  vitals.spo2 = spo2;
  vitals.temperature_tenths = temperature_tenths;
  if (scenario != NULL)
  {
    while ((scenario[scenario_length] != '\0') &&
           (scenario_length < (sizeof(vitals.scenario) - 1U)))
    {
      vitals.scenario[scenario_length] = scenario[scenario_length];
      scenario_length++;
    }
  }

  if (NetworkAlertLogic_Evaluate(&network_context.logic, &vitals,
                                 HAL_GetTick(), &event) != 0U)
  {
    network_context.pending_event = event;
    network_context.pending = 1U;
  }
  network_context.status.alert_active = network_context.logic.active;
}

NetworkAlertStatus NetworkAlert_GetStatus(void)
{
  return network_context.status;
}
