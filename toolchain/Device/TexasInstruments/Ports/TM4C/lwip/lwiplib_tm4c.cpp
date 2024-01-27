
#include "lwip/igmp.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/dhcp.h"
#include "lwiplib.h"
#include "netif/etharp.h"
#include "netif/tivaif.h"
#include "driverlib/debug.h"
#include <inc/hw_emac.h>

#ifdef PART_TM4C1292NCPDT
    #include "driverlib/gpio.h"
    #include "driverlib/pin_map.h"
#endif
//*****************************************************************************
//
// Ensure that ICMP checksum offloading is enabled; otherwise the TM4C129
// driver will not operate correctly.
//
//*****************************************************************************
#ifndef LWIP_OFFLOAD_ICMP_CHKSUM
    #define LWIP_OFFLOAD_ICMP_CHKSUM 1
#endif

struct netif g_netif;
//*****************************************************************************
//
// The lwIP Library abstration layer provides for a host callback function to
// be called periodically in the lwIP context.  This is the timer interval, in
// ms, for this periodic callback.  If the timer interval is defined to 0 (the
// default value), then no periodic host callback is performed.
//
//*****************************************************************************
#ifndef HOST_TMR_INTERVAL
    #define HOST_TMR_INTERVAL 0
#else
extern "C" void lwip_process_host_timer(void);
#endif

//*****************************************************************************
//
// The link detect polling interval.
//
//*****************************************************************************
#define LINK_TMR_INTERVAL 10

//*****************************************************************************
//
// Set the PHY configuration to the default (internal) option if necessary.
//
//*****************************************************************************
#ifndef EMAC_PHY_CONFIG
    #define EMAC_PHY_CONFIG (EMAC_PHY_TYPE_INTERNAL | EMAC_PHY_INT_MDIX_EN | EMAC_PHY_AN_100B_T_FULL_DUPLEX)
#endif

//*****************************************************************************
//
// Driverlib headers needed for this library module.
//
//*****************************************************************************
#include "driverlib/debug.h"
#include "driverlib/emac.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#if !NO_SYS
    #if RTOS_FREERTOS
        #include "FreeRTOS.h"
        #include "queue.h"
        #include "semphr.h"
        #include "task.h"
    #endif
    #if ((RTOS_FREERTOS) < 1)
        #error No RTOS is defined.  Please define an RTOS.
    #endif
    #if ((RTOS_FREERTOS) > 1)
        #error More than one RTOS defined.  Please define only one RTOS at a time.
    #endif
#endif

//*****************************************************************************
//
// The lwIP network interface structure for the Tiva Ethernet MAC.
//
//*****************************************************************************

//*****************************************************************************
//
// The application's interrupt handler for hardware timer events from the MAC.
//
//*****************************************************************************
tHardwareTimerHandler g_lwip_timestamp_handler;

//*****************************************************************************
//
// The local time for the lwIP Library Abstraction layer, used to support the
// Host and lwIP periodic callback functions.
//
//*****************************************************************************
#if NO_SYS
uint32_t g_lwip_local_timer = 0;
#endif

//*****************************************************************************
//
// The local time when the TCP timer was last serviced.
//
//*****************************************************************************
#if NO_SYS
static uint32_t g_lwip_tcp_timer = 0;
#endif

//*****************************************************************************
//
// The local time when the HOST timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && HOST_TMR_INTERVAL
static uint32_t g_ui32HostTimer = 0;
#endif

//*****************************************************************************
//
// The local time when the ARP timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && LWIP_ARP
static uint32_t g_lwip_arp_timer = 0;
#endif

//*****************************************************************************
//
// The local time when the AutoIP timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && LWIP_AUTOIP
static uint32_t g_ui32AutoIPTimer = 0;
#endif

//*****************************************************************************
//
// The local time when the DHCP Coarse timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && LWIP_DHCP
static uint32_t g_lwip_dhcp_coarse_timer = 0;
#endif

//*****************************************************************************
//
// The local time when the DHCP Fine timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && LWIP_DHCP
static uint32_t g_lwip_dhcp_fine_timer = 0;
#endif

//*****************************************************************************
//
// The local time when the IP Reassembly timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && IP_REASSEMBLY
static uint32_t g_lwip_ip_reassembly_timer = 0;
#endif

//*****************************************************************************
//
// The local time when the IGMP timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && LWIP_IGMP
static uint32_t g_lwip_igmp_timer = 0;
#endif

//*****************************************************************************
//
// The local time when the DNS timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && LWIP_DNS
static uint32_t g_ui32DNSTimer = 0;
#endif

//*****************************************************************************
//
// The local time when the link detect timer was last serviced.
//
//*****************************************************************************
#if NO_SYS && (LWIP_AUTOIP || LWIP_DHCP)
static uint32_t g_lwip_link_timer = 0;
#endif

//*****************************************************************************
//
// The default IP address acquisition mode.
//
//*****************************************************************************
static uint32_t g_ip_mode = IPADDR_USE_STATIC;

