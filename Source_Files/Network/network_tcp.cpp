/*
 *  network_tcp.cpp - TCP network functions (corresponds to AppleTalk ADSP)

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "GNU_GeneralPublicLicense.txt",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

*/
/*
 *  network_tcp.cpp - TCP network functions (corresponds to AppleTalk ADSP)
 *
 *  Written in 2000 by Christian Bauer
 *
 *  Sept-Nov 2001 (Woody Zenfell):
 *     + Now storing server_socket_set, a socket_set with just the server_socket in it,
 *       so we can use CheckSockets() to see if a connection request has arrived.
 *     + Changes to ADSPRead/Write to correctly work with nonblocking TCP (a single TCP
 *       read/write may only get partial data; need to loop since caller expects complete).
 */

#include "cseries.h"
#include "sdl_network.h"


/*
 *  Initialize/shutdown module
 */

OSErr NetADSPOpen(void)
{
//printf("NetADSPOpen\n");
	// nothing to do
	return 0;
}

OSErr NetADSPClose(void)
{
//printf("NetADSPClose\n");
	// nothing to do
	return 0;
}


/*
 *  Create connection endpoint
 */

OSErr NetADSPEstablishConnectionEnd(ConnectionEndPtr *connection)
{
//printf("NetADSPEstablishConnectionEnd\n");
	// Allocate endpoint data structure
	ConnectionEndPtr connectionEnd = (ConnectionEndPtr)malloc(sizeof(ConnectionEnd));
	if (connectionEnd == NULL)
		return -1;
	connectionEnd->socket = connectionEnd->server_socket = NULL;
        connectionEnd->server_socket_set	= NULL;

	*connection = connectionEnd;
	return 0;
}


/*
 *  Dispose of connection endpoint
 */

OSErr NetADSPDisposeConnectionEnd(ConnectionEndPtr connectionEnd)
{
//printf("NetADSPDisposeConnectionEnd\n");
	if (connectionEnd == NULL)
		return 0;

	// Close sockets
	if (connectionEnd->socket)
		SDLNet_TCP_Close(connectionEnd->socket);
	if (connectionEnd->server_socket)
		SDLNet_TCP_Close(connectionEnd->server_socket);
        if(connectionEnd->server_socket_set != NULL)
            SDLNet_FreeSocketSet(connectionEnd->server_socket_set);

	// Free endpoint data structure
	free(connectionEnd);
	return 0;
}


/*
 *  Open connection to remote machine
 */

OSErr NetADSPOpenConnection(ConnectionEndPtr connectionEnd, AddrBlock *address)
{
//printf("NetADSPOpenConnection\n");
	// Open socket
	connectionEnd->socket = SDLNet_TCP_Open(address);
	return connectionEnd->socket ? 0 : -1;
}


/*
 *  Close connection to remote machine
 */

OSErr NetADSPCloseConnection(ConnectionEndPtr connectionEnd, bool abort)
{
//printf("NetADSPCloseConnection\n");
	// Close socket
	if (connectionEnd->socket) {
		SDLNet_TCP_Close(connectionEnd->socket);
		connectionEnd->socket = NULL;
	}
	return 0;
}


/*
 *  Open server socket, wait (asynchronously) for connection from remote machine
 */

