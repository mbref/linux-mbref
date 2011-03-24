/*
 * Linux driver for the s2imac device.
 *
 * Author: Roman Wagner rw@sensortoimage.de
 *
 * This is a flat driver which is based on the emac_lite and lltemac
 * driver from John Williams <john.williams@petalogix.com> and Xilinx, Inc.
 *
 * Copyright (C) 2010 Sensor to image GmbH
 * October 2010 created
 *
 * Copyright (C) 2011 Li-Pro.Net, Stephan Linz <linz@li-pro.net>
 * January 2011 code improvements and coding style corrections
 * February 2001 change over to an interrupt driven network driver
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/io.h>

/* for SIOS2I stuff */
#include <linux/if_s2i.h>

#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>

#define DRIVER_NAME "s2imac"
#define DRIVER_VERS "1.0"

/* MDIO register basis */
#define MDIO_BASE		(lp->base_addr[S2IMAC_NETMAP] + 0x1000)

/* Tx/Rx buffer basis */
#define TXBUF     		(lp->base_addr[S2IMAC_NETMAP] + 0x4000)
#define RXBUF     		(lp->base_addr[S2IMAC_NETMAP] + 0x8000)

/* MAC registers definition */
#define RCW1			(lp->base_addr[S2IMAC_NETMAP] + (0x240 << 2))
#define TC			(lp->base_addr[S2IMAC_NETMAP] + (0x280 << 2))
#define EMMC			(lp->base_addr[S2IMAC_NETMAP] + (0x300 << 2))
#define MC			(lp->base_addr[S2IMAC_NETMAP] + (0x340 << 2))
#define UAW0			(lp->base_addr[S2IMAC_NETMAP] + (0x380 << 2))
#define UAW1			(lp->base_addr[S2IMAC_NETMAP] + (0x384 << 2))
#define AFM			(lp->base_addr[S2IMAC_NETMAP] + (0x390 << 2))

#define MDIO_RXTX_JUMBO		(1 << 30)
#define MDIO_RXTX_ENABLE	(1 << 28)
#define MDIO_RXTX_HALFDUPLEX	(1 << 26)

#define MDIO_EMMC_10BASET	(0)
#define MDIO_EMMC_100BASET	(1 << 30)
#define MDIO_EMMC_1000BASET	(1 << 31)

#define MDIO_ENABLE_MASK	(1 << 6)
#define MDIO_CLOCK_DIV_MASK	((1 << 6) - 1)

/* direct registers definition */
#define GCSR    		(lp->base_addr[S2IMAC_NETMAP] + 0xC000)
#define CLK_FREQ		(lp->base_addr[S2IMAC_NETMAP] + 0xC008)
#define TOCNT_DIV		(lp->base_addr[S2IMAC_NETMAP] + 0xC014)
#define MDIO_ACC		(lp->base_addr[S2IMAC_NETMAP] + 0xC028)
#define ETHSIZE  		(lp->base_addr[S2IMAC_NETMAP] + 0xC02C)
#define MAC_HIGH 		(lp->base_addr[S2IMAC_NETMAP] + 0xC030)
#define MAC_LOW  		(lp->base_addr[S2IMAC_NETMAP] + 0xC034)
#define TX_LEN  		(lp->base_addr[S2IMAC_NETMAP] + 0xC048)
#define RX_LEN   		(lp->base_addr[S2IMAC_NETMAP] + 0xC04C)
#define MAC_RX_HIGH		(lp->base_addr[S2IMAC_NETMAP] + 0xC090)
#define MAC_RX_LOW		(lp->base_addr[S2IMAC_NETMAP] + 0xC094)

#ifndef CONFIG_S2IMAC_POLLING
#define INT_MASK		(lp->base_addr[S2IMAC_NETMAP] + 0xC0AC)
#define INT_REQ 		(lp->base_addr[S2IMAC_NETMAP] + 0xC0B0)
#endif

#define GCSR_CONNECTED		(1)
#define GCSR_RST_PHY		(1 << 2)

#define MDIO_ACC_ENA_NBLOCK	(1 << 31)
#define MDIO_ACC_NBLOCK		(1 << 16)
#define MDIO_ACC_MIIRDY		(1 << 17)

#ifndef CONFIG_S2IMAC_POLLING
#define INT_ENABLE_RX		(1)
#define INT_ENABLE_TX		(1 << 1)
#define INT_ENABLE_IRQS		(1 << 31)
#endif

/* max MTU size of an ... */
#define XTE_MTU			1500	/* Ethernet frame */
#define XTE_JUMBO_MTU		8982	/* jumbo Ethernet frame */
#define XTE_HDR_SIZE		14	/* Ethernet header */
#define XTE_HDR_VLAN_SIZE	18	/* Ethernet header with VLAN */
#define XTE_TRL_SIZE		4	/* Ethernet trailer (FCS) */
#define XTE_MAX_FRAME_SIZE	(XTE_MTU + XTE_HDR_SIZE + XTE_TRL_SIZE)
#define XTE_MAX_VLAN_FRAME_SIZE	(XTE_MTU + XTE_HDR_VLAN_SIZE + XTE_TRL_SIZE)
#define XTE_MAX_JUMBO_FRAME_SIZE (XTE_JUMBO_MTU + XTE_HDR_SIZE + XTE_TRL_SIZE)

/* Tx timeout is 60 seconds. */
#define TX_TIMEOUT		(60*HZ)

/* BUFFER_ALIGN(adr) calculates the number of bytes to the next alignment. */
#define ALIGNMENT		4
#define BUFFER_ALIGN(adr)	((ALIGNMENT - ((u32) adr)) % ALIGNMENT)

volatile u32 *gige_txbuf;
volatile u32 *gige_rxbuf;

/**
 * struct net_local - Our private per device data
 * @ndev:		instance of the network device
 * @base_addr[]:	base address of the s2imac device
 * @coreclk:		ip core clock frequency in Hz
 * @regmem_end:		physical end of GigEV register bank
 * @regmem_start:	physical start of GigEV register bank
 * @reset_lock:		lock used for synchronization
 * @deferred_skb:	holds an skb (for transmission at a later time) when the
 *			Tx buffer is not free
 * @phy_dev:		pointer to the PHY device
 * @phy_node:		pointer to the PHY device node
 * @last_link:		last link status
 * @cur_speed:		current speed
 */
struct net_local {

	struct net_device *ndev;

#define S2IMAC_NETMAP	0
#define S2IMAC_REGMAP	1
#define MAX_S2IMAC_MAPS 2
	void __iomem *base_addr[MAX_S2IMAC_MAPS];
	unsigned int coreclk;