//*****************************************************************************
//
// The most recently detected link state.
//
//*****************************************************************************
#if LWIP_AUTOIP || LWIP_DHCP
static bool g_lwip_link_active = false;
#endif

//*****************************************************************************
//
// The IP address to be used.  This is used during the initialization of the
// stack and when the interface configuration is changed.
//
//*****************************************************************************
static uint32_t g_ip_address;

//*****************************************************************************
//
// The netmask to be used.  This is used during the initialization of the stack
// and when the interface configuration is changed.
//
//*****************************************************************************
static uint32_t g_netmask;

//*****************************************************************************
//
// The gateway address to be used.  This is used during the initialization of
// the stack and when the interface configuration is changed.
//
//*****************************************************************************
static uint32_t g_gateway;

//*****************************************************************************
//
// The stack size for the interrupt task.
//
//*****************************************************************************
#if !NO_SYS
    #define STACKSIZE_LWIPINTTASK 128
#endif

//*****************************************************************************
//
// The handle for the "queue" (semaphore) used to signal the interrupt task
// from the interrupt handler.
//
//*****************************************************************************
#if !NO_SYS
static xQueueHandle g_pInterrupt;
#endif

//*****************************************************************************
//
// This task handles reading packets from the Ethernet controller and supplying
// them to the TCP/IP thread.
//
//*****************************************************************************
#if !NO_SYS
static void lwIPInterruptTask(void *pvArg) {
    //
    // Loop forever.
    //
    while (1) {
        //
        // Wait until the semaphore has been signaled.
        //
        while (xQueueReceive(g_pInterrupt, &pvArg, portMAX_DELAY) != pdPASS) {
        }

        //
        // Processes any packets waiting to be sent or received.
        //
        tivaif_interrupt(&g_sNetIF, (uint32_t)pvArg);

        //
        // Re-enable the Ethernet interrupts.
        //
        MAP_EMACIntEnable(
            EMAC0_BASE,
            (EMAC_INT_RECEIVE | EMAC_INT_TRANSMIT | EMAC_INT_TX_STOPPED | EMAC_INT_RX_NO_BUFFER | EMAC_INT_RX_STOPPED | EMAC_INT_PHY));
    }
}
#endif

//*****************************************************************************
//
// This function performs a periodic check of the link status and responds
// appropriately if it has changed.
//
//*****************************************************************************
#if LWIP_AUTOIP || LWIP_DHCP
static void s_lwip_detect_link(void) {
    bool have_link;
    ip_addr_t ip_addr;
    ip_addr_t net_mask;
    ip_addr_t gw_addr;

    //
    // See if there is an active link.
    //
    have_link = MAP_EMACPHYRead(EMAC0_BASE, PHY_PHYS_ADDR, EPHY_BMSR) & EPHY_BMSR_LINKSTAT;

    //
    // Return without doing anything else if the link state hasn't changed.
    //
    if (g_ip_mode == IPADDR_USE_STATIC || have_link == g_lwip_link_active) {
        return;
    }

    //
    // Save the new link state.
    //
    g_lwip_link_active = have_link;

    //
    // Clear any address information from the network interface.
    //
    ip_addr.addr  = 0;
    net_mask.addr = 0;
    gw_addr.addr  = 0;
    netif_set_addr(&g_netif, &ip_addr, &net_mask, &gw_addr);

    //
    // See if there is a link now.
    //
    if (have_link) {
        //
        // Start DHCP, if enabled.
        //
    #if LWIP_DHCP
        if (g_ip_mode == IPADDR_USE_DHCP) {
            dhcp_start(&g_netif);
        }
    #endif

        //
        // Start AutoIP, if enabled and DHCP is not.
        //
    #if LWIP_AUTOIP
        if (g_ip_mode == IPADDR_USE_AUTOIP) {
            autoip_start(&g_sNetIF);
        }
    #endif
    } else {
        //
        // Stop DHCP, if enabled.
        //
    #if LWIP_DHCP
        if (g_ip_mode == IPADDR_USE_DHCP) {
            dhcp_stop(&g_netif);
        }
    #endif

        //
        // Stop AutoIP, if enabled and DHCP is not.
        //
    #if LWIP_AUTOIP
        if (g_ip_mode == IPADDR_USE_AUTOIP) {
            autoip_stop(&g_sNetIF);
        }
    #endif
    }
}
#endif

