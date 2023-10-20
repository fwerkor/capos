#!/bin/bash  
  
echo -e "\033[32mAutomatically installing dependencies, please wait...\033[0m"
echo -e "\033[32mIf failed, please visit https://openwrt.org/docs/guide-developer/toolchain/start and install dependencies manually.\033[0m"

detect_os() {  
    if [[ -f /etc/alpine-release ]]; then  
        echo "Alpine Linux"  
    elif [[ -f /etc/arch-release ]]; then  
        echo "Manjaro Arch"  
    elif [[ -f /etc/redhat-release ]]; then  
        echo "CentOS"  
    elif [[ -f /etc/fedora-release ]]; then  
        echo "Fedora"  
    elif [[ -f /etc/lsb-release ]]; then  
        echo "Debian/Ubuntu"  
    elif [[ -f /etc/gentoo-release ]]; then  
        echo "Gentoo"  
    elif [[ -f /etc/opensuse-release ]]; then  
        echo "openSUSE"  
    elif [[ -f /etc/void-packages ]]; then  
        echo "Void"  
    elif [[ -f /usr/bin/sw_vers ]]; then  
        echo "macOS / Darwin (x86_64)+MacPorts"  
    elif [[ -f /etc/openwrt_release ]]; then  
        echo "OpenWRT"  
    else  
        echo "Unknown"  
    fi  
}
  
os=$(detect_os) 
echo -e "\033[33mSystem: $os\033[0m"  


if [[ $os == "Alpine Linux" ]]; then  
    apk add argp-standalone asciidoc bash bc binutils bzip2 cdrkit coreutils diffutils elfutils-dev findutils flex musl-fts-dev g++ gawk gcc gettext git grep gzip intltool libxslt linux-headers make musl-libintl musl-obstack-dev ncurses-dev openssl-dev patch perl python3-dev rsync tar unzip util-linux wget zlib-dev  
fi

if [[ $os == "Manjaro Arch" ]]; then  
    # Essential prerequisites  
    pacman -S --needed base-devel autoconf automake bash binutils bison bzip2 fakeroot file findutils flex gawk gcc gettext git grep groff gzip libelf libtool libxslt m4 make ncurses openssl patch pkgconf python rsync sed texinfo time unzip util-linux wget which zlib  
    # Optional prerequisites, depend on the package selection  
    pacman -S --needed asciidoc help2man intltool perl-extutils-makemaker swig 
fi
 
if [[ $os == "CentOS" || $os == "Fedora" ]]; then  
    sudo dnf --setopt install_weak_deps=False --skip-broken install bash-completion bzip2 gcc gcc-c++ git make ncurses-devel patch rsync tar unzip wget which diffutils python2 python3 perl-base perl-Data-Dumper perl-File-Compare perl-File-Copy perl-FindBin perl-IPC-Cmd perl-Thread-Queue  
fi

if [[ $os == "Debian/Ubuntu" ]]; then  
    sudo apt update
    sudo apt install build-essential ccache ecj fastjar file g++ gawk gettext git java-propose-classpath libelf-dev libncurses5-dev libncursesw5-dev libssl-dev python2.7-dev python3 unzip wget python3-distutils python3-setuptools python3-dev rsync subversion swig time xsltproc zlib1g-dev -y || sudo apt install build-essential ccache ecj fastjar file g++ gawk gettext git java-propose-classpath libelf-dev libncurses5-dev libncursesw5-dev libssl-dev python python2.7-dev python3 unzip wget python3-distutils python3-setuptools python3-dev rsync subversion swig time xsltproc zlib1g-dev  -y
fi


if [[ $os == "Gentoo" ]]; then  
    echo \
    app-arch/{bzip2,sharutils,unzip,zip} sys-process/time \
    app-text/asciidoc \
    dev-libs/{libusb-compat,libxslt,openssl} dev-util/intltool \
    dev-vcs/{git,mercurial} net-misc/{rsync,wget} \
    sys-apps/util-linux sys-devel/{bc,bin86,dev86} \
    sys-libs/{ncurses,zlib} virtual/perl-ExtUtils-MakeMaker \
    | sed "s/\s/\n/g" \
    | sort \
    | sudo tee /etc/portage/sets/openwrt-prerequisites \
    && sudo emerge -DuvNa "@openwrt-prerequisites"
fi

if [[ $os == "openSUSE" ]]; then  
    sudo zypper install --no-recommends asciidoc bash bc binutils bzip2 fastjar flex gawk gcc gcc-c++ gettext-tools git git-core intltool libopenssl-devel libxslt-tools make mercurial ncurses-devel patch perl-ExtUtils-MakeMaker python-devel rsync sdcc unzip util-linux wget zlib-devel
fi
  
if [[ $os == "Void" ]]; then  
    sudo xbps-install -Su asciidoc bash bc binutils bzip2 cdrtools coreutils diffutils findutils flex gawk gcc gettext git grep intltool libxslt linux-headers make ncurses-devel openssl-devel patch perl pkg-config python-devel python3-devel rsync tar unzip util-linux wget zlib-devel
fi


if [[ $os == "macOS / Darwin (x86_64)+MacPorts" ]]; then  
    sudo port install libiconv gettext-runtime coreutils findutils gwhich gawk zlib pcre bzip2 ncurses grep getopt gettext-tools-libs gettext diffutils sharutils util-linux libxslt libxml2 help2man readline gtime gnutar unzip zip lzma xz libelf fastjar libusb libftdi0 expat sqlite3 openssl3 openssl kerberos5 dbus lz4 libunistring nettle icu gnutls p11-kit wget quilt subversion gmake pkgconfig libzip cdrtools ccache curl xxhashlib rsync libidn perl5 p5.28-xml-parser p5.30-xml-parser p5-extutils-makemaker p5-data-dumper boost-jam boost boost-build bash bash-completion binutils m4 flex intltool patchutils swig git-extras git openjdk17 openjdk7-zulu luajit libtool glib2 file python27 python310 libzzip mercurial asciidoc sdcc gnu-classpath
fi

if [[ $os == "OpenWRT" ]]; then  
    opkg update
    opkg install pkg-config make gcc diffutils autoconf automake check git git-http patch libtool-bin  
    opkg install grep rsync tar python3 getopt procps-ng-ps gawk sed xz unzip gzip bzip2 flock wget-ssl
    opkg install perl perlbase-findbin perlbase-pod perlbase-storable perlbase-feature perlbase-b perlbase-ipc perlbase-module perlbase-extutils python3
    opkg install coreutils-nohup coreutils-install coreutils-sort coreutils-ls coreutils-realpath coreutils-stat coreutils-nproc coreutils-od coreutils-mkdir coreutils-date coreutils-comm coreutils-printf coreutils-ln coreutils-cp coreutils-split coreutils-csplit coreutils-cksum coreutils-expr coreutils-tr coreutils-test coreutils-uniq coreutils-join
    opkg install libncurses-dev zlib-dev musl-fts libzstd
    ln -s libncursesw.a /usr/lib/libncurses.a
    opkg install joe joe-extras bash htop whereis less file findutils findutils-locate chattr lsattr xxd
    chmod +x /usr/share/automake-1.16;chmod -x /usr/share/automake-1.16/COPYING /usr/share/automake-1.16/INSTALL
    chmod +x /usr/share/autoconf/Autom4te/*
    chmod +x /usr/share/libtool/build-aux/*
fi


if [[ $os == "Unknown" ]]; then  
    echo -e "\033[31mUnknown OS. Please install dependencies manually.\033[0m"  
else
    echo -e "\033[32mDone!\033[0m"
fi