	unsigned long regmem_end;
	unsigned long regmem_start;

	spinlock_t reset_lock;
	struct sk_buff *deferred_skb;

	/* FIXME: use mdio bus: struct phy_device *phy_dev; */
	/* FIXME: use mdio bus: struct device_node *phy_node; */
	u32 phy_addr;		/* FIXME: really global, use mdio bus */

	int last_link;
	int cur_speed;

#ifdef CONFIG_S2IMAC_POLLING
	struct timer_list irq_timer;	/* ISR polling timer */
#endif

	struct timer_list phy_timer;	/* PHY monitoring timer */
};

/*************************/
/* s2imac driver calls */
/*************************/

#ifndef CONFIG_S2IMAC_POLLING
/**
 * s2imac_enable_interrupts - Enable the interrupts for the s2imac device
 * @drvdata:	Pointer to the s2imac device private data
 *
 * This function enables the Tx and Rx interrupts for the s2imac device along
 * with the Global Interrupt Enable.
 */
static void s2imac_enable_interrupts(struct net_local *lp)
{
	u32 reg_data;

	/* get interrput mask register */
	reg_data = in_be32((u32 *) INT_MASK);

	/* Enable the Tx interrupts, enable Rx interrupts and enable the global interrupt */
	out_be32 ((u32 *) INT_MASK, reg_data | INT_ENABLE_TX | INT_ENABLE_RX | INT_ENABLE_IRQS);
}

/**
 * s2imac_disable_interrupts - Disable the interrupts for the s2imac device
 * @drvdata:	Pointer to the s2imac device private data
 *
 * This function disables the Tx and Rx interrupts for the s2imac device,
 * along with the Global Interrupt Enable.
 */
static void s2imac_disable_interrupts(struct net_local *lp)
{
	u32 reg_data;

	/* get interrput mask register */
	reg_data = in_be32((u32 *) INT_MASK);

	/* disable the Tx interrupts, disable Rx interrupts and disable the global interrupt */
	out_be32 ((u32 *) INT_MASK, reg_data & (~INT_ENABLE_TX) & (~INT_ENABLE_RX) & (~INT_ENABLE_IRQS));
}
#endif

/* Read Ethernet PHY register */
static u16 s2imac_phy_read (struct net_local *lp, u8 reg_addr)
{
	u32 ret;

	if (in_be32 ((u32 *) MDIO_ACC) & MDIO_ACC_NBLOCK) {
		/*
		 * Non-blocking mode
		 * (only this software thread waits for MDIO access)
		 */
		ret = in_be32 ((u32 *) (MDIO_BASE + ((lp->phy_addr & 0x1F) << 7)
					+ ((reg_addr & 0x1F) << 2)));

		/*
		 * Wait here polling, until the value is ready to be read.
		 * Should we avoid endless loop due to hardware?
		 */
		do {
			ret = in_be32 ((u32 *) MDIO_ACC);
		} while (!(ret & MDIO_ACC_MIIRDY));
	} else {
		/*
		 * Blocking mode
		 * (whole system waits for the bus transaction to finish)
		 */
		ret = in_be32 ((u32 *) (MDIO_BASE + ((lp->phy_addr & 0x1F) << 7)
					+ ((reg_addr & 0x1F) << 2)));
	}
	return (u16) (ret & 0xFFFF);
}

/* Write Ethernet PHY register */
static void s2imac_phy_write (struct net_local *lp, u8 reg_addr, u16 reg_data)
{
	u32 ret;

	if (in_be32 ((u32 *) MDIO_ACC) & MDIO_ACC_NBLOCK) {
		/*
		 * Non-blocking mode
		 * (only this software thread waits for MDIO access)
		 */
		out_be32 ((u32 *) (MDIO_BASE + (lp->phy_addr << 7)
				   + ((reg_addr) << 2)), reg_data);

		/*
		 * Wait here polling, until the value is ready written.
		 * Should we avoid endless loop due to hardware?
		 */
		do {
			ret = in_be32 ((u32 *) MDIO_ACC);
		} while (!(ret & MDIO_ACC_MIIRDY));
	} else {
		/*
		 * Blocking mode
		 * (whole system waits for the bus transaction to finish)
		 */
		out_be32 ((u32 *) (MDIO_BASE + (lp->phy_addr << 7)
				   + ((reg_addr) << 2)), reg_data);
	}
}

/*
 * Detect the PHY address by scanning addresses 0 to 31 and
 * looking at the MII status register (register 1) and assuming
 * the PHY supports 10Mbps full/half duplex. Feel free to change
 * this code to match your PHY, or hardcode the address if needed.
 *
 * Use MII register 1 (MII status register) to detect PHY
 *
 * Mask used to verify certain PHY features (or register contents)
 * in the register above:
 *  0x1000: 10Mbps full duplex support
 *  0x0800: 10Mbps half duplex support
 *  0x0008: Auto-negotiation support
 */
#define PHY_DETECT_REG		1
#define PHY_DETECT_MASK		0x1808
static int detect_phy (struct net_local *lp, struct device *dev)
{
	u16 phy_reg;

	for (lp->phy_addr = 31; lp->phy_addr > 0; lp->phy_addr--) {
		phy_reg = s2imac_phy_read (lp, PHY_DETECT_REG);
		if ((phy_reg != 0xFFFF) && ((phy_reg & PHY_DETECT_MASK)
					    == PHY_DETECT_MASK)) {
			/* Found a valid PHY address */
			dev_info (dev, "PHY detected at address %d.\n",
				  lp->phy_addr);
			return lp->phy_addr;
		}
	}

	/* default to zero */
	dev_info (dev, "No PHY detected. Assuming a PHY at address  %d.\n", lp->phy_addr);
	return lp->phy_addr;
}