//*****************************************************************************
//
// This function services all of the lwIP periodic timers, including TCP and
// Host timers.  This should be called from the lwIP context, which may be
// the Ethernet interrupt (in the case of a non-RTOS system) or the lwIP
// thread, in the event that an RTOS is used.
//
//*****************************************************************************
#if NO_SYS
// lwIPServiceTimers
void lwip_process_timers() {
    //
    // Service the host timer.
    //
    #if HOST_TMR_INTERVAL
    if ((g_lwip_local_timer - g_lwip_host_timer) >= HOST_TMR_INTERVAL) {
        g_lwip_host_timer = g_lwip_local_timer;
        lwip_process_host_timer();
    }
    #endif

    //
    // Service the ARP timer.
    //
    #if LWIP_ARP
    if ((g_lwip_local_timer - g_lwip_arp_timer) >= ARP_TMR_INTERVAL) {
        g_lwip_arp_timer = g_lwip_local_timer;
        etharp_tmr();
    }
    #endif

    //
    // Service the TCP timer.
    //
    #if LWIP_TCP
    if ((g_lwip_local_timer - g_lwip_tcp_timer) >= TCP_TMR_INTERVAL) {
        g_lwip_tcp_timer = g_lwip_local_timer;
        tcp_tmr();
    }
    #endif

    //
    // Service the AutoIP timer.
    //
    #if LWIP_AUTOIP
    if ((g_ui32LocalTimer - g_ui32AutoIPTimer) >= AUTOIP_TMR_INTERVAL) {
        g_ui32AutoIPTimer = g_ui32LocalTimer;
        autoip_tmr();
    }
    #endif

    //
    // Service the DCHP Coarse Timer.
    //
    #if LWIP_DHCP
    if ((g_lwip_local_timer - g_lwip_dhcp_coarse_timer) >= DHCP_COARSE_TIMER_MSECS) {
        g_lwip_dhcp_coarse_timer = g_lwip_local_timer;
        dhcp_coarse_tmr();
    }
    #endif

    //
    // Service the DCHP Fine Timer.
    //
    #if LWIP_DHCP
    if ((g_lwip_local_timer - g_lwip_dhcp_fine_timer) >= DHCP_FINE_TIMER_MSECS) {
        g_lwip_dhcp_fine_timer = g_lwip_local_timer;
        dhcp_fine_tmr();
    }
    #endif

    //
    // Service the IP Reassembly Timer
    //
    #if IP_REASSEMBLY
    if ((g_lwip_local_timer - g_lwip_ip_reassembly_timer) >= IP_TMR_INTERVAL) {
        g_lwip_ip_reassembly_timer = g_lwip_local_timer;
        ip_reass_tmr();
    }
    #endif

    //
    // Service the IGMP Timer
    //
    #if LWIP_IGMP
    if ((g_lwip_local_timer - g_lwip_igmp_timer) >= IGMP_TMR_INTERVAL) {
        g_lwip_igmp_timer = g_lwip_local_timer;
        igmp_tmr();
    }
    #endif

    //
    // Service the DNS Timer
    //
    #if LWIP_DNS
    if ((g_ui32LocalTimer - g_ui32DNSTimer) >= DNS_TMR_INTERVAL) {
        g_ui32DNSTimer = g_ui32LocalTimer;
        dns_tmr();
    }
    #endif

    //
    // Service the link timer.
    //
    #if LWIP_AUTOIP || LWIP_DHCP
    if ((g_lwip_local_timer - g_lwip_link_timer) >= LINK_TMR_INTERVAL) {
        g_lwip_link_timer = g_lwip_local_timer;
        s_lwip_detect_link();
    }
    #endif
}
#endif

//*****************************************************************************
//
// Handles the timeout for the host callback function timer when using a RTOS.
//
//*****************************************************************************
#if !NO_SYS && HOST_TMR_INTERVAL
static void lwIPPrivateHostTimer(void *pvArg) {
    //
    // Call the application-supplied host timer callback function.
    //
    lwip_process_host_timer();

    //
    // Re-schedule the host timer callback function timeout.
    //
    sys_timeout(HOST_TMR_INTERVAL, lwIPPrivateHostTimer, nullptr);
}
#endif

//*****************************************************************************
//
// Handles the timeout for the link detect timer when using a RTOS.
//
//*****************************************************************************
#if !NO_SYS && (LWIP_AUTOIP || LWIP_DHCP)
static void lwIPPrivateLinkTimer(void *pvArg) {
    //
    // Perform the link detection.
    //
    lwIPLinkDetect();

    //
    // Re-schedule the link detect timer timeout.
    //
    sys_timeout(LINK_TMR_INTERVAL, lwIPPrivateLinkTimer, nullptr);
}
#endif