// ZZZ: originally, this closed and reopened the server_socket.
// I am going to use this in such a way that, if the server_socket still exists when this is called,
// we leave it alone.  I _think_ we are probably getting a problem with trying to reuse the same port
// too soon after it was previously used?  Maybe?  The call to SDLNet_TCP_Open() was failing if we
// had just called NetADSPCloseConnection and called this with the server_socket still in place.
OSErr NetADSPWaitForConnection(ConnectionEndPtr connectionEnd)
{
//printf("NetADSPWaitForConnection\n");
	// Close old sockets
	if (connectionEnd->socket) {
		SDLNet_TCP_Close(connectionEnd->socket);
		connectionEnd->socket = NULL;
	}
/*	if (connectionEnd->server_socket) {
		SDLNet_TCP_Close(connectionEnd->server_socket);
		connectionEnd->server_socket = NULL;
	}
        if(connectionEnd->server_socket_set != NULL) {
            SDLNet_FreeSocketSet(connectionEnd->server_socket_set);
            connectionEnd->server_socket_set = NULL;
        }
*/

        // ZZZ code follows
        if(connectionEnd->server_socket == NULL) {
            // Open server socket
            //PORTGUESS SDLNet_Open seems to like port in host byte order
            // NOPE, WRONG, only SDLNet_UDP_Open wants port in host byte order.  All other uses of
            // port in SDL_net are network byte order.
            IPaddress addr;
            addr.host = INADDR_NONE;
            addr.port = SDL_SwapBE16(DEFAULT_PORT);
            
            connectionEnd->server_socket = SDLNet_TCP_Open(&addr);
    
            if(connectionEnd->server_socket != NULL) {
                connectionEnd->server_socket_set = SDLNet_AllocSocketSet(1);
                SDLNet_TCP_AddSocket(connectionEnd->server_socket_set, connectionEnd->server_socket);
            }
        }

	return connectionEnd->server_socket ? 0 : -1;
}


/*
 *  Check status of connection, return peer address
 */

bool NetADSPCheckConnectionStatus(ConnectionEndPtr connectionEnd, AddrBlock *address)
{
        if(connectionEnd->socket == NULL) {
            // (ZZZ) Check for new connection
            
            // Note: this usage is in line with UNIX sockets "select()" usage, but is not officially mentioned
            // in the SDL_Net API.  I hope it works on all platforms anyway!  Else, there's no non-blocking
            // way to look for a connection request.
            // Revision: actually, most operations (including TCP operations) in SDL_net seem to be in
            // non-blocking mode.  So, maybe it's sufficient to call TCP_Accept directly?
            if(SDLNet_CheckSockets(connectionEnd->server_socket_set, 0) > 0) {
                // Yee-haw!  There's an incoming connection request.
                connectionEnd->socket = SDLNet_TCP_Accept(connectionEnd->server_socket);
            }
        }
        
	if (connectionEnd->socket) {
		if (address) {
			AddrBlock *remote = SDLNet_TCP_GetPeerAddress(connectionEnd->socket);
			if (remote)
				*address = *remote;
		}
		return true;
	} else
		return false;
}



// ZZZ: changes to functions below to account for SDL_net non-blocking TCP behavior.
// Results in 10 sec waiting time
#define	kNUM_TCP_RETRIES	1000


/*
 *  Write data to stream
 */

OSErr NetADSPWrite(ConnectionEndPtr connectionEnd, void *buffer, uint16 *count)
{
#ifdef DEBUG
	printf("NetADSPWrite %d: ", *count);
#endif

	int actual		= 0;
	int num_tries	= 0;
	// Since sizeof(void) is not standard
	char* buf		= (char*) buffer;

	while(actual < *count && num_tries < kNUM_TCP_RETRIES) {
		int recent = SDLNet_TCP_Send(connectionEnd->socket, &buf[actual], *count - actual);
		
		if(recent < 0)
			break;

		actual += recent;

		if(actual < *count) {
#ifdef DEBUG
			printf("z");
#endif
			SDL_Delay(10);
		}

		num_tries++;
	}
	
#ifdef DEBUG
	printf("%d\n", actual);
#endif
	
	return actual == *count ? 0 : -1;
}


/*
 *  Read data from stream
 */

OSErr NetADSPRead(ConnectionEndPtr connectionEnd, void *buffer, uint16 *count)
{
#ifdef DEBUG
	printf("NetADSPRead %d: ", *count);
#endif

	int actual		= 0;
	int num_tries	= 0;
	// Since sizeof(void) is not standard
	char* buf		= (char*) buffer;

	while(actual < *count && num_tries < kNUM_TCP_RETRIES) {
		int recent = SDLNet_TCP_Recv(connectionEnd->socket, &buf[actual], (*count) - actual);

		if(recent > 0)
                    actual += recent;

		if(actual < *count) {
#ifdef DEBUG
			printf("z");
#endif
			SDL_Delay(10);
		}

		num_tries++;
	}

	*count = actual;

#ifdef DEBUG
	printf("%d\n", actual);
#endif

	return actual > 0 ? 0 : -1;
}