static void set_mac_speed (struct net_local *lp)
{
	u32 cfg, rxtx;
	struct net_device *ndev = lp->ndev;

	switch ((s2imac_phy_read (lp, 0x11)) & 0xE000) {
	case 0x0000:
		/* 10BASE-T, half-duplex */
		cfg = MDIO_EMMC_10BASET;
		rxtx = (MDIO_RXTX_ENABLE | MDIO_RXTX_HALFDUPLEX);
		lp->cur_speed = 10;
		dev_info (&ndev->dev, "speed set to 10BASE-T/HD\n");
		break;
	case 0x2000:
		/* 10BASE-T, full-duplex */
		cfg = MDIO_EMMC_10BASET;
		rxtx = MDIO_RXTX_ENABLE;
		lp->cur_speed = 10;
		dev_info (&ndev->dev, "speed set to 10BASE-T/FD\n");
		break;
	case 0x4000:
		/* 100BASE-TX, half-duplex */
		cfg = MDIO_EMMC_100BASET;
		rxtx = (MDIO_RXTX_ENABLE | MDIO_RXTX_HALFDUPLEX);
		lp->cur_speed = 100;
		dev_info (&ndev->dev, "speed set to 100BASE-T/HD\n");
		break;
	case 0x6000:
		/* 100BASE-TX, full-duplex */
		cfg = MDIO_EMMC_100BASET;
		rxtx = MDIO_RXTX_ENABLE;
		lp->cur_speed = 100;
		dev_info (&ndev->dev, "speed set to 100BASE-T/FD\n");
		break;
	case 0x8000:
		/* 1000BASE-T, half-duplex */
		cfg = MDIO_EMMC_1000BASET;
		rxtx = (MDIO_RXTX_ENABLE | MDIO_RXTX_HALFDUPLEX);
		lp->cur_speed = 1000;
		dev_info (&ndev->dev, "speed set to 1000BASE-T/HD\n");
		break;
	case 0xA000:
		/* 1000BASE-T, full-duplex */
		cfg = MDIO_EMMC_1000BASET;
		rxtx = MDIO_RXTX_ENABLE;
		lp->cur_speed = 1000;
		dev_info (&ndev->dev, "speed set to 1000BASE-T/FD\n");
		break;
	default:
		return;
	}

	out_be32 ((u32 *) EMMC, cfg);
	/* Enable jumbo frames for Tx and Rx */
	out_be32 ((u32 *) TC, rxtx | MDIO_RXTX_JUMBO);
	out_be32 ((u32 *) RCW1, rxtx | MDIO_RXTX_JUMBO);
}

/*
 * Perform any necessary special phy setup. In the gmii case, nothing needs to
 * be done.
 */
static void phy_setup (struct net_local *lp)
{
	u32 reg;
	unsigned retries = 10;

	/* non-blocking PHY access enabled (if supported) */
	out_be32 ((u32 *) MDIO_ACC, MDIO_ACC_ENA_NBLOCK);

	/* reset the PHY
	   reg = in_be32 ((u32 *) GCSR);
	   reg |= GCSR_RST_PHY;
	   out_be32 ((u32 *) GCSR, reg); */

	/* wait for end of PHY reset */
	do {
		reg = in_be32 ((u32 *) GCSR);
	} while (reg & GCSR_RST_PHY);

	/*
	 * Setup timers and clock generators
	 *   - timeout counter clock period 1 ms
	 *   - PHY MDC frequency (max 2.5 MHz according to IEEE Std 802.3-2002,
	 *     BCM5461A supports 12.5 MHz)
	 */
#define MDIO_CLOCK	(2500000)	/* 2.5 MHz */
#define MDIO_CLOCK_DIV	(MDIO_CLOCK_DIV_MASK & \
			((lp->coreclk / (2 * MDIO_CLOCK)) - 1))

	out_be32 ((u32 *) CLK_FREQ, lp->coreclk);
	out_be32 ((u32 *) TOCNT_DIV, (lp->coreclk / 1000) - 1);
	out_be32 ((u32 *) MC, MDIO_ENABLE_MASK | MDIO_CLOCK_DIV);

	/* wait for link up */
	while (retries-- && ((s2imac_phy_read (lp, 1) & 0x24) != 0x24)) ;

	/* enable receive and transmit clients */
	reg = in_be32 ((u32 *) GCSR);
	reg |= GCSR_CONNECTED;
	out_be32 ((u32 *) GCSR, reg);
}

/*
 * The PHY registers read here should be standard registers in all PHY chips
 */
static int get_phy_status (struct net_device *ndev, int *linkup)
{
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);
	u16 reg;

	reg = s2imac_phy_read (lp, MII_BMSR);
	*linkup = (reg & BMSR_LSTATUS) != 0;

	return 0;
}

/*
 * This routine is used for two purposes. The first is to keep Xilinx
 * EMAC's duplex setting in sync with the PHY's. The second is to keep
 * the system apprised of the state of the link. Note that this driver
 * does not configure the PHY. Either the PHY should be configured for
 * auto-negotiation or it should be handled by something like mii-tool.
 */
static void poll_gmii (unsigned long data)
{
	struct net_device *ndev;
	struct net_local *lp;
	int phy_carrier;
	int netif_carrier;
	u32 reg;
	ndev = (struct net_device *)data;
	lp = (struct net_local *)netdev_priv (ndev);

	/* first, find out what's going on with the PHY */
	if (get_phy_status (ndev, &phy_carrier)) {
		dev_err (&ndev->dev, "terminating link monitoring\n");
		return;
	}

	netif_carrier = netif_carrier_ok (ndev) != 0;

	if (phy_carrier != netif_carrier) {
		if (phy_carrier) {

			set_mac_speed (lp);

			/* enable receive and transmit clients */
			reg = in_be32 ((u32 *) GCSR);
			reg |= GCSR_CONNECTED;
			out_be32 ((u32 *) GCSR, reg);

			dev_info (&ndev->dev, "PHY link carrier restored\n");
			netif_carrier_on (ndev);

		} else {

			/* disable receive and transmit clients */
			reg = in_be32 ((u32 *) GCSR);
			reg &= ~GCSR_CONNECTED;
			out_be32 ((u32 *) GCSR, reg);

			dev_info (&ndev->dev, "PHY link carrier lost\n");
			netif_carrier_off (ndev);
		}
	}

	/* Set up the timer so we'll get called again in 2 seconds. */
	lp->phy_timer.expires = jiffies + 2 * HZ;
	add_timer (&lp->phy_timer);
}

/**
 * s2imac_send_data - Send an Ethernet frame
 * @lp:		Pointer to the S2IMAC device private data
 * @data:	Pointer to the data to be sent
 * @byte_count:	Total frame size, including header
 *
 * This function checks if the Tx buffer of the s2imac device is free to send
 * data. If so, it fills the Tx buffer with data for transmission. Otherwise, it
 * returns an error.
 *
 * Return:	0 upon success or -1 if the buffer(s) are full.
 *
 * Note:	The maximum Tx packet size can not be more than Ethernet header
 *		(14 Bytes) + Maximum MTU (1500 bytes). This is excluding FCS.
 */