//*****************************************************************************
//
// Completes the initialization of lwIP.  This is directly called when not
// using a RTOS and provided as a callback to the TCP/IP thread when using a
// RTOS.
//
//*****************************************************************************
static void s_lwip_initialize(__maybe_unused void *arg) {
    ip4_addr_t ip_addr;
    ip4_addr_t net_mask;
    ip4_addr_t gw_addr;

    //
    // If not using a RTOS, initialize the lwIP stack.
    //
#if NO_SYS
    lwip_init();
#endif

    //
    // If using a RTOS, create a queue (to be used as a semaphore) to signal
    // the Ethernet interrupt task from the Ethernet interrupt handler.
    //
#if !NO_SYS
    #if RTOS_FREERTOS
    g_pInterrupt = xQueueCreate(1, sizeof(void *));
    #endif
#endif

    //
    // If using a RTOS, create the Ethernet interrupt task.
    //
#if !NO_SYS
    #if RTOS_FREERTOS
    xTaskCreate(lwIPInterruptTask, (signed portCHAR *)"eth_int", STACKSIZE_LWIPINTTASK, 0, tskIDLE_PRIORITY + 1, 0);
    #endif
#endif

    //
    // Setup the network address values.
    //
    if (g_ip_mode == IPADDR_USE_STATIC) {
        ip_addr.addr  = g_ip_address;
        net_mask.addr = g_netmask;
        gw_addr.addr  = g_gateway;
    } else {
        ip_addr.addr  = 0;
        net_mask.addr = 0;
        gw_addr.addr  = 0;
    }

    //
    // Create, configure and add the Ethernet controller interface with
    // default settings.  ip_input should be used to send packets directly to
    // the stack when not using a RTOS and tcpip_input should be used to send
    // packets to the TCP/IP thread's queue when using a RTOS.
    //
#if NO_SYS
    netif_add(&g_netif, &ip_addr, &net_mask, &gw_addr, nullptr, tivaif_init, ip_input);
#else
    netif_add(&g_sNetIF, &ip_addr, &net_mask, &gw_addr, nullptr, tivaif_init, tcpip_input);
#endif
    netif_set_default(&g_netif);

    //
    // Bring the interface up.
    //
    netif_set_up(&g_netif);

    //
    // Setup a timeout for the host timer callback function if using a RTOS.
    //
#if !NO_SYS && HOST_TMR_INTERVAL
    sys_timeout(HOST_TMR_INTERVAL, lwIPPrivateHostTimer, nullptr);
#endif

    //
    // Setup a timeout for the link detect callback function if using a RTOS.
    //
#if !NO_SYS && (LWIP_AUTOIP || LWIP_DHCP)
    sys_timeout(LINK_TMR_INTERVAL, lwIPPrivateLinkTimer, nullptr);
#endif
}

//*****************************************************************************
//
//! Initializes the lwIP TCP/IP stack.
//!
//! \param system_clock_frequency is the current system clock rate in Hz.
//! \param mac_address is a pointer to a six byte array containing the MAC
//! address to be used for the interface.
//! \param ip_address is the IP address to be used (static).
//! \param netmask is the network mask to be used (static).
//! \param gateway is the Gateway address to be used (static).
//! \param ip_mode is the IP Address Mode.  \b IPADDR_USE_STATIC will force
//! static IP addressing to be used, \b IPADDR_USE_DHCP will force DHCP with
//! fallback to Link Local (Auto IP), while \b IPADDR_USE_AUTOIP will force
//! Link Local only.
//!
//! This function performs initialization of the lwIP TCP/IP stack for the
//! Ethernet MAC, including DHCP and/or AutoIP, as configured.
//!
//! \return None.
//
//*****************************************************************************
void lwip_configure(uint32_t system_clock_frequency,
                    const uint8_t *mac_address,
                    uint32_t ip_address,
                    uint32_t netmask,
                    uint32_t gateway,
                    uint32_t ip_mode) {
    //
    // Check the parameters.
    //
#if LWIP_DHCP && LWIP_AUTOIP
    ASSERT((ip_mode == IPADDR_USE_STATIC) || (ip_mode == IPADDR_USE_DHCP) || (ip_mode == IPADDR_USE_AUTOIP));
#elif LWIP_DHCP
    ASSERT((ip_mode == IPADDR_USE_STATIC) || (ip_mode == IPADDR_USE_DHCP));
#elif LWIP_AUTOIP
    ASSERT((ip_mode == IPADDR_USE_STATIC) || (ip_mode == IPADDR_USE_AUTOIP));
#else
    ASSERT(ip_mode == IPADDR_USE_STATIC);
#endif

    //
    // Enable the ethernet peripheral.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_EMAC0);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_EMAC0);

    //
    // Enable the internal PHY if it's present and we're being
    // asked to use it.
    //
    if ((EMAC_PHY_CONFIG & EMAC_PHY_TYPE_MASK) == EMAC_PHY_TYPE_INTERNAL) {
        //
        // We've been asked to configure for use with the internal
        // PHY.  Is it present?
        //
        if (MAP_SysCtlPeripheralPresent(SYSCTL_PERIPH_EPHY0)) {
            //
            // Yes - enable and reset it.
            //
            MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_EPHY0);
            MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_EPHY0);
        } else {
            //
            // Internal PHY is not present on this part so hang here.
            //
            while (1) {
            }
        }
    }

    //
    // Wait for the MAC to come out of reset.
    //
    while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_EMAC0)) {
    }

