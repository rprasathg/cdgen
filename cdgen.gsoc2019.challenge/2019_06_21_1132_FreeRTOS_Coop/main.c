/******************************************************************
******************************************************************
**************#####**####*****#####**######*###****##*************
*************##******#***##**##******##*****##*#***##*************
*************#*******#****#**#****##*######*##**#**##*************
*************##******#***##**##***##*##*****##***#*##*************
**************#####**####*****######*######*##****###*************
******************************************************************
******************************************************************
*Author		:	Ram Prasath Govindarajan
*Tool 		:	CDGen_GSoC
*Version 	:	V1.0.0
*Title 		:   Main File
*Description	:	Main Fucntion and Scheduling
******************************************************************
******************************************************************/


/* System headers. */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "partest.h"
#include "taskDef.h"

/* Demo file headers. */
#include "BlockQ.h"
#include "PollQ.h"
#include "death.h"
#include "crflash.h"
#include "flop.h"
#include "print.h"
#include "fileIO.h"
#include "semtest.h"
#include "integer.h"
#include "dynamic.h"
#include "mevents.h"
#include "crhook.h"
#include "blocktim.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "countsem.h"
#include "recmutex.h"

#include "AsyncIO/AsyncIO.h"
#include "AsyncIO/AsyncIOSocket.h"
#include "AsyncIO/PosixMessageQueueIPC.h"
#include "AsyncIO/AsyncIOSerial.h"

/* TaskPriorities. */
	#define mainTask_ESSP0	( tskIDLE_PRIORITY +1 )
	#define mainTask_ESSP5	( tskIDLE_PRIORITY +2 )
	#define mainTask_ESSP4	( tskIDLE_PRIORITY +3 )
	#define mainTask_ESSP9	( tskIDLE_PRIORITY +4 )
	#define mainTask_ESSP3	( tskIDLE_PRIORITY +5 )
	#define mainTask_ESSP8	( tskIDLE_PRIORITY +6 )
	#define mainTask_ESSP1	( tskIDLE_PRIORITY +7 )
	#define mainTask_ESSP6	( tskIDLE_PRIORITY +8 )
	#define mainTask_ESSP7	( tskIDLE_PRIORITY +9 )
	#define mainTask_ESSP2	( tskIDLE_PRIORITY +10 )

#define mainDEBUG_LOG_BUFFER_SIZE	( ( unsigned short ) 20480 )

/* The number of flash co-routines to create. */
#define mainNUM_FLASH_CO_ROUTINES	( 8 )

/* The create delete Tasks routines are very expensive so they are
 * disabled unless required for testing. */
#define mainUSE_SUICIDAL_TASKS_DEMO		0

/* UDP Packet size to send/receive. */
#define mainUDP_SEND_ADDRESS		"127.0.0.1"
#define mainUDP_PORT				( 9999 )

/* Remove some of the CPU intensive tasks. */
#define mainCPU_INTENSIVE_TASKS		0

/* Task function for the "Print" task as described at the top of the file. */
static void vErrorChecks( void *pvParameters );

/* Function that checks the unique count of all the other tasks as described at
the top of the file. */
static void prvCheckOtherTasksAreStillRunning( void );

/* Constant definition used to turn on/off the pre-emptive scheduler. */
static const short sUsingPreemption = configUSE_PREEMPTION;

/* Used to demonstrate the "task switched in" callback function. */
static portBASE_TYPE prvExampleTaskHook( void * pvParameter );

/* Send/Receive UDP packets. */
static void prvUDPTask( void *pvParameters );

/* Send/Receive via Posix Message Queues for local IPC. */
static void prvMessageQueueTask( void *pvParameters );
static void vMessageQueueReceive( xMessageObject xMsg, void *pvContext );
static volatile mqd_t xMessageQueuePipeHandle = 0;

/* Send/Receive UDP packets. */
static void prvSerialConsoleEchoTask( void *pvParameters );

/* Just used to count the number of times the example task callback function is
called, and the number of times a queue send passes. */
static unsigned long uxCheckTaskHookCallCount = 0;
static unsigned long uxQueueSendPassedCount = 0;
static int iSerialReceive = 0;
/*-----------------------------------------------------------*/

//my own sleep task
void sleepTimerMs(int ticks){
	int var;
	for (var = 0; var < ticks; ++var) {
		taskENTER_CRITICAL();
		vTaskDelay(1);
		taskEXIT_CRITICAL();
	}
}



