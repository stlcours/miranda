/*
UPnP plugin for Miranda IM
Copyright (C) 2006 borkra

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* Main file for the Weather Protocol, includes loading, unloading,
   upgrading, support for plugin uninsaller, and anything that doesn't
   belong to any other file.
*/

#include "commonheaders.h"
#include "netlib.h"

static char search_request_msg[] = 
	"M-SEARCH * HTTP/1.1\r\n"
	"MX: 2\r\n"
	"HOST: 239.255.255.250:1900\r\n"
	"MAN: \"ssdp:discover\"\r\n"
	"ST: urn:schemas-upnp-org:service:%s\r\n"
	"\r\n";

static char xml_get_hdr[] =
	"GET %s HTTP/1.1\r\n"
	"Connection: close\r\n"
	"Host: %s:%s\r\n\r\n";

static char soap_post_hdr[] =
	"POST %s HTTP/1.1\r\n"
	"HOST: %s:%s\r\n"
	"CONTENT-LENGTH: %u\r\n"
	"CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
	"SOAPACTION: \"%s#%s\"\r\n\r\n"
	"%s";

static char soap_post_hdr_m[] =
	"M-POST %s URL HTTP/1.1\r\n"
	"HOST: %s:%s\r\n"
	"CONTENT-LENGTH: %u\r\n"
	"CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
	"MAN: \"http://schemas.xmlsoap.org/soap/envelope/\"; ns=01\r\n"
	"01-SOAPACTION: \"%s#%s\"\r\n\r\n"
	"%s";

static char search_device[] = 
	"<serviceType>%s</serviceType>";

static char soap_action[] =
	"<s:Envelope\r\n"
	"    xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n"
	"    s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
	"  <s:Body>\r\n"
	"    <u:%s xmlns:u=\"%s\">\r\n"
	"%s"
	"    </u:%s>\r\n"
	"  </s:Body>\r\n"
	"</s:Envelope>\r\n";

static char add_port_mapping[] =
	"      <NewRemoteHost></NewRemoteHost>\r\n"
	"      <NewExternalPort>%i</NewExternalPort>\r\n"
	"      <NewProtocol>%s</NewProtocol>\r\n"
	"      <NewInternalPort>%i</NewInternalPort>\r\n"
	"      <NewInternalClient>%s</NewInternalClient>\r\n"
	"      <NewEnabled>1</NewEnabled>\r\n"
	"      <NewPortMappingDescription>Miranda</NewPortMappingDescription>\r\n"
	"      <NewLeaseDuration>0</NewLeaseDuration>\r\n";

static char delete_port_mapping[] =
	"     <NewRemoteHost></NewRemoteHost>\r\n"
	"     <NewExternalPort>%i</NewExternalPort>\r\n"
	"     <NewProtocol>%s</NewProtocol>\r\n";

static char get_port_mapping[] =
	"     <NewPortMappingIndex>%i</NewPortMappingIndex>\r\n";

static char default_http_port[] = "80";

static BOOL gatewayFound = FALSE;
static SOCKADDR_IN locIP;
static time_t lastDiscTime = 0;
static int expireTime = 120;

static WORD *portList;
static unsigned numports, numportsAlloc;
HANDLE portListMutex, cleanupThread;

static char szCtlUrl[256], szDev[256];


static BOOL txtParseParam(char* szData, char* presearch, 
						  char* start, char* finish, char* param, int size)
{
	char *cp, *cp1;
	int len;
	
	*param = 0;

	if (presearch != NULL)
	{
		cp1 = strstr(szData, presearch);
		if (cp1 == NULL) return FALSE;
	}
	else
		cp1 = szData;

	cp = strstr(cp1, start);
	if (cp == NULL) return FALSE;
	cp += strlen(start);
	while (*cp == ' ') ++cp;

	cp1 = strstr(cp, finish);
	if (cp1 == NULL) return FALSE;
	while (*(cp1-1) == ' ' && cp1 > cp) --cp1;

	len = min(cp1 - cp, size);
	strncpy(param, cp, len);
	param[len] = 0;

	return TRUE;
}

void parseURL(char* szUrl, char* szHost, char* szPort, char* szPath)
{
	char *ppath, *phost, *pport;
	int sz;

	phost = strstr(szUrl,"://");
	if (phost == NULL) phost = szUrl;
	else phost += 3;
	
	ppath = strchr(phost,'/');
	if (ppath == NULL) ppath = phost + strlen(phost);
	
	pport = strchr(phost,':');
	if (pport == NULL) pport = ppath;

	if (szHost != NULL)
	{
		sz = pport - phost + 1;
		if (sz>256) sz = 256;
		strncpy(szHost, phost, sz);
		szHost[sz-1] = 0;
	}

	if (szPort != NULL)
	{
		sz = ppath - pport;
		if (sz > 1 && sz <= 5)
		{
			strncpy(szPort, pport+1, sz);
			szPort[sz-1] = 0;
		}
		else
			szPort = default_http_port;
	}

	if (szPath != NULL)
	{
		strncpy(szPath, ppath, 256);
		szPort[255] = 0;
	}
}


