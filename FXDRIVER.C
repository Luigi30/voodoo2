#include "FXDRIVER.H"
#include "V2PCI.H"

extern DPT driver$dpt;
extern DDT driver$ddt;
extern FDT driver$fdt;

//#define DEBUG

// PCI bus ADP is FFFFFFFF.81D1F7C0 

static $DESCRIPTOR (voodoo2_section_name, "voodoo2_global");

int driver$init_tables()
{
    // Populate the DPT.
    ini_dpt_name        (&driver$dpt, "SSTDRIVER");
    ini_dpt_ucbsize     (&driver$dpt, sizeof(SST_UCB));
    ini_dpt_defunits    (&driver$dpt, 1 );
    ini_dpt_adapt       (&driver$dpt, AT$_PCI);
    ini_dpt_struc_init  (&driver$dpt, SST$struc_init);      // called to initialize the driver's structures on load
    ini_dpt_struc_reinit(&driver$dpt, SST$struc_reinit);    // called to reinitialize the driver's structures
    ini_dpt_end         (&driver$dpt);

    // Populate the DDT. These functions get called by the kernel.
    ini_ddt_unitinit    (&driver$ddt, SST$unit_init);
    ini_ddt_start       (&driver$ddt, SST$startio);
    ini_ddt_cancel      (&driver$ddt, ioc_std$cancelio);
    ini_ddt_end         (&driver$ddt);

    ini_fdt_act (&driver$fdt, IO$_READPBLK,     SST$read,           BUFFERED_64);
    ini_fdt_act (&driver$fdt, IO$_WRITEPBLK,    SST$write,          BUFFERED_64);
    ini_fdt_act (&driver$fdt, IO$_ACCESS,       SST$getLinearPtr,   BUFFERED_64);
    ini_fdt_end (&driver$fdt);

    /*
    ini_fdt_act (&driver$fdt, IO$_WRITELBLK, lr$write, BUFFERED_64);
    ini_fdt_act (&driver$fdt, IO$_WRITEPBLK, lr$write, BUFFERED_64);
    ini_fdt_act (&driver$fdt, IO$_WRITEVBLK, lr$write, BUFFERED_64);
    ini_fdt_act (&driver$fdt, IO$_SETMODE, lr$setmode, BUFFERED_64);
    ini_fdt_act (&driver$fdt, IO$_SETCHAR, lr$setmode, BUFFERED_64);
    ini_fdt_act (&driver$fdt, IO$_SENSEMODE, exe_std$sensemode, BUFFERED_64);
    ini_fdt_act (&driver$fdt, IO$_SENSECHAR, exe_std$sensemode, BUFFERED_64);
    ini_fdt_end (&driver$fdt);
    */

    return SS$_NORMAL;
}

/* Skeleton driver functions. */