#define DELAY_MULT 100

int main( void )
{
	xTaskHandle hUDPTask, hMQTask, hSerialTask;
	xQueueHandle xUDPReceiveQueue = NULL, xIPCQueue = NULL, xSerialRxQueue = NULL;
	int iSocketReceive = 0;
	struct sockaddr_in xReceiveAddress;

//	 Initialise hardware and utilities.
	vParTestInitialise();
	vPrintInitialise();

//	 Initialise Receives sockets.
	xReceiveAddress.sin_family = AF_INET;
	xReceiveAddress.sin_addr.s_addr = INADDR_ANY;
	xReceiveAddress.sin_port = htons( mainUDP_PORT );

//	 Set-up the Receive Queue and open the socket ready to receive.
	xUDPReceiveQueue = xQueueCreate( 2, sizeof ( xUDPPacket ) );
	iSocketReceive = iSocketOpenUDP( vUDPReceiveAndDeliverCallback, xUDPReceiveQueue, &xReceiveAddress );

//	 Remember to open a whole in your Firewall to be able to receive!!!

//	 Set-up the IPC Message queue.
	xIPCQueue = xQueueCreate( 2, sizeof( xMessageObject ) );
	xMessageQueuePipeHandle = xPosixIPCOpen( "/Local_Loopback", vMessageQueueReceive, xIPCQueue );
	vPosixIPCEmpty( xMessageQueuePipeHandle );

//	 Set-up the Serial Console Echo task
	if ( pdTRUE == lAsyncIOSerialOpen( "/dev/ttyS0", &iSerialReceive ) )
	{
		xSerialRxQueue = xQueueCreate( 2, sizeof ( unsigned char ) );
		(void)lAsyncIORegisterCallback( iSerialReceive, vAsyncSerialIODataAvailableISR, xSerialRxQueue );
	}
//	 Create the co-routines that communicate with the tick hook.
	vStartHookCoRoutines();



	xTaskCreate( vTask_ESSP0, "TASK_ESSP0", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP0, NULL );
	xTaskCreate( vTask_ESSP1, "TASK_ESSP1", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP1, NULL );
	xTaskCreate( vTask_ESSP2, "TASK_ESSP2", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP2, NULL );
	xTaskCreate( vTask_ESSP3, "TASK_ESSP3", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP3, NULL );
	xTaskCreate( vTask_ESSP4, "TASK_ESSP4", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP4, NULL );
	xTaskCreate( vTask_ESSP5, "TASK_ESSP5", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP5, NULL );
	xTaskCreate( vTask_ESSP6, "TASK_ESSP6", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP6, NULL );
	xTaskCreate( vTask_ESSP7, "TASK_ESSP7", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP7, NULL );
	xTaskCreate( vTask_ESSP8, "TASK_ESSP8", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP8, NULL );
	xTaskCreate( vTask_ESSP9, "TASK_ESSP9", configMINIMAL_STACK_SIZE, NULL, mainTask_ESSP9, NULL );
	vTaskStartScheduler();
	return 0;
}


/*-----------------------------------------------------------*/

static portBASE_TYPE prvExampleTaskHook( void * pvParameter )
{
	if( pvParameter != ( void * ) 0xabcd )
	{
		/* The parameter did not contain the value we expected, so cause an
		error to be detected by setting the call count back to zero. */
		uxCheckTaskHookCallCount = 0;
	}
	else
	{
		/* Simply increment a number so we know the callback has been executed. */
		uxCheckTaskHookCallCount++;
	}

	return 0;
}
/*-----------------------------------------------------------*/

void vMainQueueSendPassed( void )
{
	/* This is just an example implementation of the "queue send" trace hook. */
	uxQueueSendPassedCount++;
}
/*-----------------------------------------------------------*/

