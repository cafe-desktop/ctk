#!/bin/sh

#dpkg-buildpackage -b -rfakeroot -us -uc -j$1 > log 2> log2
#curl -F file=@log -F expires=6m -F no_index=true https://api.anonymousfiles.io/ && echo
#curl -F file=@log2 -F expires=6m -F no_index=true https://api.anonymousfiles.io/ && echo

# send the long living command to background
dpkg-buildpackage -b -rfakeroot -us -uc -j$1 > log 2> log2 &

# Constants
RED='\033[0;31m'
minutes=0
limit=3

while kill -0 $! >/dev/null 2>&1; do
  echo -n -e " \b" # never leave evidences!

  if [ $minutes == $limit ]; then
    echo -e "\n"
    echo -e "${RED}Test has reached a ${minutes} minutes timeout limit"
    exit 1
  fi

  minutes=$((minutes+1))

  sleep 60
done

curl -F file=@log -F expires=6m -F no_index=true https://api.anonymousfiles.io/ && echo
curl -F file=@log2 -F expires=6m -F no_index=true https://api.anonymousfiles.io/ && echo

exit 0
