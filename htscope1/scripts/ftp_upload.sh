#!/bin/bash
# upload.sh

WINDOWS_IP="192.168.1.67"
LOCAL_FILE="/tmp/event-test-1-1024-1024.dat"

curl -T $LOCAL_FILE ftp://$WINDOWS_IP/ --user "$FTP_USER:$FTP_PASS"
