#!/bin/bash
# ftp_upload.sh

LOCAL_FILE=${1:-/tmp/event-test-1-1024-1024.dat}
FTP_IP="192.168.1.67"

if [ $VERBOSE ]; then
    echo FTP_IP:$FTP_IP
    echo FTP_USER:$FTP_USER
    echo FTP_PASS:$FTP_PASS
    echo LOCAL_FILE:$LOCAL_FILE

curl -T $LOCAL_FILE ftp://$FTP_IP/ --user "$FTP_USER:$FTP_PASS"
