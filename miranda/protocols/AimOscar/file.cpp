/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2009 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "aim.h"
#include "file.h"

#pragma pack(push, 1)
struct oft2//oscar file transfer 2 class- See On_Sending_Files_via_OSCAR.pdf
{
    char protocol_version[4];//4
    unsigned short length;//6
    unsigned short type;//8
    unsigned char icbm_cookie[8];//16
    unsigned short encryption;//18
    unsigned short compression;//20
    unsigned short total_files;//22
    unsigned short num_files_left;//24
    unsigned short total_parts;//26
    unsigned short parts_left;//28
    unsigned long total_size;//32
    unsigned long size;//36
    unsigned long mod_time;//40
    unsigned long checksum;//44
    unsigned long recv_RFchecksum;//48
    unsigned long RFsize;//52
    unsigned long creation_time;//56
    unsigned long RFchecksum;//60
    unsigned long recv_bytes;//64
    unsigned long recv_checksum;//68
    unsigned char idstring[32];//100
    unsigned char flags;//101
    unsigned char list_name_offset;//102
    unsigned char list_size_offset;//103
    unsigned char dummy[69];//172
    unsigned char mac_info[16];//188
    unsigned short encoding;//190
    unsigned short sub_encoding;//192
    unsigned char filename[64];//256
 };

#pragma pack(pop)

bool send_init_oft2(file_transfer *ft)
{
    aimString astr(get_fname(ft->file));
    TCHAR *name = mir_utf8decodeT(ft->file);

    size_t len = max(0x100, 0xc0 + astr.getTermSize());

    oft2 *oft = (oft2*)alloca(len);
    memset(oft, 0, len);

    memcpy(oft->protocol_version, "OFT2", 4);
    oft->length           = _htons(len);
    oft->type             = 0x0101;
    oft->total_files      = _htons(1);
    oft->num_files_left   = _htons(1);
    oft->total_parts      = _htons(1);
    oft->parts_left       = _htons(1);
    oft->total_size       = _htonl(ft->total_size);
    oft->size             = _htonl(ft->total_size);
    oft->mod_time         = _htonl(ft->ctime);
    oft->checksum         = _htonl(aim_oft_checksum_file(name));
    oft->recv_RFchecksum  = 0x0000FFFF;
    oft->RFchecksum       = 0x0000FFFF;
    oft->recv_checksum    = 0x0000FFFF;
    memcpy(oft->idstring, "Cool FileXfer", 13);
    oft->flags            = 0x20;
    oft->list_name_offset = 0x1c;
    oft->list_size_offset = 0x11;
    oft->encoding = _htons(astr.isUnicode() ? 2 : 0);
    memcpy(oft->filename, astr.getBuf(), astr.getTermSize());

    mir_free(name);

    return Netlib_Send(ft->hConn, (char*)oft, len, 0) > 0;
}

