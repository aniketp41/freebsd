/*
 * Copyright (c) 1997 Ted Faber
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Absolutely no warranty of function or purpose is made by the author
 *    Ted Faber.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * are met:
 */

#include "pci.h"
#if NPCI > 0

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <pci/pcireg.h>
#include <pci/pcivar.h>
#include <pci/pcic_p.h>

static u_long pcic_pci_count = 0;
static char *pcic_pci_probe  __P((pcici_t, pcidi_t));
static void pd6832_legacy_init __P((pcici_t tag, int unit));
static void pcic_pci_attach __P((pcici_t, int));

static struct pci_device pcic_pci_driver = {
	"pcic",
	pcic_pci_probe,
	pcic_pci_attach,
	&pcic_pci_count,
	NULL
};

DATA_SET (pcidevice_set, pcic_pci_driver);

/*
 * Return the ID string for a PD6832 if the vendor/product id matches,
 * NULL otherwise.
 */

static char*
pcic_pci_probe(pcici_t tag, pcidi_t type)
{
	switch(type) {
		case PCI_DEVICE_ID_PCIC_PD6832:
			return "Cirrus Logic CL-PD6832 CardBus Adapter";
			break;
		default:
			break;
	}
	return NULL;
}

/*
 * Set up the CL-PD6832 to look like a ISA based PCMCIA chip (a
 * PD672X).  This routine is called once per PCMCIA socket.
 */

static void
pd6832_legacy_init(pcici_t tag, int unit)
{
	u_long bcr; 		/* to set interrupts */
	u_short io_port;	/* the io_port to map this slot on */
  
   
	/*
	 * I think this should be a call to pci_map_port, but that
	 * routine won't map regiaters above 0x28, and the register we
	 * need to map is 0x44
	 */
    
	io_port = pci_conf_read(tag, PD6832_LEGACY_16BIT_IOADDR)
	    & ~PCI_MAP_IO;
    
	/*
	 * Configure the first I/O window to contain PD6832_NUM_REGS
	 * words and deactivate the second by setting the limit lower
	 * than the base
	 */
 		
	pci_conf_write(tag, PD6832_IO_BASE0, io_port | 1);
	pci_conf_write(tag, PD6832_IO_LIMIT0, (io_port + PD6832_NUM_REGS) | 1);
 		
	pci_conf_write(tag, PD6832_IO_BASE1, (io_port +0x20) | 1);
	pci_conf_write(tag, PD6832_IO_LIMIT1, io_port | 1 );
 
 	       
	/*
	 * Set default operating mode (I/O port space) and allocate
	 * this socket to the current unit
	 */
    
	pci_conf_write(tag, PCI_COMMAND_STATUS_REG, PD6832_COMMAND_DEFAULTS );
	pci_conf_write(tag, PD6832_SOCKET, unit);
 
	/*
	 * Set up the card inserted/card removed interrupts to come
	 * through the isa IRQ
	 */
 	       
	bcr = pci_conf_read(tag, PD6832_BRIDGE_CONTROL);
	bcr |= (PD6832_BCR_ISA_IRQ|PD6832_BCR_MGMT_IRQ_ENA);
	pci_conf_write(tag, PD6832_BRIDGE_CONTROL, bcr);

	if (bootverbose) { 		
		printf("CardBus: Legacy PC-card 16bit I/O address [0x%x]\n",
		   io_port);
#ifdef PCIC_DEBUG
		{
			int	i,j;
 		
			printf ("PCI Config space:\n");
			for (j = 0; j < 0x98; j += 16) {
				printf("%02x: ", j);
				for (i = 0; i < 16; i += 4) {
					printf(" %08x",
					   pci_conf_read(tag, i+j));
				}
				printf("\n");
			}
		}
#endif
	}
}


/*
 * This will be a general PCI based card dispatch routine.  Right now
 * it only understands the CL-PD6832
 */

static void
pcic_pci_attach(pcici_t config_id, int unit)
{
	u_long pcic_type;	/* The vendor id of the PCI pcic */

	pcic_type = pci_conf_read(config_id, PCI_ID_REG);

	switch (pcic_type) { 
		case PCI_DEVICE_ID_PCIC_PD6832:
			pd6832_legacy_init(config_id,unit);
			break;
	}
	return;
}

#endif /* NPCI > 0 */