static int s2imac_send_data (struct net_local *lp, u8 * data,
			     unsigned int byte_count)
{
	u16 *buf, val, val1;
	u32 len, i;

#ifndef CONFIG_S2IMAC_POLLING
	if(in_be32 ((u32 *) TX_LEN))
		return -1; /* Buffer was full, return failure */
#endif

	/* If the length is too large, truncate it */
	if (byte_count > ETH_FRAME_LEN)
		byte_count = ETH_FRAME_LEN;

	len = (byte_count / 4) + 1;

	buf = (u16 *) data;

	/* first 2 bytes of buffer not used */
	val = *buf++;
	gige_txbuf[0] = (u32) (((u32) 0x0000 << 16) | (u32) val);

	for (i = 1; i < len; i++) {
		val = *buf++;
		val1 = *buf++;
		gige_txbuf[i] = (u32) (((u32) val << 16) | ((u32) val1));
	}

	/* send reply */
	while (in_be32 ((u32 *) TX_LEN)) {
	};
	out_be32 ((u32 *) TX_LEN, byte_count);

	return 0;
}

#ifndef CONFIG_S2IMAC_POLLING
/**********************/
/* Interrupt Handlers */
/**********************/

/**
 * s2imac_tx_handler - Interrupt handler for frames sent
 * @dev:	Pointer to the network device
 *
 * This function updates the number of packets transmitted and handles the
 * deferred skb, if there is one.
 */
static void s2imac_tx_handler(struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) netdev_priv(dev);

	if (lp->deferred_skb) {
		dev->stats.tx_packets++;
		if (s2imac_send_data(lp,
				(u8 *) lp->deferred_skb->data,
				lp->deferred_skb->len) != 0) {
			dev->stats.tx_bytes += lp->deferred_skb->len;
			dev_kfree_skb_irq(lp->deferred_skb);
			lp->deferred_skb = NULL;
			dev->trans_start = jiffies;
			netif_wake_queue(dev);
		}
	}
	/* clear tx interrupt */
	out_be32 ((u32 *) INT_REQ,  INT_ENABLE_TX);
}

/**
 * s2imac_rx_handler- Interrupt handler for frames received
 * @dev:	Pointer to the network device
 *
 * This function allocates memory for a socket buffer, fills it with data
 * received and hands it over to the TCP/IP stack.
 */
static void s2imac_rx_handler(struct net_device *dev)
{
	struct net_local *lp = (struct net_local *) netdev_priv(dev);
	struct sk_buff *skb;
	unsigned int align;
	u32 len, l, i, align_buffer;
	u16 *to_u16_ptr, *from_u16_ptr;

	len = in_be32 ((u32 *) RX_LEN);

	if (len) {
		skb = dev_alloc_skb (len + ALIGNMENT);
		if (!skb) {
			dev->stats.rx_dropped++;
			dev_err (&dev->dev,
				 "Could not allocate receive buffer\n");
			return;
		}

		/*
		 * A new skb should have the data halfword aligned, but this code is
		 * here just in case that isn't true. Calculate how many
		 * bytes we should reserve to get the data to start on a word
		 * boundary
		 */
		align = BUFFER_ALIGN (skb->data);
		if (align)
			skb_reserve (skb, align);

		skb_reserve (skb, 2);

		l = (len / 4) + 1;

		to_u16_ptr = (u16 *) skb->data;

		/* first 2 bytes of buffer not used */
		for (i = 0; i < l; i++) {
			align_buffer =
			    ((gige_rxbuf[i] << 16) | (gige_rxbuf[i + 1] >> 16));
			from_u16_ptr = (u16 *) & align_buffer;

			/* Read data from source */
			*to_u16_ptr++ = *from_u16_ptr++;
			*to_u16_ptr++ = *from_u16_ptr++;

		}

		/* clear rx interrupt */
		out_be32 ((u32 *) INT_REQ,  INT_ENABLE_RX);
		out_be32 ((u32 *) RX_LEN, 0);

		skb_put (skb, len);	/* Tell the skb how much data we got */
		skb->dev = dev;	/* Fill out required meta-data */

		skb->protocol = eth_type_trans (skb, dev);
		skb->ip_summed = CHECKSUM_NONE;

		dev->stats.rx_packets++;
		dev->stats.rx_bytes += len;

		netif_rx (skb);	/* Send the packet upstream */

	} else {
		dev->stats.rx_errors++;
	}
}

/**
 * s2imac_interrupt - Interrupt handler for this driver
 * @irq:	Irq of the s2imac device
 * @dev_id:	Void pointer to the network device instance used as callback
 *		reference
 *
 * This function handles the Tx and Rx interrupts of the s2imac device.
 */
static irqreturn_t s2imac_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = dev_id;
	struct net_local *lp = (struct net_local *) netdev_priv(dev);
	u32 reg_data;
	unsigned long flags;

	spin_lock_irqsave (&lp->reset_lock, flags);

	/* read the irq status */
	reg_data = in_be32((u32 *) INT_REQ);

	/* check if rx interrupt */
	if((reg_data & INT_ENABLE_RX) == INT_ENABLE_RX)
		s2imac_rx_handler(dev);

	/* check if tx interrupt */
	if((reg_data & INT_ENABLE_TX) == INT_ENABLE_TX)
		s2imac_tx_handler(dev);

	spin_unlock_irqrestore (&lp->reset_lock, flags);

	return IRQ_HANDLED;
}

#else

static void s2imac_irq_timer (unsigned long data)
{
	struct net_device *ndev = (struct net_device *)data;
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);
	struct sk_buff *skb;
	unsigned int align;
	u32 len, l, i, align_buffer;
	u16 *to_u16_ptr, *from_u16_ptr;

	len = in_be32 ((u32 *) RX_LEN);

	if (len) {
		skb = dev_alloc_skb (len + ALIGNMENT);
		if (!skb) {
			ndev->stats.rx_dropped++;
			dev_err (&ndev->dev,
				"Could not allocate receive buffer\n");
			return;
		}

		/*
		 * A new skb should have the data halfword aligned, but this code is
		 * here just in case that isn't true. Calculate how many
		 * bytes we should reserve to get the data to start on a word
		 * boundary
		 */
		align = BUFFER_ALIGN (skb->data);
		if (align)
			skb_reserve (skb, align);

		skb_reserve (skb, 2);

		l = (len / 4) + 1;

		to_u16_ptr = (u16 *) skb->data;

		/* first 2 bytes of buffer not used */
		for (i = 0; i < l; i++) {
			align_buffer =
				((gige_rxbuf[i] << 16) | (gige_rxbuf[i + 1] >> 16));
			from_u16_ptr = (u16 *) & align_buffer;

			/* Read data from source */
			*to_u16_ptr++ = *from_u16_ptr++;
			*to_u16_ptr++ = *from_u16_ptr++;

		}
		out_be32 ((u32 *) RX_LEN, 0);

		skb_put (skb, len);	/* Tell the skb how much data we got */
		skb->dev = ndev;	/* Fill out required meta-data */

		skb->protocol = eth_type_trans (skb, ndev);
		skb->ip_summed = CHECKSUM_NONE;

		ndev->stats.rx_packets++;
		ndev->stats.rx_bytes += len;

		netif_rx (skb);		/* Send the packet upstream */

	} else {
		ndev->stats.rx_errors++;
	}

	mod_timer (&lp->irq_timer, jiffies);
}
#endif