#ifdef PART_TM4C1292NCPDT

    //    Enable all MII related pins
    //
    // Enable Peripheral Clocks
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    // Configure the GPIO Pin Mux for PP0
    // for EN0INTRN
    MAP_GPIOPinConfigure(GPIO_PP0_EN0INTRN);
    GPIOPinTypeEthernetMII(GPIO_PORTP_BASE, GPIO_PIN_0);
    // Configure the GPIO Pin Mux for PK4
    // for EN0RXD3
    MAP_GPIOPinConfigure(GPIO_PK4_EN0RXD3);
    GPIOPinTypeEthernetMII(GPIO_PORTK_BASE, GPIO_PIN_4);
    // Configure the GPIO Pin Mux for PK6
    // for EN0TXD2
    MAP_GPIOPinConfigure(GPIO_PK6_EN0TXD2);
    GPIOPinTypeEthernetMII(GPIO_PORTK_BASE, GPIO_PIN_6);
    // Configure the GPIO Pin Mux for PG5
    // for EN0TXD1
    MAP_GPIOPinConfigure(GPIO_PG5_EN0TXD1);
    GPIOPinTypeEthernetMII(GPIO_PORTG_BASE, GPIO_PIN_5);
    // Configure the GPIO Pin Mux for PA6
    // for EN0RXCK
    MAP_GPIOPinConfigure(GPIO_PA6_EN0RXCK);
    GPIOPinTypeEthernetMII(GPIO_PORTA_BASE, GPIO_PIN_6);
    // Configure the GPIO Pin Mux for PG7
    // for EN0RXDV
    MAP_GPIOPinConfigure(GPIO_PG7_EN0RXDV);
    GPIOPinTypeEthernetMII(GPIO_PORTG_BASE, GPIO_PIN_7);
    // Configure the GPIO Pin Mux for PG3
    // for EN0TXEN
    MAP_GPIOPinConfigure(GPIO_PG3_EN0TXEN);
    GPIOPinTypeEthernetMII(GPIO_PORTG_BASE, GPIO_PIN_3);
    // Configure the GPIO Pin Mux for PQ5
    // for EN0RXD0
    MAP_GPIOPinConfigure(GPIO_PQ5_EN0RXD0);
    GPIOPinTypeEthernetMII(GPIO_PORTQ_BASE, GPIO_PIN_5);
    // Configure the GPIO Pin Mux for PM6
    // for EN0CRS
    MAP_GPIOPinConfigure(GPIO_PM6_EN0CRS);
    GPIOPinTypeEthernetMII(GPIO_PORTM_BASE, GPIO_PIN_6);
    // Configure the GPIO Pin Mux for PK5
    // for EN0RXD2
    MAP_GPIOPinConfigure(GPIO_PK5_EN0RXD2);
    GPIOPinTypeEthernetMII(GPIO_PORTK_BASE, GPIO_PIN_5);
    // Configure the GPIO Pin Mux for PG4
    // for EN0TXD0
    MAP_GPIOPinConfigure(GPIO_PG4_EN0TXD0);
    GPIOPinTypeEthernetMII(GPIO_PORTG_BASE, GPIO_PIN_4);
    // Configure the GPIO Pin Mux for PG2
    // for EN0TXCK
    MAP_GPIOPinConfigure(GPIO_PG2_EN0TXCK);
    GPIOPinTypeEthernetMII(GPIO_PORTG_BASE, GPIO_PIN_2);
    // Configure the GPIO Pin Mux for PQ6
    // for EN0RXD1
    MAP_GPIOPinConfigure(GPIO_PQ6_EN0RXD1);
    GPIOPinTypeEthernetMII(GPIO_PORTQ_BASE, GPIO_PIN_6);
    // Configure the GPIO Pin Mux for PK7
    // for EN0TXD3
    MAP_GPIOPinConfigure(GPIO_PK7_EN0TXD3);
    GPIOPinTypeEthernetMII(GPIO_PORTK_BASE, GPIO_PIN_7);
    // Configure the GPIO Pin Mux for PM7
    // for EN0COL
    MAP_GPIOPinConfigure(GPIO_PM7_EN0COL);
    GPIOPinTypeEthernetMII(GPIO_PORTM_BASE, GPIO_PIN_7);

#endif

    //
    // Configure for use with whichever PHY the user requires.
    //
    EMACPHYConfigSet(EMAC0_BASE, EMAC_PHY_CONFIG);

    //
    // Initialize the MAC and set the DMA mode.
    //
    MAP_EMACInit(EMAC0_BASE, system_clock_frequency, EMAC_BCONFIG_MIXED_BURST | EMAC_BCONFIG_PRIORITY_FIXED, 4, 4, 0);

    //
    // Set MAC configuration options.
    //
    MAP_EMACConfigSet(
        EMAC0_BASE,
        (EMAC_CONFIG_FULL_DUPLEX | EMAC_CONFIG_CHECKSUM_OFFLOAD | EMAC_CONFIG_7BYTE_PREAMBLE | EMAC_CONFIG_IF_GAP_96BITS |
         EMAC_CONFIG_USE_MACADDR0 | EMAC_CONFIG_SA_FROM_DESCRIPTOR | EMAC_CONFIG_BO_LIMIT_1024),
        (EMAC_MODE_RX_STORE_FORWARD | EMAC_MODE_TX_STORE_FORWARD | EMAC_MODE_TX_THRESHOLD_64_BYTES | EMAC_MODE_RX_THRESHOLD_64_BYTES),
        0);

    //
    // Program the hardware with its MAC address (for filtering).
    //
    MAP_EMACAddrSet(EMAC0_BASE, 0, (uint8_t *)mac_address);

    //
    // Save the network configuration for later use by the private
    // initialization.
    //
    g_ip_mode    = ip_mode;
    g_ip_address = ip_address;
    g_netmask    = netmask;
    g_gateway    = gateway;

    //
    // Initialize lwIP.  The remainder of initialization is done immediately if
    // not using a RTOS and it is deferred to the TCP/IP thread's context if
    // using a RTOS.
    //