void prvUDPTask( void *pvParameters )
{
static xUDPPacket xPacket;
struct sockaddr_in xSendAddress;
int iSocketSend, iReturn = 0, iSendTaskList = pdTRUE;
xQueueHandle xUDPReceiveQueue = (xQueueHandle)pvParameters;

	/* Open a socket for sending. */
	iSocketSend = iSocketOpenUDP( NULL, NULL, NULL );

	if ( iSocketSend != 0 )
	{
		xSendAddress.sin_family = AF_INET;
		/* Set the UDP main address to reflect your local subnet. */
		iReturn = !inet_aton( mainUDP_SEND_ADDRESS, (struct in_addr *) &( xSendAddress.sin_addr.s_addr ) );
		xSendAddress.sin_port = htons( mainUDP_PORT );

		/* Remember to open a whole in your Firewall to be able to receive!!! */

		for (;;)
		{
			if ( pdPASS == xQueueReceive( xUDPReceiveQueue, &xPacket, 2500 / portTICK_RATE_MS ) )
			{
				/* Data received. Process it. */
				xPacket.ucNull = 0;	/* Ensure the string is terminated. */
				printf( "%s", xPacket.ucPacket );
			}
			else
			{
				/* Timeout. Send the data. */
				if ( iSendTaskList )
				{
					vTaskList( xPacket.ucPacket );
				}
				else
				{
					vTaskGetRunTimeStats( xPacket.ucPacket );
				}
				iSendTaskList = !iSendTaskList;

				/* Send the Updated Tasks list. */
				iReturn = iSocketUDPSendTo( iSocketSend, &xPacket, &xSendAddress );

				if ( sizeof( xUDPPacket ) != iReturn )
				{
					printf( "UDP Failed to send whole packet: %d.\n", errno );
				}
			}
		}
	}
	else
	{
		vSocketClose( iSocketSend );
		printf( "UDP Task: Unable to open a socket.\n" );
	}

	/* Unable to open the socket. Bail out. */
	vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

void prvMessageQueueTask( void *pvParameters )
{
xMessageObject xTxMsg, xRxMsg = { { 0 } };

	sprintf( xTxMsg.cMesssageBytes, "Test message.\n" );

	for ( ;; )
	{
		if ( pdTRUE == lPosixIPCSendMessage( xMessageQueuePipeHandle, xTxMsg ) )
		{
			if ( pdTRUE != xQueueReceive( (xQueueHandle)pvParameters, &xRxMsg, 5000 / portTICK_RATE_MS ) )
			{
				printf( "MQ Task: Queue Receive Failed.\n" );
			}
			else
			{
				printf( "MQ Task: %s\n", xRxMsg.cMesssageBytes );
			}
		}
		vTaskDelay( 5000 );
	}
}
/*-----------------------------------------------------------*/

void vMessageQueueReceive( xMessageObject xMsg, void *pvContext )
{
portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if ( pdTRUE != xQueueSendFromISR( (xQueueHandle)pvContext, &xMsg, &xHigherPriorityTaskWoken ) )
	{
		printf( "MQ Rx failed.\n" );
	}
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
/*-----------------------------------------------------------*/

static void vErrorChecks( void *pvParameters )
{
portTickType xExpectedWakeTime;
const portTickType xPrintRate = ( portTickType ) 5000 / portTICK_RATE_MS;
const long lMaxAllowableTimeDifference = ( long ) 0;
portTickType xWakeTime;
long lTimeDifference;
const char *pcReceivedMessage;
const char * const pcTaskBlockedTooLongMsg = "Print task blocked too long!\r\n";

	( void ) pvParameters;

	/* Register our callback function. */
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
	vTaskSetApplicationTaskTag( NULL, prvExampleTaskHook );
#endif

	/* Loop continuously, blocking, then checking all the other tasks are still
	running, before blocking once again.  This task blocks on the queue of
	messages that require displaying so will wake either by its time out expiring,
	or a message becoming available. */
	for( ;; )
	{
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
		/* Call the task hook function. We are not interested in the return as the error check catches it. */
		(void) xTaskCallApplicationTaskHook( NULL, (void *)0xabcd );
#endif

		/* Calculate the time we will unblock if no messages are received
		on the queue.  This is used to check that we have not blocked for too long. */
		xExpectedWakeTime = xTaskGetTickCount();
		xExpectedWakeTime += xPrintRate;

		/* Block waiting for either a time out or a message to be posted that
		required displaying. */
		pcReceivedMessage = pcPrintGetNextMessage( xPrintRate );

		/* Was a message received? */
		if( pcReceivedMessage == NULL )
		{
			/* A message was not received so we timed out, did we unblock at the
			expected time? */
			xWakeTime = xTaskGetTickCount();

			/* Calculate the difference between the time we unblocked and the
			time we should have unblocked. */
			if( xWakeTime > xExpectedWakeTime )
			{
				lTimeDifference = ( long ) ( xWakeTime - xExpectedWakeTime );
			}
			else
			{
				lTimeDifference = ( long ) ( xExpectedWakeTime - xWakeTime );
			}

			if( lTimeDifference > lMaxAllowableTimeDifference )
			{
				/* We blocked too long - create a message that will get
				printed out the next time around.  If we are not using
				preemption then we won't expect the timing to be so
				accurate. */
				if( sUsingPreemption == pdTRUE )
				{
					vPrintDisplayMessage( &pcTaskBlockedTooLongMsg );
				}
			}

			/* Check the other tasks are still running, just in case. */
			prvCheckOtherTasksAreStillRunning();
		}
		else
		{
			/* We unblocked due to a message becoming available.  Send the message
			for printing. */
			vDisplayMessage( pcReceivedMessage );
		}
	}
}
/*-----------------------------------------------------------*/

static void prvCheckOtherTasksAreStillRunning( void )
{
static short sErrorHasOccurred = pdFALSE;
static unsigned long uxLastHookCallCount = 0, uxLastQueueSendCount = 0;

	if( xAreBlockingQueuesStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Blocking queues count unchanged!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreMathsTaskStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Maths task count unchanged!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xArePollingQueuesStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Polling queue count unchanged!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreSemaphoreTasksStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Semaphore take count unchanged!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreMultiEventTasksStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Error in multi events tasks!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreHookCoRoutinesStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Error in tick hook to co-routine communications!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreBlockTimeTestTasksStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Error in block time test tasks!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreQueuePeekTasksStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Error in queue peek test task!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

#if mainCPU_INTENSIVE_TASKS == 1
	if( xAreRecursiveMutexTasksStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Error in recursive mutex tasks!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreDynamicPriorityTasksStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Dynamic priority count unchanged!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreGenericQueueTasksStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Error in generic queue test task!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}

	if( xAreCountingSemaphoreTasksStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Error in counting semaphore demo task!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}
#endif /* mainCPU_INTENSIVE_TASKS */

#if mainUSE_SUICIDAL_TASKS_DEMO == 1
	if( xIsCreateTaskStillRunning() != pdTRUE )
	{
		vDisplayMessage( "Incorrect number of tasks running!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}
#endif /* mainUSE_SUICIDAL_TASKS_DEMO */

	/* The hook function associated with this task is called each time the task
	is switched in.  We therefore expect the number of times the callback
	function has been executed to have increrment since the last time this
	function executed. */
#if ( configUSE_APPLICATION_TASK_TAG == 1 )
	if( uxCheckTaskHookCallCount <= uxLastHookCallCount )
	{
		vDisplayMessage( "Error in task hook call count!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}
	else
	{
		uxLastHookCallCount = uxCheckTaskHookCallCount;
	}
#endif /* configUSE_APPLICATION_TASK_TAG */

	/* We would expect some queue sending to occur between calls of this
	function. */
	if( uxQueueSendPassedCount <= uxLastQueueSendCount )
	{
		vDisplayMessage( "Error in queue send hook call count!\r\n" );
		sErrorHasOccurred = pdTRUE;
	}
	else
	{
		uxLastQueueSendCount = uxQueueSendPassedCount;
	}

	if( sErrorHasOccurred == pdFALSE )
	{
		vDisplayMessage( "OK\n" );
	}
}
/*-----------------------------------------------------------*/

void prvSerialConsoleEchoTask( void *pvParameters )
{
xQueueHandle hSerialRxQueue = ( xQueueHandle )pvParameters;
unsigned char ucRx;
	if ( NULL != hSerialRxQueue )
	{
		for ( ;; )
		{
			if ( pdTRUE == xQueueReceive( hSerialRxQueue, &ucRx, portMAX_DELAY ) )
			{
				/* Echo it back to the sender. */
				(void)write( iSerialReceive, &ucRx, 1 );
			}
		}
	}

	/* Port wasn't opened. */
	printf( "Serial Rx Task exiting.\n" );
	vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	/* The co-routines are executed in the idle task using the idle task hook. */
	vCoRoutineSchedule();	/* Comment this out if not using Co-routines. */

#ifdef __GCC_POSIX__
	struct timespec xTimeToSleep, xTimeSlept;
		/* Makes the process more agreeable when using the Posix simulator. */
		xTimeToSleep.tv_sec = 1;
		xTimeToSleep.tv_nsec = 0;
		nanosleep( &xTimeToSleep, &xTimeSlept );
#endif
}
/*-----------------------------------------------------------*/