/**
 * s2imac_update_address - Update the MAC address in the device
 * @lp:		Pointer to the s2imac device private data
 * @address_ptr:Pointer to the MAC address (MAC address is a 48-bit value)
 *
 * Tx must be idle and Rx should be idle for deterministic results.
 * It is recommended that this function should be called after the
 * initialization and before transmission of any packets from the device.
 * The MAC address can be programmed using any of the two transmit
 * buffers (if configured).
 */
static void s2imac_update_address (struct net_local *lp, u8 * address_ptr)
{
	int val;
	u32 mac_l, mac_h;

	/* set up unicast MAC address filter */
	val = ((address_ptr[3] << 24) | (address_ptr[2] << 16) |
	       (address_ptr[1] << 8) | (address_ptr[0]));
	out_be32 ((u32 *) UAW0, val);
	val = (address_ptr[5] << 8) | address_ptr[4];
	out_be32 ((u32 *) UAW1, val);

	mac_h = (address_ptr[0] << 8) | address_ptr[1];
	mac_l = ((address_ptr[2] << 24) | (address_ptr[3] << 16) |
		 (address_ptr[4] << 8) | (address_ptr[5]));
	out_be32 ((u32 *) MAC_HIGH, mac_h);	/* set gige_mac_h register */
	out_be32 ((u32 *) MAC_LOW, mac_l);	/* set gige_mac_l register */
	out_be32 ((u32 *) MAC_RX_HIGH, mac_h);	/* set gige_rx_mac_h register */
	out_be32 ((u32 *) MAC_RX_LOW, mac_l);	/* set gige_rx_mac_l register */
}

/**
 * s2imac_set_mac_address - Set the MAC address for this device
 * @dev:	Pointer to the network device instance
 * @addr:	Void pointer to the sockaddr structure
 *
 * This function copies the HW address from the sockaddr strucutre to the
 * net_device structure and updates the address in HW.
 *
 * Return:	Error if the net device is busy or 0 if the addr is set
 *		successfully
 */
static int s2imac_set_mac_address (struct net_device *ndev, void *address)
{
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);
	struct sockaddr *addr = address;

	if (netif_running (ndev))
		return -EBUSY;

	memcpy (ndev->dev_addr, addr->sa_data, ndev->addr_len);
	s2imac_update_address (lp, ndev->dev_addr);
	return 0;
}

/**
 * s2imac_tx_timeout - Callback for Tx Timeout
 * @dev:	Pointer to the network device
 *
 * This function is called when Tx time out occurs for S2IMAC device.
 */
static void s2imac_tx_timeout (struct net_device *ndev)
{
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);
	unsigned long flags;

	dev_err (&ndev->dev, "Exceeded transmit timeout of %lu ms\n",
		 TX_TIMEOUT * 1000UL / HZ);

	ndev->stats.tx_errors++;

	/* Reset the device */
	spin_lock_irqsave (&lp->reset_lock, flags);

	/* Shouldn't really be necessary, but shouldn't hurt */
	netif_stop_queue (ndev);

#ifndef CONFIG_S2IMAC_POLLING
	s2imac_disable_interrupts(lp);
	s2imac_enable_interrupts(lp);
#endif

	if (lp->deferred_skb) {
		dev_kfree_skb (lp->deferred_skb);
		lp->deferred_skb = NULL;
		ndev->stats.tx_errors++;
	}

	/* To exclude tx timeout */
	ndev->trans_start = 0xffffffff - TX_TIMEOUT - TX_TIMEOUT;

	/* We're all ready to go. Start the queue */
	netif_wake_queue (ndev);
	spin_unlock_irqrestore (&lp->reset_lock, flags);
}

/**
 * s2imac_open - Open the network device
 * @dev:	Pointer to the network device
 *
 * This function sets the MAC address, requests an IRQ and enables interrupts
 * for the s2imac device and starts the Tx queue.
 * It also connects to the phy device, if MDIO is included in s2imac device.
 */
static int s2imac_open (struct net_device *ndev)
{
	u32 reg;
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);

#ifndef CONFIG_S2IMAC_POLLING
	int retval;
	s2imac_disable_interrupts(lp);
#endif

	/* Just to be safe, stop the device first */
	netif_stop_queue (ndev);

	/* Set the MAC address each time opened */
	s2imac_update_address (lp, ndev->dev_addr);

	/* transmit and receive packet buffers */
	gige_txbuf = (volatile u32 *)TXBUF;
	gige_rxbuf = (volatile u32 *)RXBUF;

	/* setup phy */
	set_mac_speed (lp);

	/* enable receive and transmit clients */
	reg = in_be32 ((u32 *) GCSR);
	reg |= GCSR_CONNECTED;
	out_be32 ((u32 *) GCSR, reg);

#ifndef CONFIG_S2IMAC_POLLING
	/* grab the IRQ */
	retval = request_irq(ndev->irq, s2imac_interrupt, 0, ndev->name, ndev);
	if (retval) {
		dev_err(&lp->ndev->dev, "Could not allocate interrupt %d\n",
			ndev->irq);
		return retval;
	}

	/* Enable Interrupts */
	s2imac_enable_interrupts(lp);
#else
	mod_timer (&lp->irq_timer, jiffies);
#endif

	/* We're ready to go */
	netif_start_queue (ndev);

	/* Set up the PHY monitoring timer. */
	lp->phy_timer.expires = jiffies + 2 * HZ;
	lp->phy_timer.data = (unsigned long)ndev;
	lp->phy_timer.function = &poll_gmii;
	init_timer (&lp->phy_timer);
	add_timer (&lp->phy_timer);

	return 0;
}

/**
 * s2imac_close - Close the network device
 * @dev:	Pointer to the network device
 *
 * This function stops the Tx queue, disables interrupts and frees the IRQ for
 * the s2imac device.
 * It also disconnects the phy device associated with the S2IMAC device.
 */
