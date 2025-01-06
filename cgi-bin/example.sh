#!/bin/bash
echo "Content-type: text/html"
echo ''
echo 'CGI Bash example<br>'
echo `df -h / : grep -v Filesystem`