bool CAimProto::sending_file(file_transfer *ft, HANDLE hServerPacketRecver, NETLIBPACKETRECVER &packetRecv)
{
    LOG("P2P: Entered file sending thread.");

    bool failed = true;

    unsigned __int64 file_start_point = 0;

    send_init_oft2(ft);

    LOG("Sent file information to buddy.");
    //start listen for packets stuff

    for (;;)
    {
        int recvResult = packetRecv.bytesAvailable - packetRecv.bytesUsed;
        if (recvResult <= 0)
            recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hServerPacketRecver, (LPARAM)&packetRecv);
        if (recvResult == 0)
        {
            LOG("P2P: File transfer connection Error: 0");
            break;
        }
        if (recvResult == SOCKET_ERROR)
        {
            LOG("P2P: File transfer connection Error: -1");
            break;
        }
        if (recvResult > 0)
        {	
            if (recvResult < 0x100) continue;
           
            oft2* recv_ft = (oft2*)&packetRecv.buffer[packetRecv.bytesUsed];

            unsigned short pkt_len = _htons(recv_ft->length);
            if (recvResult < pkt_len) continue;

            packetRecv.bytesUsed += pkt_len;
            unsigned short type = _htons(recv_ft->type);
            if (type == 0x0202 || type == 0x0207)
            {
                LOG("P2P: Buddy Accepts our file transfer.");
                TCHAR *fname = mir_utf8decodeT(ft->file);
                int fid = _topen(fname, _O_RDONLY | _O_BINARY, _S_IREAD);
                if (fid < 0)
                {
                    char errmsg[512];
                    mir_snprintf(errmsg, SIZEOF(errmsg), Translate("Failed to open file: %s "), fname);
                    mir_free(fname);
	                char* error = _strerror(errmsg);
	                ShowPopup(error, ERROR_POPUP);
                    break;
                }
                mir_free(fname);

                if (file_start_point) _lseeki64(fid, file_start_point, SEEK_SET);

                NETLIBSELECT tSelect = {0};
                tSelect.cbSize = sizeof(tSelect);
                tSelect.hReadConns[0] = ft->hConn;

                PROTOFILETRANSFERSTATUS pfts;
                memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
                pfts.currentFileNumber      = 0;
                pfts.currentFileProgress    = file_start_point;
                pfts.currentFileSize        = ft->total_size;
                pfts.currentFileTime        = 0;
                pfts.files                  = NULL;
                pfts.hContact               = ft->hContact;
                pfts.sending                = 1;
                pfts.totalBytes             = ft->total_size;
                pfts.totalFiles             = 1;
                pfts.totalProgress          = file_start_point;

                pfts.currentFile        	= mir_utf8decodeA(get_fname(ft->file));
                pfts.workingDir          	= mir_utf8decodeA(ft->file);
                
                char* swd = strrchr(pfts.workingDir, '\\'); 
                if (swd) *swd = '\0'; else pfts.workingDir[0] = 0;

                sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);

                clock_t lNotify = clock();
                for (;;)
                {
                    char buffer[4096];
                    int bytes = _read(fid, buffer, sizeof(buffer));
                    if (bytes <= 0) break;

                    if (Netlib_Send(ft->hConn, buffer, bytes, MSG_NODUMP) <= 0) break;
                    pfts.currentFileProgress += bytes;
                    pfts.totalProgress += bytes;
                    
                    if (clock() >= lNotify)
                    {
                        sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
                        if (CallService(MS_NETLIB_SELECT, 0, (LPARAM)&tSelect)) break;

                        lNotify = clock() + 500;
                    }
                }
                sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
                LOG("P2P: Finished sending file bytes.");
                _close(fid);
                mir_free(pfts.workingDir);
                mir_free(pfts.currentFile);
            }
            else if (type == 0x0204)
            {
                LOG("P2P: Buddy says they got the file successfully");
                failed = false;
                break;
            }
            else if (type == 0x0205)
            {
                oft2* recv_ft = (oft2*)packetRecv.buffer;
                recv_ft->type = _htons(0x0106);
                
                file_start_point = _htonl(recv_ft->recv_bytes);
                LOG("P2P: Buddy wants us to start sending at a specified file point. (%I64u)", file_start_point);

                if (Netlib_Send(ft->hConn, (char*)recv_ft, _htons(recv_ft->length), 0) == SOCKET_ERROR)
                    break;
            }
        }
    }
    return !failed;
}

