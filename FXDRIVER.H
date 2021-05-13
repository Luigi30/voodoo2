#include <stdio.h>

// I/O stuff
#include <alphadef.h>

/* Include files for system structures from SYS$LIB_C.TLB */

#include <adpdef.h>				/* Adaptor control block definitions		*/
#include <busarraydef.h>			/* Bus array header and entry definitions	*/
#include <ccbdef.h>				/* Channel control block definitions		*/
#include <crbdef.h>				/* Channel request block definitions		*/
#include <ctype.h>				/* Character type macro definitions		*/
#include <dcdef.h>				/* Device adaptor, class and type definitions	*/
#include <ddbdef.h>				/* Device data block definitions		*/
#include <ddtdef.h>				/* Driver dispatch table definitions		*/
#include <descrip.h>
#include <devdef.h>				/* Device characteristics definitions		*/
#include <dptdef.h>				/* Driver prologue table definitions		*/
#include <dyndef.h>				/* Data structure type definitions		*/
#include <fdtdef.h>				/* Function decision table definitions		*/				
#include <hwrpbdef.h>				/* Hardware RPB definitions			*/
#include <idbdef.h>				/* Interrupt dispatch block definitions		*/
#include <ints.h>				/* Integer type definitions			*/
#include <iocdef.h>				/* Ioc$node_data function code definitions	*/
#include <iodef.h>				/* I/O function code definitions		*/
#include <iohandledef.h>			/* I/O mapping handle definitions		*/
#include <irpdef.h>				/* I/O request packet definitions		*/
#include <fkbdef.h>				/* Fork block definitions			*/
#include <orbdef.h>				/* Object rights block definitions		*/				
#include <pcbdef.h>				/* Process control block definitions		*/
#include <pcidef.h>				/* PCI bus definitions				*/
#include <PRTDEF.H>
#include <PSLDEF.H>
#include <scsidef.h>				/* SCSI definitions				*/
#include <scdtdef.h>				/* SCSI connection descriptor table definitions	*/
#include <scdrpdef.h>				/* SCSI class driver request packet definitions	*/
#include <spdtdef.h>				/* SCSI port descriptor table definitions	*/
#include <stdtdef.h>				/* SCSI target descriptor table definitions	*/
#include <spldef.h>				/* Spinlock control block definitions		*/
#include <splcoddef.h>				/* Spinlock index definitions			*/
#include <ssdef.h>				/* System service status code definitions	*/
#include <stsdef.h>				/* System service status definitions	*/
#include <stddef.h>				/* ISO common definitions			*/
#include <string.h>				/* String processing function definitions	*/
#include <ucbdef.h>				/* Unit control block definitions		*/
#include <VA_RANGEDEF.H>
#include <vecdef.h>				/* CRB interrupt transfer vector definitions	*/

/* Include files which define exec routine prototypes, from SYS$LIB_C.TLB */

#include <exe_routines.h>
#include <ioc_routines.h>
#include <sch_routines.h>
#include <smp_routines.h>
#include <STARLET.H>

/* Driver Macros from SYS$LIB_C.TLB */

#include <vms_drivers.h>
#include <vms_macros.h>

/* Include files from DECC$RTLDEF.TLB */

#include <builtins.h>

extern void exe_std$outbyte(uint8);
extern void exe_std$outhex(uint64);
extern void exe_std$outchar(char);
extern void exe_std$outcstring( char * );		/* ** TEMP m/b defined in exe_routines.h */
extern void exe_std$outcrlf();			/* ** TEMP m/b defined in exe_routines.h */

typedef struct {
    UCB     ucb$r_ucb;                   /* Generic UCB */
    uint64  hIO;
    uint64  virtual_base;
    uint64  physical_base;
} SST_UCB;

void SST$struc_init(CRB *crb, DDB *ddb, IDB *idb, ORB *orb, SST_UCB *ucb);
void SST$startio(IRP *irp, SST_UCB *ucb);
int SST$unit_init(IDB *idb, SST_UCB *ucb);
void SST$struc_reinit (CRB *crb, DDB *ddb, IDB *idb, ORB *orb, UCB *ucb);

void SST$print_message(SST_UCB *ucb, char *message, int severity, int crlf);

int SST$write (IRP *irp, PCB *pcb, SST_UCB *ucb, CCB *ccb);
int SST$read (IRP *irp, PCB *pcb, SST_UCB *ucb, CCB *ccb);
int SST$getLinearPtr (IRP *irp, PCB *pcb, SST_UCB *ucb, CCB *ccb);