// unit_init - Unit initialization entry point
// Called to initialize an individual device unit.
// This should fork() and do its work in another process.
int SST$unit_init(  IDB *idb,			    /* Interrupt Data Block pointer			*/
		            SST_UCB * ucb )		    /* Unit Control Block pointer			*/
{
    ADP     *adp = ucb->ucb$r_ucb.ucb$ps_adp;
    CRB     *crb = ucb->ucb$r_ucb.ucb$l_crb;

    int vendorid, deviceid;
    int status;

    // We can't use C RTL functions from inside the kernel.

    // confirm this is the right device
    status = ioc$read_pci_config(adp,
                        crb->crb$l_node,
                        0,
                        2,
                        (int *)&vendorid);

    status = ioc$read_pci_config(adp,
                        crb->crb$l_node,
                        2,
                        2,
                        (int *)&deviceid);

#ifdef DEBUG
    SST$print_message(ucb, "unit_init: vendor id is ", STS$K_INFO, FALSE);
    exe_std$outhex(vendorid);
    SST$print_message(ucb, "unit_init: device id is ", STS$K_INFO, FALSE);
    exe_std$outhex(deviceid >> 16);
    exe_std$outcrlf();
#endif

    int physical;
    // get the base physical address
    status = ioc$read_pci_config(adp,
                        crb->crb$l_node,
                        V2PCI$MEMBASEADDR,
                        4,
                        &physical);

    ucb->physical_base = physical;

#ifdef DEBUG
    SST$print_message(ucb, "unit_init: physical base addr is $", STS$K_INFO, FALSE);
    exe_std$outhex(ucb->physical_base);
    exe_std$outcrlf();
#endif

    uint64 hIO;

    status = ioc$map_io(adp,
                        crb->crb$l_node,
                        &ucb->physical_base,
                        1048576*16,
                        IOC$K_BUS_MEM_BYTE_GRAN,
                        &hIO);

    if(status != SS$_NORMAL)
    {
        SST$print_message(ucb, "unit_init: IOC$MAP_IO failure: ", STS$K_INFO, FALSE);
        exe_std$outhex(status);
        exe_std$outcrlf();
        return SS$_ABORT;
    }

    ucb->hIO = hIO;
#ifdef DEBUG
    SST$print_message(ucb, "unit_init: pci mapped into kernel, iohandle ", STS$K_INFO, TRUE);
    exe_std$outhex(ucb->hIO);
    exe_std$outcrlf();
#endif

    uint64 v2_physical_address = ((IOHANDLE *)ucb->hIO)->iohandle$q_platform_pa;
    uint64 voodoo2_section_ident = 0;

#ifdef DEBUG
    SST$print_message(ucb, "unit_init: V2 phys addr is ", STS$K_INFO, TRUE);
    exe_std$outhex(v2_physical_address);
    exe_std$outcrlf();

    SST$print_message(ucb, "unit_init: crmpsc_pfn_64: ", STS$K_INFO, FALSE);
    exe_std$outhex(status);
    exe_std$outcrlf();
#endif

    // OK, if we're mapped we should be able to read the status register.
    int status_register = 0x12345678;
    int read_offset = (1 << 10) | 0; // Chuck register 0x0000

    status = ioc$read_io(adp,
                         &ucb->hIO,
                         read_offset,
                         4,
                         &status_register);

#ifdef DEBUG
    SST$print_message(ucb, "unit_init: status register is $", STS$K_INFO, FALSE);
    exe_std$outhex(status_register);
    exe_std$outcrlf();
#endif

    ucb->ucb$r_ucb.ucb$v_online = 1;

    return( SS$_NORMAL );
}

void SST$startio (IRP *irp, SST_UCB *ucb) {
    CRB     *crb = ucb->ucb$r_ucb.ucb$l_crb;

    uint32 *output_buffer = (uint32 *)irp->irp$l_qio_p1;
    int data_offset = irp->irp$l_qio_p2;
    int status;
    int orig_ipl;

    device_lock(ucb->ucb$r_ucb.ucb$l_dlck, RAISE_IPL, &orig_ipl);

    int read_value = 0xFFFFFFFF;
    int write_value;

    // The card only supports 32-bit access, so we should always be retrieving one longword at a time.
    if(irp->irp$l_qio_p3 == IO$_ACCESS)
    {
        #ifdef DEBUG
        SST$print_message(ucb, "startio: IO$_ACCESS", STS$K_INFO, TRUE);
        #endif
        IOHANDLE *handle = (IOHANDLE *)ucb->hIO;
        uint64 *buffer = (uint64 *)irp->irp$l_qio_p1;
        *buffer = handle->iohandle$q_bus_pa;
    }
    else
    {
        if(irp->irp$l_qio_p4 == TRUE)
        {
            // TRUE if config space, FALSE if memory space
            switch(irp->irp$l_qio_p3)
            {
                case IO$_READPBLK:
                    #ifdef DEBUG
                    SST$print_message(ucb, "startio: read PCI config. output buffer ", STS$K_INFO, FALSE);
                    exe_std$outhex((int)output_buffer);
                    exe_std$outcrlf();
                    SST$print_message(ucb, "startio: data offset ", STS$K_INFO, FALSE);
                    exe_std$outhex(data_offset);
                    exe_std$outcrlf();
                    #endif

                    status = ioc$read_pci_config(ucb->ucb$r_ucb.ucb$ps_adp,
                        crb->crb$l_node,
                        data_offset,
                        4,
                        &read_value);
                    break;
                case IO$_WRITEPBLK:
                        #ifdef DEBUG
                        SST$print_message(ucb, "startio: write PCI config. address ", STS$K_INFO, FALSE);
                        exe_std$outhex(data_offset);
                        exe_std$outcrlf();
                        SST$print_message(ucb, "startio: data ", STS$K_INFO, FALSE);
                        exe_std$outhex(*output_buffer);
                        exe_std$outcrlf();
                        #endif

                    write_value = *output_buffer;
                    status = ioc$write_pci_config(ucb->ucb$r_ucb.ucb$ps_adp,
                        crb->crb$l_node,
                        data_offset,
                        4,
                        write_value);
                    break;  
            }
        }
        else if(irp->irp$l_qio_p4 == FALSE)
        {
            switch(irp->irp$l_qio_p3)
            {
                case IO$_READPBLK:
                    status = ioc$read_io(ucb->ucb$r_ucb.ucb$ps_adp,
                        &ucb->hIO,
                        data_offset,
                        4,
                        &read_value);
                    break;
                case IO$_WRITEPBLK:
                    status = ioc$write_io(ucb->ucb$r_ucb.ucb$ps_adp,
                        &ucb->hIO,
                        data_offset,
                        4,
                        &read_value);
                    break;  
            }
        }  

        *output_buffer = read_value;
    }

    #ifdef DEBUG
    SST$print_message(ucb, "startio: read value ", STS$K_INFO, FALSE);
    exe_std$outhex(read_value);
    exe_std$outcrlf();
    SST$print_message(ucb, "startio: status ", STS$K_INFO, FALSE);
    exe_std$outhex(status);
    exe_std$outcrlf();
    #endif

    ioc_std$reqcom(SS$_NORMAL, 0, &(ucb->ucb$r_ucb));
    return;   
}