static LongLog(char* szData)
{
	char* buf = szData;
	int sz = strlen(szData);

	while ( sz > 1000)
	{
		char* nbuf = buf + 1000;
		char t = *nbuf;
		*nbuf = 0;
		Netlib_Logf(NULL, buf);
		*nbuf = t;
		buf = nbuf;
		sz -= 1000;
	}
	Netlib_Logf(NULL, buf);
}


static void discoverUPnP(char* szUrl, int sizeUrl)
{
	char* buf;
	int buflen;
	unsigned i, j, nip = 0;
	char* szData = NULL;
	unsigned* ips = NULL;

	static const unsigned any = INADDR_ANY;
	fd_set readfd;
	TIMEVAL tv = { 1, 0 };

	char hostname[256];
	PHOSTENT he;

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	SOCKADDR_IN enetaddr;
	enetaddr.sin_family = AF_INET;
	enetaddr.sin_port = htons(1900);
	enetaddr.sin_addr.s_addr = inet_addr("239.255.255.250");

	FD_ZERO(&readfd);
	FD_SET(sock, &readfd);

	szUrl[0] = 0;

	gethostname( hostname, sizeof( hostname ));
	he = gethostbyname( hostname );

	if (he)
	{
		while(he->h_addr_list[nip]) ++nip;

		ips = mir_alloc(nip * sizeof(unsigned));

		for (j=0; j<nip; ++j)
			ips[j] = *(unsigned*)he->h_addr_list[j];
	}

	buf = mir_alloc(1500);

	for(i = 3;  --i && szUrl[0] == 0;) 
	{
		for (j=0; j<nip; ++j)
		{
			if (ips)
				setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&ips[j], sizeof(unsigned));

			buflen = mir_snprintf(buf, 1500, search_request_msg, "WANIPConnection:1");
			sendto(sock, buf, buflen, 0, (SOCKADDR*)&enetaddr, sizeof(enetaddr)); 
			LongLog(buf);

			buflen = mir_snprintf(buf, 1500, search_request_msg, "WANPPPConnection:1");
			sendto(sock, buf, buflen, 0, (SOCKADDR*)&enetaddr, sizeof(enetaddr)); 
			LongLog(buf);
		}

		while (select(0, &readfd, NULL, NULL, &tv) == 1) 
		{
			buflen = recv(sock, buf, 1500, 0);
			if (buflen != SOCKET_ERROR) 
			{
				buf[buflen] = 0;
				LongLog(buf);

				if (txtParseParam(buf, NULL, "LOCATION:", "\r", szUrl, sizeUrl) ||
					txtParseParam(buf, NULL, "Location:", "\r", szUrl, sizeUrl))
				{
					char age[30];
					txtParseParam(buf, NULL, "ST:", "\r", szDev, sizeof(szDev));
					txtParseParam(buf, "max-age", "=", "\r", age, sizeof(age));
					expireTime = atoi(age);
					break;
				}
			}
		}
	}

	mir_free(buf);
	mir_free(ips);
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&any, sizeof(unsigned));
	closesocket(sock);
}


