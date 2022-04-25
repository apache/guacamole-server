#!/bin/sh -e
UBUNTU_RELEASE=$1
APT_SOURCE=""
APT_SECURITY_SOURCE=""
test -n "$APT_SOURCE" && test -n "$APT_SECURITY_SOURCE"  || ( echo "Please set mirror repo location when you set USE_DEFAULT_REPO to false" && exit 1)
echo "Add apt mirror $APT_SOURCE $APT_SECURITY_SOURCE"
cp /etc/apt/sources.list  /etc/apt/sources.list.bak
cat <<EOF > /etc/apt/sources.list  
    deb ${APT_SOURCE} ${UBUNTU_RELEASE} main restricted universe multiverse 
    deb ${APT_SOURCE}  ${UBUNTU_RELEASE}-updates main restricted universe multiverse
    deb ${APT_SECURITY_SOURCE}  ${UBUNTU_RELEASE}-security main restricted universe multiverse 
    deb ${APT_SOURCE}  ${UBUNTU_RELEASE}-backports main restricted universe multiverse 
EOF