// Called upon initial load. Set up any structures the driver needs.
void SST$struc_init (CRB *crb, DDB *ddb, IDB *idb, ORB *orb, SST_UCB *ucb) {
    ucb->ucb$r_ucb.ucb$b_devclass = DC$_VIDEO;
    #ifdef DEBUG
    SST$print_message(ucb, "struc_init called. makima is listening", STS$K_INFO, TRUE);
    #endif
    return;
}

void SST$struc_reinit ( CRB *crb,		/* Channel request block			*/
                        DDB *ddb,		/* Device data block				*/
		                IDB *idb,		/* Interrupt dispatch block			*/
		                ORB *orb,		/* Object rights block				*/
		                UCB *ucb )		/* Unit control block				*/
{
    return;
}

void SST$print_message(SST_UCB *ucb, char *message, int severity, int crlf)
{
    DDB		*ddb;				/* Device Data Block pointer			*/

    ddb = ucb->ucb$r_ucb.ucb$l_ddb;

    /* Ignore informational messages unless the SYSGEN UserD1
     * parameter is set.
     */
    /*
    if((( sgn$gl_userd1 & 1 ) == 0 ) &&
       ( severity == STS$K_INFO )) {
	return;
    }
    */
    
    /* Print out a message prefix using the severity code
     * and device name from the DDB.
     */
    exe_std$outzstring( "\a\r\n%SSTDRIVER-" );
    switch( severity ) {
    case STS$K_INFO:
    default:
	exe_std$outzstring( "I- " );
	break;
    case STS$K_WARNING:
	exe_std$outzstring( "W- " );
	break;
    case STS$K_ERROR:
	exe_std$outzstring( "E- " );
	break;
    case STS$K_SEVERE:
	exe_std$outzstring( "F- " );
	break;
    }
    exe_std$outcstring( ddb->ddb$t_name );
    exe_std$outzstring( "0, " );

    /* Print the user-provided portion of the message followed
     * by CR-LF.
     */
    exe_std$outzstring( message );
    if(crlf) exe_std$outcrlf();
    return;
}

/* QIO functions */
int SST$getLinearPtr (IRP *irp, PCB *pcb, SST_UCB *ucb, CCB *ccb) {
    // Skeleton.

    // todo: preprocessing
    irp->irp$l_qio_p3 = IO$_ACCESS;
    return ( call_qiodrvpkt (irp, (UCB *)ucb) );

    //return ( call_finishio (irp, (UCB *)ucb, SS$_NORMAL, 0) );
}

int SST$read (IRP *irp, PCB *pcb, SST_UCB *ucb, CCB *ccb) {
    // Skeleton.

    // todo: preprocessing
    irp->irp$l_qio_p3 = IO$_READPBLK;
    return ( call_qiodrvpkt (irp, (UCB *)ucb) );

    //return ( call_finishio (irp, (UCB *)ucb, SS$_NORMAL, 0) );
}

int SST$write (IRP *irp, PCB *pcb, SST_UCB *ucb, CCB *ccb) {
    // Skeleton.
    
    // todo: preprocessing
    irp->irp$l_qio_p3 = IO$_WRITEPBLK;

    return ( call_qiodrvpkt (irp, (UCB *)ucb) );
    //return ( call_finishio (irp, (UCB *)ucb, SS$_NORMAL, 0) );
}