#if NO_SYS
    s_lwip_initialize(nullptr);
#else
    tcpip_init(lwIPPrivateInit, 0);
#endif
}

//*****************************************************************************
//
//! Registers an interrupt callback function to handle the IEEE-1588 timer.
//!
//! \param timer_callback points to a function which is called whenever the
//! Ethernet MAC reports an interrupt relating to the IEEE-1588 hardware timer.
//!
//! This function allows an application to register a handler for all
//! interrupts generated by the IEEE-1588 hardware timer in the Ethernet MAC.
//! To allow minimal latency timer handling, the callback function provided
//! will be called in interrupt context, regardless of whether or not lwIP is
//! configured to operate with an RTOS.  In an RTOS environment, the callback
//! function is responsible for ensuring that all processing it performs is
//! compatible with the low level interrupt context it is called in.
//!
//! The callback function takes two parameters.  The first is the base address
//! of the MAC reporting the timer interrupt and the second is the timer
//! interrupt status as reported by EMACTimestampIntStatus().  Note that
//! EMACTimestampIntStatus() causes the timer interrupt sources to be cleared
//! so the application should not call EMACTimestampIntStatus() within the
//! handler.
//!
//! \return None.
//
//*****************************************************************************
void lwip_register_timer_callback(tHardwareTimerHandler timer_callback) {
    //
    // Remember the callback function address passed.
    //
    g_lwip_timestamp_handler = timer_callback;
}

//*****************************************************************************
//
//! Handles periodic timer events for the lwIP TCP/IP stack.
//!
//! \param system_time is the incremental time for this periodic interrupt.
//!
//! This function will update the local timer by the value in \e ui32TimeMS.
//! If the system is configured for use without an RTOS, an Ethernet interrupt
//! will be triggered to allow the lwIP periodic timers to be serviced in the
//! Ethernet interrupt.
//!
//! \return None.
//
//*****************************************************************************
#if NO_SYS
void lwip_timer(uint32_t system_time) {
    //
    // Increment the lwIP Ethernet timer.
    //
    g_lwip_local_timer = system_time;

    //
    // Generate an Ethernet interrupt.  This will perform the actual work
    // of checking the lwIP timers and taking the appropriate actions.  This is
    // needed since lwIP is not re-entrant, and this allows all lwIP calls to
    // be placed inside the Ethernet interrupt handler ensuring that all calls
    // into lwIP are coming from the same context, preventing any reentrancy
    // issues.  Putting all the lwIP calls in the Ethernet interrupt handler
    // avoids the use of mutexes to avoid re-entering lwIP.
    //

    sys_check_timeouts();

    // receive_poll(&g_netif);

    //HWREG(NVIC_SW_TRIG) |= INT_EMAC0 - 16;
}
#endif