static int httpTransact (char* szUrl, char* szResult, int resSize, char* szActionName)
{
	// Parse URL
	char szHost[256], szPort[6], szPath[256], szRes[6];
	int sz, res = 0;

	char* szPostHdr = soap_post_hdr;
	char* szData = mir_alloc(4096);
	char* szReq = szActionName ? mir_strdup(szResult) : NULL;
	szResult[0] = 0;

	parseURL(szUrl, szHost, szPort, szPath);

	for (;;)
	{
		if (szActionName == NULL) 
			sz = mir_snprintf (szData, 4096,
				xml_get_hdr, szPath, szHost, szPort);
		else
		{
			char szData1[1024];
			
			sz = mir_snprintf (szData1, sizeof(szData1),
				soap_action, szActionName, szDev, szReq, szActionName);

			sz = mir_snprintf (szData, 4096,
				szPostHdr, szPath, szHost, szPort, 
				sz, szDev, szActionName, szData1);
		}

		{
			static TIMEVAL tv = { 3, 0 };
			static unsigned ttl = 4;
			fd_set readfd;

			SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

			SOCKADDR_IN enetaddr;
			enetaddr.sin_family = AF_INET;
			enetaddr.sin_port = htons((unsigned short)atol(szPort));
			enetaddr.sin_addr.s_addr = inet_addr(szHost);

			if (enetaddr.sin_addr.s_addr == INADDR_NONE)
			{
				PHOSTENT he = gethostbyname(szHost);
				if (he) 
					enetaddr.sin_addr.s_addr = *(unsigned*)he->h_addr_list[0];
			}

			Netlib_Logf(NULL, "UPnP HTTP connection Host: %s Port: %s\n", szHost, szPort); 

			FD_ZERO(&readfd);
			FD_SET(sock, &readfd);

			setsockopt(sock, IPPROTO_IP, IP_TTL, (char *)&ttl, sizeof(unsigned));

			if (connect(sock, (SOCKADDR*)&enetaddr, sizeof(enetaddr)) == 0)
			{
				if (send( sock, szData, sz, 0 ) != SOCKET_ERROR)
				{
					LongLog(szData);
					sz = 0;
					for(;;) 
					{
						int bytesRecv;
						char *hdrend;

						if (select(0, &readfd, NULL, NULL, &tv) != 1) 
						{
							Netlib_Logf(NULL, "UPnP select timeout"); 
							break;
						}

						bytesRecv = recv( sock, &szResult[sz], resSize-sz, 0 );
						if ( bytesRecv == 0 || bytesRecv == SOCKET_ERROR) 
							break;
						else
							sz += bytesRecv;

						if (sz >= (resSize-1)) 
						{
							szResult[resSize-1] = 0;
							break;
						}
						else
							szResult[sz] = 0;

						hdrend = strstr(szResult, "\r\n\r\n");
						if (hdrend != NULL &&
						   (txtParseParam(szResult, NULL, "Content-Length:", "\r", szRes, sizeof(szRes)) ||
						    txtParseParam(szResult, NULL, "CONTENT-LENGTH:", "\r", szRes, sizeof(szRes))))
						{
							int pktsz = atol(szRes) + (hdrend - szResult + 4);
							if (sz >= pktsz)
							{
								szResult[pktsz] = 0;
								break;
							}
						}

					}
					LongLog(szResult);
				}
				else
					Netlib_Logf(NULL, "UPnP send failed %d", WSAGetLastError()); 
			}
			else
				Netlib_Logf(NULL, "UPnP connect failed %d", WSAGetLastError()); 

			if (szActionName == NULL) 
			{
				int len = sizeof(locIP);
				getsockname(sock, (SOCKADDR*)&locIP, &len);
			}

			shutdown(sock, 2);
			closesocket(sock);
		}
		txtParseParam(szResult, "HTTP", " ", " ", szRes, sizeof(szRes));
		res = atol(szRes);
		if (szActionName != NULL && res == 405 && szPostHdr == soap_post_hdr)
			szPostHdr = soap_post_hdr_m;
		else
			break;
	}

	mir_free(szData);
	mir_free(szReq);
	return res;
}


static void findUPnPGateway(void)
{
	time_t curTime = time(NULL);

	if ((curTime - lastDiscTime) >= expireTime)
	{
		char szUrl[256];
		char* szData = mir_alloc(8192);

		lastDiscTime = curTime;

		discoverUPnP(szUrl, sizeof(szUrl));
		
		gatewayFound = szUrl[0] != 0;

		if (gatewayFound)
		{
			char szHostNew[256], szHostExist[256];
			parseURL(szUrl, szHostNew, NULL, NULL);
			parseURL(szCtlUrl, szHostExist, NULL, NULL);

			if (strcmp(szHostNew, szHostExist) == 0)
				return;
			else
				txtParseParam(szUrl, NULL, "http://", "/", szCtlUrl, sizeof(szCtlUrl));
		}
		else
			return;
		
		gatewayFound = httpTransact(szUrl, szData, 8192, NULL) == 200;

		if (gatewayFound)
		{
			char szTemp[256];
			size_t ctlLen;

			txtParseParam(szData, NULL, "<URLBase>", "</URLBase>", szTemp, sizeof(szTemp));
			if (szTemp[0] != 0) strcpy(szCtlUrl, szTemp);
			ctlLen = strlen(szCtlUrl);
			if (ctlLen > 0 && szCtlUrl[ctlLen-1] == '/')
				szCtlUrl[--ctlLen] = 0;

			mir_snprintf(szTemp, sizeof(szTemp), search_device, szDev);
			txtParseParam(szData, szTemp, "<controlURL>", "</controlURL>", szUrl, sizeof(szUrl));
			switch (szUrl[0])
			{
				case 0:
					gatewayFound = FALSE;
					break;

				case '/': 
					strncat(szCtlUrl, szUrl, sizeof(szCtlUrl) - ctlLen);
					szCtlUrl[sizeof(szCtlUrl)-1] = 0;
					break;

				default: 
					strncpy(szCtlUrl, szUrl, sizeof(szCtlUrl));
					szCtlUrl[sizeof(szCtlUrl)-1] = 0;
					break;
			}
		}
		Netlib_Logf(NULL, "UPnP Gateway detected %d, Control URL: %s\n", gatewayFound, szCtlUrl); 
		mir_free(szData);
	}
}