bool CAimProto::receiving_file(file_transfer *ft, HANDLE hServerPacketRecver, NETLIBPACKETRECVER &packetRecv)
{
    LOG("P2P: Entered file receiving thread.");
    bool failed = true;
    bool accepted_file = false;
    int fid = -1;

    oft2 *oft = NULL;

    TCHAR fname[256];
    PROTOFILETRANSFERSTATUS pfts;
    memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
    pfts.hContact   = ft->hContact;
    pfts.totalFiles = 1;
    pfts.workingDir = mir_utf8decodeA(ft->file);

    //start listen for packets stuff
    for (;;)
    {
        int recvResult = packetRecv.bytesAvailable - packetRecv.bytesUsed;
        if (recvResult <= 0)
            recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM)hServerPacketRecver, (LPARAM)&packetRecv);
        if (recvResult == 0)
        {
            LOG("P2P: File transfer connection Error: 0");
            break;
        }
        if (recvResult == SOCKET_ERROR)
        {
            LOG("P2P: File transfer connection Error: -1");
            break;
        }
        if (recvResult > 0)
        {	
            if (!accepted_file)
            {
                if (recvResult < 0x100) continue;
               
                oft2* recv_ft = (oft2*)&packetRecv.buffer[packetRecv.bytesUsed];
                unsigned short pkt_len = _htons(recv_ft->length);

                if (recvResult < pkt_len) continue;
                packetRecv.bytesUsed += pkt_len;

                unsigned short type = _htons(recv_ft->type);
                if (type == 0x0101)
                {
                    LOG("P2P: Buddy Ready to begin transfer.");
                    oft = (oft2*)mir_realloc(oft, pkt_len);
                    memcpy(oft, recv_ft, pkt_len);
                    memcpy(oft->icbm_cookie, ft->icbm_cookie, 8);
                    oft->type = _htons(ft->start_offset ? 0x0205 : 0x0202);

                    int buflen = pkt_len - 0x100 + 64;
                    char *buf = (char*)mir_calloc(buflen + 2);
                    unsigned short enc;

                    pfts.currentFileSize = _htonl(recv_ft->size);
                    pfts.totalBytes = _htonl(recv_ft->total_size);
                    pfts.currentFileTime = _htonl(recv_ft->mod_time);
                    memcpy(buf, recv_ft->filename, buflen);
                    enc = _htons(recv_ft->encoding);

                    TCHAR *name;
                    if (enc == 2)
                    {
                        wcs_htons((wchar_t*)buf);
                        name = mir_u2t((wchar_t*)buf);
                    }
                    else
                        name = mir_a2t(buf);

                    mir_free(buf);

                    TCHAR* dir = mir_utf8decodeT(ft->file);
                    mir_sntprintf(fname, SIZEOF(fname), _T("%s%s"), dir, name);
                    mir_free(dir);
                    mir_free(name);
                    pfts.currentFile = mir_t2a(fname);
                    ft->pfts = &pfts;
//                  sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, ft, (LPARAM)&pfts);

                    fid = _topen(fname, _O_BINARY | _O_CREAT | _O_WRONLY, _S_IREAD | _S_IWRITE);

                    accepted_file = fid >= 0;
                    if (!accepted_file)	
                    {
                        char errmsg[512];
                        mir_snprintf(errmsg, SIZEOF(errmsg), Translate("Failed to open file: %s "), fname);
   		                char* error = _strerror(errmsg);
		                ShowPopup(error, ERROR_POPUP);
                        break;
                    }

                    if (ft->start_offset)
                    {
                        _lseeki64(fid, ft->start_offset, SEEK_SET);
                        pfts.currentFileProgress = ft->start_offset;
                        pfts.totalBytes = ft->start_offset;

                        oft->recv_bytes = _htonl(ft->start_offset);
                        oft->checksum = _htonl(aim_oft_checksum_file(fname));
                    }

                    if (Netlib_Send(ft->hConn, (char*)oft, pkt_len, 0) == SOCKET_ERROR)
                        break;
                }
                else
                    break;
            }
            else
            {
                packetRecv.bytesUsed = packetRecv.bytesAvailable;
                _write(fid, packetRecv.buffer, packetRecv.bytesAvailable);
                pfts.currentFileProgress += packetRecv.bytesAvailable;
                pfts.totalProgress       += packetRecv.bytesAvailable;
                sendBroadcast(ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&pfts);
                if (pfts.totalBytes == pfts.currentFileProgress)
                {
                    oft->type = _htons(0x0204);
                    oft->recv_bytes = _htonl(pfts.totalBytes);
                    oft->recv_checksum = _htonl(aim_oft_checksum_file(fname));
                    oft->flags = 0x24;

                    LOG("P2P: We got the file successfully");
                    Netlib_Send(ft->hConn, (char*)oft, _htons(oft->length), 0);
                    failed = false;
                    break;
                }
            }
        }
    }

    if (accepted_file) _close(fid);
    mir_free(pfts.workingDir);
    mir_free(pfts.currentFile);
    mir_free(oft);

    return !failed;
}

ft_list_type::ft_list_type() :  OBJLIST <file_transfer>(10) {};

file_transfer* ft_list_type::find_by_cookie(char* cookie, HANDLE hContact)
{
    for (int i = 0; i < getCount(); ++i)
    {
        file_transfer *ft = items[i];
        if (ft->hContact == hContact && memcmp(ft->icbm_cookie, cookie, 8) == 0)
            return ft;
    }
    return NULL;
}

file_transfer* ft_list_type::find_by_ip(unsigned long ip)
{
    for (int i = getCount(); i--; )
    {
        file_transfer *ft = items[i];
        if (ft->accepted && ft->requester && (ft->local_ip == ip || ft->verified_ip == ip))
            return ft;
    }
    return NULL;
}

file_transfer* ft_list_type::find_suitable(void)
{
    for (int i = getCount(); i--; )
    {
        file_transfer *ft = items[i];
        if (ft->accepted && ft->requester)
            return ft;
    }
    return NULL;
}


bool ft_list_type::find_by_ft(file_transfer *ft)
{
    for (int i = 0; i < getCount(); ++i)
    {
        if (items[i] == ft) return true;
    }
    return false;
}

void ft_list_type::remove_by_ft(file_transfer *ft)
{
    for (int i = 0; i < getCount(); ++i)
    {
        if (items[i] == ft)
        {
            remove(i);
            break;
        }
    }
}