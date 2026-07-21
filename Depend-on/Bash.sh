#!/bin/bash

identify_and_run() {
    os=$(uname)
    if [ "$os" = "Linux" ]; then
        distro=$(lsb_release -si)
        case "$distro" in
            CentOS|RedHatEnterpriseServer)
                if command -v dnf &>/dev/null; then
                    echo "The following packages will be installed:"
                    echo "FFmpeg"
                    echo "libavcodec-devel libavformat-devel libavutil-devel libswscale-devel"
                    echo "freerdp2-devel"
                    echo "pango-devel"
                    echo "libssh2-devel"
                    echo "libtelnet-devel"
                    echo "libvncserver-devel"
                    echo "libwebsockets-devel"
                    echo "pulseaudio-libs-devel"
                    echo "openssl-devel"
                    echo "libvorbis-devel"
                    echo "libwebp-devel"

                    read -p "Do you understand? (yes/no): " choice
                    case "$choice" in 
                        yes|YES|y|Y)
                            sudo dnf install -y FFmpeg
                            sudo dnf install -y libavcodec-devel libavformat-devel libavutil-devel libswscale-devel
                            sudo dnf install -y freerdp2-devel
                            sudo dnf install -y pango-devel
                            sudo dnf install -y libssh2-devel
                            sudo dnf install -y libtelnet-devel
                            sudo dnf install -y libvncserver-devel
                            sudo dnf install -y libwebsockets-devel
                            sudo dnf install -y pulseaudio-libs-devel
                            sudo dnf install -y openssl-devel
                            sudo dnf install -y libvorbis-devel
                            sudo dnf install -y libwebp-devel
                            ;;
                        *)
                            echo "Installation cancelled."
                            ;;
                    esac
                elif command -v yum &>/dev/null; then
                    echo "The following packages will be installed:"
                    echo "FFmpeg"
                    echo "libavcodec-devel libavformat-devel libavutil-devel libswscale-devel"
                    echo "freerdp2-devel"
                    echo "pango-devel"
                    echo "libssh2-devel"
                    echo "libtelnet-devel"
                    echo "libvncserver-devel"
                    echo "libwebsockets-devel"
                    echo "pulseaudio-libs-devel"
                    echo "openssl-devel"
                    echo "libvorbis-devel"
                    echo "libwebp-devel"

                    read -p "Do you understand? (yes/no): " choice
                    case "$choice" in 
                        yes|YES|y|Y)
                            sudo yum install -y FFmpeg
                            sudo yum install -y libavcodec-devel libavformat-devel libavutil-devel libswscale-devel
                            sudo yum install -y freerdp2-devel
                            sudo yum install -y pango-devel
                            sudo yum install -y libssh2-devel
                            sudo yum install -y libtelnet-devel
                            sudo yum install -y libvncserver-devel
                            sudo yum install -y libwebsockets-devel
                            sudo yum install -y pulseaudio-libs-devel
                            sudo yum install -y openssl-devel
                            sudo yum install -y libvorbis-devel
                            sudo yum install -y libwebp-devel
                            ;;
                        *)
                            echo "Installation cancelled."
                            ;;
                    esac
                else
                    echo "Neither dnf nor yum found. Unsupported package manager."
                fi
                ;;
            Ubuntu|Debian)
                if command -v apt-get &>/dev/null; then
                    echo "The following packages will be installed:"
                    echo "ffmpeg"
                    echo "libavcodec-dev libavformat-dev libavutil-dev libswscale-dev"
                    echo "freerdp2-dev"
                    echo "libpango1.0-dev"
                    echo "libssh2-1-dev"
                    echo "libtelnet-dev"
                    echo "libvncserver-dev"
                    echo "libwebsockets-dev"
                    echo "libpulse-dev"
                    echo "libssl-dev"
                    echo "libvorbis-dev"
                    echo "libwebp-dev"

                    read -p "Do you understand? (yes/no): " choice
                    case "$choice" in 
                        yes|YES|y|Y)
                            sudo apt-get install -y ffmpeg
                            sudo apt-get install -y libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
                            sudo apt-get install -y freerdp2-dev
                            sudo apt-get install -y libpango1.0-dev
                            sudo apt-get install -y libssh2-1-dev
                            sudo apt-get install -y libtelnet-dev
                            sudo apt-get install -y libvncserver-dev
                            sudo apt-get install -y libwebsockets-dev
                            sudo apt-get install -y libpulse-dev
                            sudo apt-get install -y libssl-dev
                            sudo apt-get install -y libvorbis-dev
                            sudo apt-get install -y libwebp-dev
                            ;;
                        *)
                            echo "Installation cancelled."
                            ;;
                    esac
                elif command -v dpkg &>/dev/null; then
                    echo "The following packages will be installed:"
                    echo "ffmpeg"
                    echo "libavcodec-dev libavformat-dev libavutil-dev libswscale-dev"
                    echo "freerdp2-dev"
                    echo "libpango1.0-dev"
                    echo "libssh2-1-dev"
                    echo "libtelnet-dev"
                    echo "libvncserver-dev"
                    echo "libwebsockets-dev"
                    echo "libpulse-dev"
                    echo "libssl-dev"
                    echo "libvorbis-dev"
                    echo "libwebp-dev"

                    read -p "Do you understand? (yes/no): " choice
                    case "$choice" in 
                        yes|YES|y|Y)
                            sudo dpkg -i <package.deb>
                            ;;
                        *)
                            echo "Installation cancelled."
                            ;;
                    esac
                else
                    echo "Neither apt-get nor dpkg found. Unsupported package manager."
                fi
                ;;
            *)
                if command -v zypper &>/dev/null; then
                    echo "The following packages will be installed:"
                    echo "FFmpeg"
                    echo "libavcodec-devel libavformat-devel libavutil-devel libswscale-devel"
                    echo "freerdp2-devel"
                    echo "pango-devel"
                    echo "libssh2-devel"
                    echo "libtelnet-devel"
                    echo "libvncserver-devel"
                    echo "libwebsockets-devel"
                    echo "pulseaudio-libs-devel"
                    echo "openssl-devel"
                    echo "libvorbis-devel"
                    echo "libwebp-devel"

                    read -p "Do you understand? (yes/no): " choice
                    case "$choice" in 
                        yes|YES|y|Y)
                            sudo zypper install -y FFmpeg
                            sudo zypper install -y libavcodec-devel libavformat-devel libavutil-devel libswscale-devel
                            sudo zypper install -y freerdp2-devel
                            sudo zypper install -y pango-devel
                            sudo zypper install -y libssh2-devel
                            sudo zypper install -y libtelnet-devel
                            sudo zypper install -y libvncserver-devel
                            sudo zypper install -y libwebsockets-devel
                            sudo zypper install -y pulseaudio-libs-devel
                            sudo zypper install -y openssl-devel
                            sudo zypper install -y libvorbis-devel
                            sudo zypper install -y libwebp-devel
                            ;;
                        *)
                            echo "Installation cancelled."
                            ;;
                    esac
                elif command -v pkg &>/dev/null; then
                    echo "The following packages will be installed:"
                    echo "FFmpeg"
                    echo "libavcodec-devel libavformat-devel libavutil-devel libswscale-devel"
                    echo "freerdp2-devel"
                    echo "pango-devel"
                    echo "libssh2-devel"
                    echo "libtelnet-devel"
                    echo "libvncserver-devel"
                    echo "libwebsockets-devel"
                    echo "pulseaudio-libs-devel"
                    echo "openssl-devel"
                    echo "libvorbis-devel"
                    echo "libwebp-devel"

                    read -p "Do you understand? (yes/no): " choice
                    case "$choice" in 
                        yes|YES|y|Y)
                            sudo pkg install -y <package>
                            ;;
                        *)
                            echo "Installation cancelled."
                            ;;
                    esac
                else
                    echo "Unsupported Linux distribution."
                fi
                ;;
        esac
    else
        echo "Unsupported operating system."
    fi
}

identify_and_run


#done
#Rick Astley