BOOL NetlibUPnPAddPortMapping(WORD intport, char *proto, 
							  WORD *extport, DWORD *extip, BOOL search)
{
	int res = 0;

	findUPnPGateway();

	if (gatewayFound)
	{
		char* szData = mir_alloc(4096);
		char szExtIP[30];

		*extport = intport - 1;
		*extip = ntohl(locIP.sin_addr.S_un.S_addr);

		WaitForSingleObject(portListMutex, INFINITE);

		do {
			++*extport;
			mir_snprintf(szData, 4096, add_port_mapping, 
				*extport, proto, intport, inet_ntoa(locIP.sin_addr));
			res = httpTransact(szCtlUrl, szData, 4096, "AddPortMapping");
		} while (search && res == 718);
		
		if (res == 200)
		{
			szData[0] = 0;
			res = httpTransact(szCtlUrl, szData, 4096, "GetExternalIPAddress");
			if (res == 200 && txtParseParam(szData, "<NewExternalIPAddress", ">", "<", szExtIP, sizeof(szExtIP)))
				*extip = ntohl(inet_addr(szExtIP));

			if (numports >= numportsAlloc)
				mir_realloc(portList, sizeof(WORD)*(numportsAlloc += 10));
			portList[numports++] = *extport;
		}

		mir_free(szData);
		ReleaseMutex(portListMutex);
	}

	return res == 200;
}


void NetlibUPnPDeletePortMapping(WORD extport, char* proto)
{
	if (extport != 0)
	{
		unsigned i;
//		findUPnPGateway();

		if (gatewayFound)
		{
			char* szData = mir_alloc(4096);
			
			WaitForSingleObject(portListMutex, INFINITE);

			mir_snprintf(szData, 4096, delete_port_mapping, 
				extport, proto);
			httpTransact(szCtlUrl, szData, 4096, "DeletePortMapping");

			for (i=0; i<numports; ++i)
			{
				if ( portList[i] == extport && --numports > 0)
					memmove(&portList[i], &portList[i+1], (numports - i)*sizeof(WORD));
			}
			ReleaseMutex(portListMutex);

			mir_free(szData);
		}
	}
}

static void NetlibUPnPCleanup(void* extra)
{
	findUPnPGateway();

	if ( gatewayFound ) {
		char* szData = mir_alloc(4096);
		char buf[50], lip[50];
		unsigned i, j = 0, k;
		
		WORD ports[30];

		strcpy(lip, inet_ntoa(locIP.sin_addr));

		for (i=0; !Miranda_Terminated(); ++i)  {
			mir_snprintf(szData, 4096, get_port_mapping, i);

			ReleaseMutex(portListMutex);
			WaitForSingleObject(portListMutex, INFINITE);

			if (httpTransact(szCtlUrl, szData, 4096, "GetGenericPortMappingEntry") != 200)
				break;

			if (!txtParseParam(szData, "<NewPortMappingDescription", ">", "<", buf, sizeof(buf)) || strcmp(buf, "Miranda") != 0)
				continue;

			if (!txtParseParam(szData, "<NewInternalClient", ">", "<", buf, sizeof(buf)) || strcmp(buf, lip) != 0)
				continue;

			if (txtParseParam(szData, "<NewExternalPort", ">", "<", buf, sizeof(buf))) {
				WORD mport = (WORD)atol(buf);

				for (k=0; k<numports; ++k)
					if ( portList[k] == mport)
						break;

				if (k >= numports && j < 30)
					ports[j++] = mport;
			}
		}
		ReleaseMutex(portListMutex);
		mir_free(szData);

		for (i=0; i<j && !Miranda_Terminated(); ++i) { 
			WaitForSingleObject(portListMutex, INFINITE);
			NetlibUPnPDeletePortMapping(ports[i], "TCP");
			ReleaseMutex(portListMutex);
		}
	}

	// this handle will be closed automatically by _endthread()
	cleanupThread = NULL;
}

void NetlibUPnPInit(void)
{
	numports = 0;
	numportsAlloc = 10;
	portList = mir_alloc(sizeof(WORD)*numportsAlloc);
	
	portListMutex = CreateMutex(NULL, FALSE, NULL);

	cleanupThread = (HANDLE)forkthread(NetlibUPnPCleanup, 0, NULL);
}

void NetlibUPnPDestroy(void)
{
	if ( cleanupThread != NULL )
		WaitForSingleObject(cleanupThread, INFINITE);

	mir_free(portList);
	CloseHandle(portListMutex);
}