static int s2imac_close (struct net_device *ndev)
{
	u32 reg;
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);

	netif_stop_queue (ndev);

#ifndef CONFIG_S2IMAC_POLLING
	s2imac_disable_interrupts(lp);
	free_irq(ndev->irq, ndev);
#endif

	/* Shut down the PHY monitoring timer. */
	del_timer_sync (&lp->phy_timer);

	/* disable receive and transmit clients */
	reg = in_be32 ((u32 *) GCSR);
	reg &= ~GCSR_CONNECTED;
	out_be32 ((u32 *) GCSR, reg);

	return 0;
}

/**
 * s2imac_get_stats - Get the stats for the net_device
 * @dev:	Pointer to the network device
 *
 * This function returns the address of the 'net_device_stats' structure for the
 * given network device. This structure holds usage statistics for the network
 * device.
 *
 * Return:	Pointer to the net_device_stats structure.
 */
static struct net_device_stats *s2imac_get_stats (struct net_device *ndev)
{
	return &ndev->stats;
}

/**
 * s2imac_send - Transmit a frame
 * @orig_skb:	Pointer to the socket buffer to be transmitted
 * @dev:	Pointer to the network device
 *
 * This function checks if the Tx buffer of the s2imac device is free to send
 * data. If so, it fills the Tx buffer with data from socket buffer data,
 * updates the stats and frees the socket buffer. The Tx completion is signaled
 * by an interrupt. If the Tx buffer isn't free, then the socket buffer is
 * deferred and the Tx queue is stopped so that the deferred socket buffer can
 * be transmitted when the s2imac device is free to transmit data.
 *
 * Return:	0, always.
 */
static int s2imac_send (struct sk_buff *orig_skb, struct net_device *ndev)
{
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);
	struct sk_buff *new_skb;
	unsigned int len;
	unsigned long flags;

	len = orig_skb->len;

	new_skb = orig_skb;

	spin_lock_irqsave (&lp->reset_lock, flags);
	if (s2imac_send_data (lp, (u8 *) new_skb->data, len) != 0) {
		/*
		 * If the s2imac Tx buffer is busy, stop the Tx queue and
		 * defer the skb for transmission at a later point when the
		 * current transmission is complete
		 */
		netif_stop_queue (ndev);
		lp->deferred_skb = new_skb;
		spin_unlock_irqrestore (&lp->reset_lock, flags);
		return 0;
	}
	spin_unlock_irqrestore (&lp->reset_lock, flags);

	ndev->stats.tx_bytes += len;
	dev_kfree_skb (new_skb);
	ndev->trans_start = jiffies;

	return 0;
}

/**
 * s2imac_remove_ndev - Free the network device
 * @ndev:	Pointer to the network device to be freed
 *
 * This function un maps the IO region of the S2IMAC device and frees the net
 * device.
 */
static void s2imac_remove_ndev (struct net_device *ndev)
{
	if (ndev) {
		struct net_local *lp = (struct net_local *)netdev_priv (ndev);

		if (lp->base_addr[S2IMAC_NETMAP])
			iounmap ((void __iomem __force *)(lp->base_addr
							  [S2IMAC_NETMAP]));
		free_netdev (ndev);
	}
}

static int s2imac_ethtool_get_settings (struct net_device *ndev,
					struct ethtool_cmd *ecmd)
{
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);
	u16 gmii_cmd, gmii_status, gmii_advControl;

	memset (ecmd, 0, sizeof (struct ethtool_cmd));

	gmii_cmd = s2imac_phy_read (lp, MII_BMCR);
	gmii_status = s2imac_phy_read (lp, MII_BMSR);

	gmii_advControl = s2imac_phy_read (lp, MII_ADVERTISE);

	ecmd->duplex = DUPLEX_FULL;

	ecmd->supported |= SUPPORTED_MII;

	ecmd->port = PORT_MII;

	ecmd->speed = lp->cur_speed;

	if (gmii_status & BMSR_ANEGCAPABLE) {
		ecmd->supported |= SUPPORTED_Autoneg;
	}
	if (gmii_status & BMSR_ANEGCOMPLETE) {
		ecmd->autoneg = AUTONEG_ENABLE;
		ecmd->advertising |= ADVERTISED_Autoneg;
	} else {
		ecmd->autoneg = AUTONEG_DISABLE;
	}
	ecmd->phy_address = lp->phy_addr;
	ecmd->transceiver = XCVR_INTERNAL;

	ecmd->supported |= SUPPORTED_10baseT_Full | SUPPORTED_100baseT_Full |
	    SUPPORTED_1000baseT_Full | SUPPORTED_Autoneg;

	return 0;
}

static int s2imac_ethtool_set_settings (struct net_device *ndev,
					struct ethtool_cmd *ecmd)
{
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);

	if ((ecmd->duplex != DUPLEX_FULL) ||
	    (ecmd->transceiver != XCVR_INTERNAL) ||
	    (ecmd->phy_address && (ecmd->phy_address != lp->phy_addr))) {
		return -EOPNOTSUPP;
	}

	if ((ecmd->speed != 1000) && (ecmd->speed != 100) &&
	    (ecmd->speed != 10)) {
		dev_err (&ndev->dev, "speed not supported: %d\n", ecmd->speed);
		return -EOPNOTSUPP;
	}

	if (ecmd->speed != lp->cur_speed) {
		lp->cur_speed = ecmd->speed;
		dev_err (&ndev->dev, "can not change speed to: %d\n", ecmd->speed);
	}
	return 0;
}

static struct net_device_ops s2imac_netdev_ops;

/* From include/linux/ethtool.h */
static struct ethtool_ops ethtool_ops = {
	.get_settings = s2imac_ethtool_get_settings,
	.set_settings = s2imac_ethtool_set_settings,
	.get_link = ethtool_op_get_link
};

/**
 * s2imac_of_probe - Probe method for the s2imac device.
 * @ofdev:	Pointer to OF device structure
 * @match:	Pointer to the structure used for matching a device
 *
 * This function probes for the s2imac device in the device tree.
 * It initializes the driver data structure and the hardware, sets the MAC
 * address and registers the network device.
 *
 * Return:	0, if the driver is bound to the S2IMAC device, or
 *		a negative error if there is failure.
 */