//*****************************************************************************
//
//! Handles Ethernet interrupts for the lwIP TCP/IP stack.
//!
//! This function handles Ethernet interrupts for the lwIP TCP/IP stack.  At
//! the lowest level, all receive packets are placed into a packet queue for
//! processing at a higher level.  Also, the transmit packet queue is checked
//! and packets are drained and transmitted through the Ethernet MAC as needed.
//! If the system is configured without an RTOS, additional processing is
//! performed at the interrupt level.  The packet queues are processed by the
//! lwIP TCP/IP code, and lwIP periodic timers are serviced (as needed).
//!
//! \return None.
//
//*****************************************************************************
void lwip_ethernet_interrupt_handler(void) {
    uint32_t interrupt_status;
    uint32_t timer_status;
#if !NO_SYS
    portBASE_TYPE xWake;
#endif

    //
    // Read and Clear the interrupt.
    //
    interrupt_status = EMACIntStatus(EMAC0_BASE, true);

#if EEE_SUPPORT
    if (ui32Status & EMAC_INT_LPI) {
        EMACLPIStatus(EMAC0_BASE);
    }
#endif

    //
    // If the PMT mode exit status bit is set then enable the MAC transmit
    // and receive paths, read the PMT status to clear the interrupt and
    // clear the interrupt flag.
    //
    if (interrupt_status & EMAC_INT_POWER_MGMNT) {
        MAP_EMACTxEnable(EMAC0_BASE);
        MAP_EMACRxEnable(EMAC0_BASE);

        EMACPowerManagementStatusGet(EMAC0_BASE);

        interrupt_status &= ~(EMAC_INT_POWER_MGMNT);
    }

    //
    // If the interrupt really came from the Ethernet and not our
    // timer, clear it.
    //
    if (interrupt_status) {
        MAP_EMACIntClear(EMAC0_BASE, interrupt_status);
    }

    //
    // Check to see whether a hardware timer interrupt has been reported.
    //
    if (interrupt_status & EMAC_INT_TIMESTAMP) {
        //
        // Yes - read and clear the timestamp interrupt status.
        //
        timer_status = EMACTimestampIntStatus(EMAC0_BASE);

        //
        // If a timer interrupt handler has been registered, call it.
        //
        if (g_lwip_timestamp_handler) {
            g_lwip_timestamp_handler(EMAC0_BASE, timer_status);
        }
    }

    //
    // The handling of the interrupt is different based on the use of a RTOS.
    //
#if NO_SYS
    //
    // No RTOS is being used.  If a transmit/receive interrupt was active,
    // run the low-level interrupt handler.
    //
    if (interrupt_status) {
        tivaif_interrupt(&g_netif, interrupt_status);
    }

    //
    // Service the lwIP timers.
    //
#else
    //
    // A RTOS is being used.  Signal the Ethernet interrupt task.
    //
    xQueueSendFromISR(g_pInterrupt, (void *)&ui32Status, &xWake);

    //
    // Disable the Ethernet interrupts.  Since the interrupts have not been
    // handled, they are not asserted.  Once they are handled by the Ethernet
    // interrupt task, it will re-enable the interrupts.
    //
    MAP_EMACIntDisable(
        EMAC0_BASE,
        (EMAC_INT_RECEIVE | EMAC_INT_TRANSMIT | EMAC_INT_TX_STOPPED | EMAC_INT_RX_NO_BUFFER | EMAC_INT_RX_STOPPED | EMAC_INT_PHY));

    //
    // Potentially task switch as a result of the above queue write.
    //
    #if RTOS_FREERTOS
    if (xWake == pdTRUE) {
        portYIELD_FROM_ISR(true);
    }
    #endif
#endif
}

//*****************************************************************************
//
//! Returns the IP address for this interface.
//!
//! This function will read and return the currently assigned IP address for
//! the Stellaris Ethernet interface.
//!
//! \return Returns the assigned IP address for this interface.
//
//*****************************************************************************
uint32_t lwip_get_local_ip(void) {
#if LWIP_AUTOIP || LWIP_DHCP
    if (g_lwip_link_active) {
        return ((uint32_t)g_netif.ip_addr.addr);
    }
    return 0xffffffff;
#else
    return ((uint32_t)g_sNetIF.ip_addr.addr);
#endif
}

//*****************************************************************************
//
//! Returns the network mask for this interface.
//!
//! This function will read and return the currently assigned network mask for
//! the Stellaris Ethernet interface.
//!
//! \return the assigned network mask for this interface.
//
//*****************************************************************************
uint32_t lwip_get_local_mask(void) {
    return ((uint32_t)g_netif.netmask.addr);
}

//*****************************************************************************
//
//! Returns the gateway address for this interface.
//!
//! This function will read and return the currently assigned gateway address
//! for the Stellaris Ethernet interface.
//!
//! \return the assigned gateway address for this interface.
//
//*****************************************************************************
uint32_t lwip_get_local_gateway(void) {
    return ((uint32_t)g_netif.gw.addr);
}

//*****************************************************************************
//
//! Returns the local MAC/HW address for this interface.
//!
//! \param mac_address is a pointer to an array of bytes used to store the MAC
//! address.
//!
//! This function will read the currently assigned MAC address into the array
//! passed in \e mac_address.
//!
//! \return None.
//
//*****************************************************************************
void lwip_get_local_mac(uint8_t *mac_address) {
    MAP_EMACAddrGet(EMAC0_BASE, 0, mac_address);
}

