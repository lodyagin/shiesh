#include "StdAfx.h"
#include "RConnectedSocket.h"

RConnectedSocket::RConnectedSocket (SOCKET con_socket)
   : RInOutSocket (con_socket)
{
}

/*RSocketAddress RConnectedSocket::get_peer_address ()
{

}*/