static int __devinit s2imac_of_probe (struct of_device *ofdev,
				      const struct of_device_id *match)
{
#ifndef CONFIG_S2IMAC_POLLING
	struct resource r_irq; /* Interrupt resources */
#endif
	struct resource r_mem;	/* IO mem resources */
	struct net_device *ndev;
	struct net_local *lp;
	struct device_node *np = ofdev->node;
	struct device *dev = &ofdev->dev;
	const void *mac_address;
	const unsigned int *clk;
	int rc = 0;
	u32 mac_l, mac_h;

#ifdef CONFIG_MICROBLAZE
	struct device_node *cpu;
#endif

	dev_info (dev, "probing \'%s\'\n", np->name);

	/* Create an ethernet device instance */
	ndev = alloc_etherdev (sizeof (struct net_local));
	if (!ndev) {
		dev_err (dev, "Could not allocate network device\n");
		return -ENOMEM;
	}

	dev_set_drvdata (dev, ndev);
	SET_NETDEV_DEV (ndev, dev);

	lp = netdev_priv (ndev);
	lp->ndev = ndev;

	/* Get clock configuration for the device */
	clk = of_get_property (np, "clock-frequency", NULL);
	if (!clk) {
		dev_warn (dev, "no clock-frequency property set\n");
		rc = -ENODEV;

#ifndef CONFIG_MICROBLAZE

		goto error3;

#else /* is CONFIG_MICROBLAZE */

		cpu = of_find_node_by_type (NULL, "cpu");
		if (!cpu) {
			dev_err (dev, "you don't have a cpu?\n");
			goto error3;
		}

		clk = of_get_property (cpu, "clock-frequency", NULL);
		if (!clk) {
			dev_err (dev, "no CPU clock-frequency property set\n");
			goto error3;
		}

		dev_warn (dev, "use CPU clock-frequency\n");
		rc = 0;
#endif
	}

	lp->coreclk = *clk;
	dev_info (dev, "core clock is %d Hz\n", lp->coreclk);

	/* Get iospace for the device */
	rc = of_address_to_resource (np, S2IMAC_NETMAP, &r_mem);
	if (rc) {
		dev_err (dev, "invalid address\n");
		goto error3;
	}

	ndev->mem_start = r_mem.start;
	ndev->mem_end = r_mem.end;

	if (!request_mem_region (ndev->mem_start,
				 ndev->mem_end - ndev->mem_start + 1,
				 DRIVER_NAME)) {
		dev_err (dev, "Couldn't lock memory region at %p\n",
			 (void *)ndev->mem_start);
		rc = -EBUSY;
		goto error3;
	}

	/* Get iospace for the register */
	rc = of_address_to_resource (np, S2IMAC_REGMAP, &r_mem);
	if (rc) {
		dev_err (dev, "invalid address\n");
		goto error2;
	}

	lp->regmem_start = r_mem.start;
	lp->regmem_end = r_mem.end;

	if (!request_mem_region (lp->regmem_start,
				 lp->regmem_end - lp->regmem_start + 1,
				 DRIVER_NAME)) {
		dev_err (dev, "Couldn't lock memory region at %p\n",
			 (void *)ndev->mem_start);
		rc = -EBUSY;
		goto error2;
	}

	/* Get the virtual base address for the device and register */
	lp->base_addr[S2IMAC_NETMAP] =
	    ioremap (ndev->mem_start, ndev->mem_end - ndev->mem_start + 1);
	lp->base_addr[S2IMAC_REGMAP] =
	    ioremap (lp->regmem_start, lp->regmem_end - lp->regmem_start + 1);
	if ((NULL == lp->base_addr[S2IMAC_NETMAP])
	    || (NULL == lp->base_addr[S2IMAC_REGMAP])) {
		dev_err (dev, "Could not allocate iomem\n");
		rc = -EIO;
		goto error1;
	}

	dev_info (dev, "core found at 0x%p mapped to 0x%p\n",
		  (void *)ndev->mem_start,
		  (void *)lp->base_addr[S2IMAC_NETMAP]);

	dev_info (dev, "register found at 0x%p mapped to 0x%p\n",
		  (void *)lp->regmem_start,
		  (void *)lp->base_addr[S2IMAC_REGMAP]);

#ifndef CONFIG_S2IMAC_POLLING
	/* Get IRQ for the device */
	rc = of_irq_to_resource(np, 0, &r_irq);
	if (rc == NO_IRQ) {
		dev_err(dev, "no IRQ found\n");
		goto error0;
	}
	ndev->irq = r_irq.start;
	dev_info(dev,"interrupt is %d\n", ndev->irq);
#endif

	spin_lock_init (&lp->reset_lock);

	/* check if mac address is already set */
	mac_h = in_be32 ((u32 *) MAC_HIGH);
	mac_l = in_be32 ((u32 *) MAC_LOW);

	if ((mac_h == 0) && (mac_l == 0)) {
		mac_address = of_get_mac_address (np);
		if (mac_address)
			/* Copy the MAC address from OF node. */
			memcpy (ndev->dev_addr, mac_address, 6);
		else
			dev_warn (dev, "No MAC address found\n");

		/* Set the MAC address in the device */
		s2imac_update_address (lp, ndev->dev_addr);
	} else {
		ndev->dev_addr[0] = (u8) (mac_h >> 8);
		ndev->dev_addr[1] = (u8) (mac_h & 0xff);
		ndev->dev_addr[2] = (u8) (mac_l >> 24);
		ndev->dev_addr[3] = (u8) (mac_l >> 16);
		ndev->dev_addr[4] = (u8) (mac_l >> 8);
		ndev->dev_addr[5] = (u8) (mac_l & 0xff);
	}

	dev_info (dev, "MAC address is now %02x:%02x:%02x:%02x:%02x:%02x\n",
		  ndev->dev_addr[0], ndev->dev_addr[1], ndev->dev_addr[2],
		  ndev->dev_addr[3], ndev->dev_addr[4], ndev->dev_addr[5]);

	/* Scan to find the PHY */
	phy_setup (lp);
	lp->phy_addr = detect_phy (lp, dev);

#ifdef CONFIG_S2IMAC_POLLING
	init_timer (&lp->irq_timer);
	lp->irq_timer.function = s2imac_irq_timer;
	lp->irq_timer.data = (unsigned long)ndev;
#endif

	ndev->netdev_ops = &s2imac_netdev_ops;
	ndev->flags &= ~IFF_MULTICAST;
	ndev->watchdog_timeo = TX_TIMEOUT;

	/* Set ethtool IOCTL handler vectors. */
	SET_ETHTOOL_OPS (ndev, &ethtool_ops);

	/* Finally, register the device */
	rc = register_netdev (ndev);
	if (rc) {
		dev_err (dev, "Cannot register network device, aborting\n");
		goto error0;
	}

	return 0;

error0:
	iounmap (lp->base_addr[S2IMAC_REGMAP]);
	iounmap (lp->base_addr[S2IMAC_NETMAP]);

error1:
	release_mem_region (lp->regmem_start,
			    lp->regmem_end - lp->regmem_start + 1);

error2:
	release_mem_region (ndev->mem_start,
			    ndev->mem_end - ndev->mem_start + 1);

error3:
	s2imac_remove_ndev (ndev);
	return rc;
}