//*****************************************************************************
//
// Completes the network configuration change.  This is directly called when
// not using a RTOS and provided as a callback to the TCP/IP thread when using
// a RTOS.
//
//*****************************************************************************
static void s_lwip_network_config_changed(void *arg) {
    uint32_t ip_mode;
    ip4_addr_t ip_addr;
    ip4_addr_t net_mask;
    ip4_addr_t gw_addr;

    //
    // Get the new address mode.
    //
    ip_mode = (uint32_t)arg;

    //
    // Setup the network address values.
    //
    if (ip_mode == IPADDR_USE_STATIC) {
        ip_addr.addr  = htonl(g_ip_address);
        net_mask.addr = htonl(g_netmask);
        gw_addr.addr  = htonl(g_gateway);
    }
#if LWIP_DHCP || LWIP_AUTOIP
    else {
        ip_addr.addr  = 0;
        net_mask.addr = 0;
        gw_addr.addr  = 0;
    }
#endif

    //
    // Switch on the current IP Address Aquisition mode.
    //
    switch (g_ip_mode) {
        //
        // Static IP
        //
        case IPADDR_USE_STATIC: {
            //
            // Set the new address parameters.  This will change the address
            // configuration in lwIP, and if necessary, will reset any links
            // that are active.  This is valid for all three modes.
            //
            netif_set_addr(&g_netif, &ip_addr, &net_mask, &gw_addr);

            //
            // If we are going to DHCP mode, then start the DHCP server now.
            //
#if LWIP_DHCP
            if ((ip_mode == IPADDR_USE_DHCP) && g_lwip_link_active) {
                dhcp_start(&g_netif);
            }
#endif

            //
            // If we are going to AutoIP mode, then start the AutoIP process
            // now.
            //
#if LWIP_AUTOIP
            if ((ip_mode == IPADDR_USE_AUTOIP) && g_bLinkActive) {
                autoip_start(&g_sNetIF);
            }
#endif

            //
            // And we're done.
            //
            break;
        }

        //
        // DHCP (with AutoIP fallback).
        //
#if LWIP_DHCP
        case IPADDR_USE_DHCP: {
            //
            // If we are going to static IP addressing, then disable DHCP and
            // force the new static IP address.
            //
            if (ip_mode == IPADDR_USE_STATIC) {
                dhcp_stop(&g_netif);
                netif_set_addr(&g_netif, &ip_addr, &net_mask, &gw_addr);
            }
            //
            // If we are going to AUTO IP addressing, then disable DHCP, set
            // the default addresses, and start AutoIP.
            //
    #if LWIP_AUTOIP
            else if (ip_mode == IPADDR_USE_AUTOIP) {
                dhcp_stop(&g_sNetIF);
                netif_set_addr(&g_sNetIF, &ip_addr, &net_mask, &gw_addr);
                if (g_bLinkActive) {
                    autoip_start(&g_sNetIF);
                }
            }
    #endif
            break;
        }
#endif

        //
        // AUTOIP
        //
#if LWIP_AUTOIP
        case IPADDR_USE_AUTOIP: {
            //
            // If we are going to static IP addressing, then disable AutoIP and
            // force the new static IP address.
            //
            if (ip_mode == IPADDR_USE_STATIC) {
                autoip_stop(&g_sNetIF);
                netif_set_addr(&g_sNetIF, &ip_addr, &net_mask, &gw_addr);
            }
            //
            // If we are going to DHCP addressing, then disable AutoIP, set the
            // default addresses, and start dhcp.
            //
    #if LWIP_DHCP
            else if (ip_mode == IPADDR_USE_DHCP) {
                autoip_stop(&g_sNetIF);
                netif_set_addr(&g_sNetIF, &ip_addr, &net_mask, &gw_addr);
                if (g_bLinkActive) {
                    dhcp_start(&g_sNetIF);
                }
            }
    #endif
            break;
        }
#endif
        default: break;
    }

    //
    // Bring the interface up.
    //
    netif_set_up(&g_netif);

    //
    // Save the new mode.
    //
    g_ip_mode = ip_mode;
}

//*****************************************************************************
//
//! Change the configuration of the lwIP network interface.
//!
//! \param ip_address is the new IP address to be used (static).
//! \param netmask is the new network mask to be used (static).
//! \param gateway is the new Gateway address to be used (static).
//! \param ip_mode is the IP Address Mode.  \b IPADDR_USE_STATIC 0 will
//! force static IP addressing to be used, \b IPADDR_USE_DHCP will force DHCP
//! with fallback to Link Local (Auto IP), while \b IPADDR_USE_AUTOIP will
//! force Link Local only.
//!
//! This function will evaluate the new configuration data.  If necessary, the
//! interface will be brought down, reconfigured, and then brought back up
//! with the new configuration.
//!
//! \return None.
//
//*****************************************************************************
void lwip_change_network_config(uint32_t ip_address, uint32_t netmask, uint32_t gateway, uint32_t ip_mode) {
    //
    // Check the parameters.
    //
#if LWIP_DHCP && LWIP_AUTOIP
    ASSERT((ip_mode == IPADDR_USE_STATIC) || (ip_mode == IPADDR_USE_DHCP) || (ip_mode == IPADDR_USE_AUTOIP));
#elif LWIP_DHCP
    ASSERT((ip_mode == IPADDR_USE_STATIC) || (ip_mode == IPADDR_USE_DHCP));
#elif LWIP_AUTOIP
    ASSERT((ip_mode == IPADDR_USE_STATIC) || (ip_mode == IPADDR_USE_AUTOIP));
#else
    ASSERT(ip_mode == IPADDR_USE_STATIC);
#endif

    //
    // Save the network configuration for later use by the private network
    // configuration change.
    //
    g_ip_address = ip_address;
    g_netmask    = netmask;
    g_gateway    = gateway;

    //
    // Complete the network configuration change.  The remainder is done
    // immediately if not using a RTOS and it is deferred to the TCP/IP
    // thread's context if using a RTOS.
    //
#if NO_SYS
    s_lwip_network_config_changed((void *)ip_mode);
#else
    tcpip_callback(lwIPPrivateNetworkConfigChange, (void *)ip_mode);
#endif
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