/**
 * s2imac_of_remove - Unbind the driver from the s2imac device.
 * @of_dev:	Pointer to OF device structure
 *
 * This function is called if a device is physically removed from the system or
 * if the driver module is being unloaded. It frees any resources allocated to
 * the device.
 *
 * Return:	0, always.
 */
static int __devexit s2imac_of_remove (struct of_device *of_dev)
{
	struct device *dev = &of_dev->dev;
	struct net_device *ndev = dev_get_drvdata (dev);
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);

	unregister_netdev (ndev);

	iounmap (lp->base_addr[S2IMAC_REGMAP]);
	iounmap (lp->base_addr[S2IMAC_NETMAP]);

	release_mem_region (lp->regmem_start,
			    lp->regmem_end - lp->regmem_start + 1);
	release_mem_region (ndev->mem_start,
			    ndev->mem_end - ndev->mem_start + 1);

	s2imac_remove_ndev (ndev);
	dev_set_drvdata (dev, NULL);

	return 0;
}

static int s2imac_change_mtu (struct net_device *ndev, int new_mtu)
{
	int device_enable = 0;
#ifdef CONFIG_XILINX_GIGE_VLAN
	int head_size = XTE_HDR_VLAN_SIZE;
#else
	int head_size = XTE_HDR_SIZE;
#endif
	int max_frame = new_mtu + head_size + XTE_TRL_SIZE;
	int min_frame = 1 + head_size + XTE_TRL_SIZE;

	if (max_frame < min_frame)
		return -EINVAL;

	if (max_frame > XTE_MAX_JUMBO_FRAME_SIZE) {
		dev_info (&ndev->dev, "Wrong MTU packet size. Use %d size\n",
			XTE_JUMBO_MTU);
		new_mtu = XTE_JUMBO_MTU;
	}

	ndev->mtu = new_mtu;	/* change mtu in net_device structure */

	/* stop driver */
	if (netif_running (ndev)) {
		device_enable = 1;
		s2imac_close (ndev);
	}

	if (device_enable)
		s2imac_open (ndev);	/* open the device */
	return 0;
}

static int s2imac_ioctl (struct net_device *ndev, struct ifreq *rq, int cmd)
{
	struct net_local *lp = (struct net_local *)netdev_priv (ndev);
	s2igen_content_t gen_content;

	if (copy_from_user (&gen_content, rq->ifr_data, sizeof (gen_content)))
		return -EFAULT;

	/* check address range 1st slot --> device base address */
	if((cmd == SIOS2IGENRD) || (cmd == SIOS2IGENWR)){

		if (gen_content.address > (ndev->mem_end - ndev->mem_start + 1))
			return -EINVAL;
	}

	/* check address range 2st slot --> register base address */
	if((cmd == SIOS2IREGRD) || (cmd == SIOS2IREGWR)){

		if (gen_content.address > (lp->regmem_end - lp->regmem_start + 1))
			return -EINVAL;
	}

	switch (cmd) {
	case SIOS2IGENRD:	/* core read */

		gen_content.data =
		    in_be32 ((u32 *) (lp->base_addr[S2IMAC_NETMAP] + gen_content.address));

		if (copy_to_user
		    (rq->ifr_data, &gen_content, sizeof (gen_content)))
			return -EFAULT;

		return 0;

	case SIOS2IGENWR:	/* core write */

		out_be32 ((u32 *) (lp->base_addr[S2IMAC_NETMAP] + gen_content.address),
			  gen_content.data);

		return 0;

	case SIOS2IREGRD:	/* register read */

		gen_content.data =
		    in_be32 ((u32 *) (lp->base_addr[S2IMAC_REGMAP] + gen_content.address));

		if (copy_to_user
		    (rq->ifr_data, &gen_content, sizeof (gen_content)))
			return -EFAULT;

		return 0;

	case SIOS2IREGWR:	/* register write */

		out_be32 ((u32 *) (lp->base_addr[S2IMAC_REGMAP] + gen_content.address),
			  gen_content.data);

		return 0;

	default:
		return -EOPNOTSUPP;
	}
}

static struct net_device_ops s2imac_netdev_ops = {
	.ndo_open = s2imac_open,
	.ndo_stop = s2imac_close,
	.ndo_start_xmit = s2imac_send,
	.ndo_do_ioctl = s2imac_ioctl,
	.ndo_change_mtu = s2imac_change_mtu,
	.ndo_set_mac_address = s2imac_set_mac_address,
	.ndo_tx_timeout = s2imac_tx_timeout,
	.ndo_get_stats = s2imac_get_stats,
};

/* Match table for OF platform binding */
static struct of_device_id s2imac_of_match[] __devinitdata = {
	{ .compatible = "s2i,s2imac-1.00.a", },
	{ .compatible = "xlnx,s2imac-epc-1.02.a", },
	{ .compatible = "s2i,s2imac-epc", .type = "network", },
	{ /* end of list */ },
};

MODULE_DEVICE_TABLE (of, s2imac_of_match);

static struct of_platform_driver s2imac_of_driver = {
	.name = DRIVER_NAME,
	.match_table = s2imac_of_match,
	.probe = s2imac_of_probe,
	.remove = __devexit_p (s2imac_of_remove),
};

/**
 * s2imac_init - Initial driver registration call
 *
 * Return:	0 upon success, or a negative error upon failure.
 */
static int __init s2imac_init (void)
{
	/* No kernel boot options used, we just need to register the driver */
	return of_register_platform_driver (&s2imac_of_driver);
}

/**
 * s2imac_cleanup - Driver un-registration call
 */
static void __exit s2imac_cleanup (void)
{
	of_unregister_platform_driver (&s2imac_of_driver);
}

module_init (s2imac_init);
module_exit (s2imac_cleanup);

MODULE_AUTHOR ("Sensor to Image GmbH.");
MODULE_DESCRIPTION ("S2I Ethernet MAC driver");
MODULE_VERSION (DRIVER_VERS);
MODULE_LICENSE ("GPL");
